#ifndef PN532_H
#define PN532_H

#include <deque>
#include <iomanip>
#include <memory>
#include <stdint.h>
#include <stdbool.h>
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
 *     - TFI = request TFI + 1
 *  4. No need to ACK/NACK
 * 
 * HW
 *  - uart_1, uart_0 doesn't work, probably coz it colides with usb/serial
 *  - i2c 1 & 2, 400KHz
 *    addr 0x24, ie 0x48 for write 0x49 for read
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
     * @param tag target tag id
     */
    pn532_t(int dev_num, int p1, int p2, backend be, uint32_t tag);

    uint32_t version();
    const std::string name() const { return name_; }

    /**
     * Setup PN532 to look for passive target
     * Meaning it will write to UART if found any
     * @warning need to rewind() after each finding
     */
    void rewind();

    /**
     * @return tag or 0 if nothing
     * @warning need to rewind if actually found a tag
     */
    uint32_t get_tag();

    /**
     * @return true if target tag is in range
     * @warning need to rewind if actually is in range
     */
    bool has_target_tag()
    {
        return get_tag() == target_tag_;
    }
    
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

    /**
     * i2c or uart are the 2 backend possible
     * pico has 2 uart and 2 i2c bus
     */
    std::shared_ptr<pn532_backend_t> backend_;

    /** Number of tags hit */
    uint32_t tag_cnt_;

    std::string name_;

    /**
     * Chevron locked as long as this tag is in range
     */
    uint32_t target_tag_;
};

#endif // PN532_H
