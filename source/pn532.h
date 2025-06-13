#ifndef PN532_H
#define PN532_H

#include <deque>
#include <iomanip>
#include <memory>
#include <stdint.h>
#include <stdbool.h>
#include <string>
#include <vector>

#include "pn532_backend.h"

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
 *     - commande byte (DATA[0]) = request command byte + 1
 *  4. No need to ACK/NACK
 * 
 * HW
 *  - uart_1, uart_0
 *  - i2c 1 & 2, 400KHz
 *    addr 0x24 7 bit address, ie 0x48 for write 0x49 for read
 */
struct pn532_t
{
    typedef enum {
        uart, i2c
    } backend;
    
    /**
     * @param dev_num uart/i2c bus number
     * @param p1 uart RX or i2c SCL
     * @param p2 uart TX oe i2c SDA
     * @param ba backend enum uart/i2c
     * @param key target NDEF text record tag, if present unlocks
     */
    pn532_t(int dev_num, int p1, int p2, backend be, std::string key);

    uint32_t version();
    const std::string name() const { return name_; }

    /**
     * Setup PN532 to look for passive target
     * Meaning it will write to UART if found any
     * @warning need to rewind() after each finding
     */
    void rewind();

    /**
     * Check if a nearby NFC tag has the target text
     * @return true if find NDEF text record with key
     * @warning need to rewind if actually found a tag
     */
    bool key_in_tag();

protected:
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

    uint8_t read_reg(uint16_t reg);
    void write_reg(uint16_t reg, uint8_t value);

    /**
     * i2c or uart are the 2 backend possible
     * pico has 2 uart and 2 i2c bus
     */
    std::shared_ptr<pn532_backend_t> backend_;

    /** Number of tags hit */
    uint32_t tag_cnt_;
    uint32_t last_id_read_;

    std::string name_;

    /**
     * Chevron locked as long as this tag is in range
     */
    std::string key_;
};

#endif // PN532_H
