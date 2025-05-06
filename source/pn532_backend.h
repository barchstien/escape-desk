#pragma once

#include <vector>

struct pn532_backend_t
{
    /**
     * @param i2c/uart num
     * @param i2c SCL, uart RX
     * @param i2c SDA, uart TX
     */
    virtual void init(int, int, int) = 0;

    /** 
     * @return 1 byte or -1 if error
     */
    virtual int16_t read_byte(unsigned int timeout_msec) = 0;

    virtual void write_bytes(const uint8_t* data, int len) = 0;
};