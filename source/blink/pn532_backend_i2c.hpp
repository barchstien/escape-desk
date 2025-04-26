#pragma once

#include "pn532_backend.h"

#include <hardware/i2c.h>
#include <pico/stdlib.h>

#include <deque>
#include <stdio.h>

#define PN532_I2C_ADDRESS 0x24

struct pn532_backend_i2c_t : public pn532_backend_t
{
    static constexpr unsigned int BAUD_RATE = 100000;

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

        sleep_ms(100);

        // debug
        //uint8_t data;
        //printf("test....\n");
        //int ret = i2c_read_blocking(i2c_, PN532_I2C_ADDRESS, &data, 1, false);
        //printf(" - test read: %i\n", ret);
        //ret = i2c_read_blocking(i2c_, PN532_I2C_ADDRESS, &data, 1, false);
        //printf(" + test read: %i\n", ret);

        //bool enabled = uart_is_enabled(i2c_);
        //printf("UART %i is enabled: %u\n", i2c_num, enabled);

        //wakeup();
    }

    virtual int16_t read_byte(unsigned int timeout_msec) override
    {
        if (buffer_.size() == 0)
        {
            // TODO non-blocking !
            //wait_for_ready_byte();
            uint8_t ready_byte[30];
            int ret = i2c_read_blocking(
                i2c_, PN532_I2C_ADDRESS, 
                ready_byte, 30, 
                false // no STOP is true
            );
            if (ret < 0)
            {
                return -1;
            }
            if (ready_byte[0] != 1)
            {
                return -1;
            }
            // pop ready byte
            //buffer_.pop_front();
            for (int i=1; i<30; i++)
            {
                buffer_.push_back(ready_byte[i]);
            }

            // debug
            printf("i2c read: ");
            for (int i=0; i<ret; i++)
            {
                printf("%#x ", ready_byte[i]);
            }
            printf("\n");
        }

        //return ready_byte[0];
        auto b = buffer_.front();
        buffer_.pop_front();
        return b;
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
        else
        {
            printf("i2c wrote %i bytes, wait for ready byte...\n", ret);
            sleep_ms(10);
            wait_for_ready_byte();
        }
    }

private:
    i2c_inst_t *i2c_;
    int scl_;
    int sda_;

    std::deque<uint8_t> buffer_;

    void wait_for_ready_byte()
    {
        while (true)
        {
            uint8_t ready_byte;
            int ret = i2c_read_blocking(
                i2c_, PN532_I2C_ADDRESS, 
                &ready_byte, 1, 
                false // no STOP if true
            );
            if (ret != sizeof(ready_byte))
            {
                printf("Error reading i2c\n");
                sleep_ms(100);
            }
            else
            {
                printf("Got ready byte %i\n", ready_byte);
                return;
            }
        }
    }
};