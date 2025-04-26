#pragma once

#include "pn532_backend.h"

#include <hardware/i2c.h>
#include <pico/stdlib.h>

#include <stdio.h>

#define PN532_I2C_ADDRESS 0x24

struct pn532_backend_i2c_t : public pn532_backend_t
{
    static constexpr unsigned int BAUD_RATE = 400000;

    virtual void init(int i2c_num, int scl, int sda) override
    {
        i2c_ = i2c_get_instance(i2c_num);
        scl_ = scl;
        sda_ = sda;
        int b = i2c_init(i2c_, BAUD_RATE);
        printf("I2C %i set baud to %i \n", i2c_num, b);
        gpio_set_function(scl_, GPIO_FUNC_I2C);
        gpio_set_function(sda_, GPIO_FUNC_I2C);
        gpio_pull_up(scl_);
        gpio_pull_up(sda_);

        sleep_ms(1);

        //bool enabled = uart_is_enabled(i2c_);
        //printf("UART %i is enabled: %u\n", i2c_num, enabled);

        //wakeup();
    }

    virtual int16_t read_byte(unsigned int timeout_msec) override
    {
        //if (uart_is_readable_within_us(i2c_, timeout_msec * 1000))
        //{
        //    //return uart_getc(uart_);
        //    uint8_t c = uart_getc(i2c_);
        //    //printf("read_byte: %#x \n", c);
        //    return c;
        //}
        wait_for_ready_byte();
        return -1;
    }

    virtual void write_bytes(const uint8_t* data, int len) override
    {
        //
        //uart_write_blocking(i2c_, data, len);
        //uart_tx_wait_blocking(i2c_);
        int ret = i2c_write_blocking(
            i2c_, PN532_I2C_ADDRESS, 
            data, len, 
            false // no STOP is true
        );
        if (ret != len)
        {
            printf("i2c write wrote only %i bytes, expected %i\n",
                ret, len
            );
        }
    }

private:
    i2c_inst_t *i2c_;
    int scl_;
    int sda_;

    void wait_for_ready_byte()
    {
        while (true)
        {
            uint8_t ready_byte;
            int ret = i2c_read_blocking(
                i2c_, PN532_I2C_ADDRESS, 
                &ready_byte, 1, 
                true // no STOP is true
            );
            if (ret != sizeof(ready_byte))
            {
                printf("Error reading i2c\n");
                sleep_ms(10);
            }
            else
            {
                printf("Got ready byte %i\n", ready_byte);
                return;
            }
        }
    }
};