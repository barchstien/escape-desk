#ifndef PN532_H
#define PN532_H

#include <deque>
#include <iomanip>
//#include <memory>
#include <stdint.h>
#include <stdbool.h>
//#include <sstream>
#include <vector>

#include "hardware/uart.h"

//#define PN532_MIFARE_ISO14443A 0x00

static inline void hexdump(std::vector<uint8_t>& data)
{
    for (int i=0; i<data.size(); i++)
    {
        printf("%#x ", data[i]);
    }
}

static inline void hexdump(std::deque<uint8_t>& data)
{
    for (int i=0; i<data.size(); i++)
    {
        printf("%#x ", data[i]);
    }
}

#if 0
typedef struct {
    uart_inst_t *uart;
} PN532;

//void pn532_uart_init();
uint32_t pn532_getFirmwareVersion(PN532 *dev);
void pn532_SAMConfig(PN532 *dev);
bool pn532_readPassiveTargetID(PN532 *dev, uint8_t cardbaudrate, uint8_t *uid, uint8_t *uidLength);
#endif

/**
 * Command and control PN532
 * Data Format
 *  - preamble  00
 *  - Start     00 FF
 *  - LEN       DATA + TFI
 *  - LCS       (0x100 - LEN) & 0xFF
 *  - TFI       0xD4 if hots->PN532, else 0xD5
 *  - DATA      LEN - 1 bytes
 *  - DCS       0x100 - (0xff & SUM(TF1 + Data_n))
 *  - postamble 00
 * ACK/NACK
 *  - preamble  00
 *  - Start     00 FF 
 *  - OK/NOK    00 FF / FF 00
 *  - postamble 00
 * 
 * Sequence : 
 *  1. send command
 *  2. get ACK/NACK
 *  3. get result
 *     - TFI is 0xD5
 *     - TFI = request TFI + 1
 *  4. No need to ACK/NACK
 */
struct pn532_t
{
    //
    pn532_t(uart_inst_t* u, int rx_pin, int tx_pin);

    uint32_t version();

    void loop_for_tag();

    void wakeup();
    
    /** Write a frame command/data */
    void write_frame(const uint8_t* data, int len, int preamble_len);
    
    /** 
     * Read the frame, verify integrity
     * @return data without preamble LEN LCS TFI DCS postamble
     *         or 0x00ff for ACK, or 0xff00 for NACK
     */
    std::vector<uint8_t> read_frame();

    /** 
     * @param frame 2 bytes, as received from read_frame()
     */
    static bool is_ack(std::vector<uint8_t> frame);
    static bool is_nack(std::vector<uint8_t> frame);

protected:
    uint8_t read_reg(uint16_t reg);
    void write_reg(uint16_t reg, uint8_t value);

private:
    uart_inst_t *uart_;
    int rx_pin_;
    int tx_pin_;
};

#endif // PN532_H
