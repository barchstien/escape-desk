#pragma once

#include "pn532_backend.h"

#include <hardware/i2c.h>
#include <pico/stdlib.h>

#include <deque>
#include <stdio.h>

#define PN532_I2C_ADDRESS 0x24
#define I2C_READ_LEN 20

struct pn532_backend_i2c_t : public pn532_backend_t
{
    static constexpr unsigned int BAUD_RATE = 4e5;

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
    }

    virtual int16_t read_byte(unsigned int timeout_msec) override
    {
        if (buffer_.size() == 0)
        {
            if (false == wait_for_ready_byte(timeout_msec))
            {
                return -1;
            }
            uint8_t read_buff[I2C_READ_LEN];
            memset(read_buff, 0, I2C_READ_LEN);
            int ret = i2c_read_timeout_us(
                i2c_, PN532_I2C_ADDRESS, 
                read_buff, I2C_READ_LEN, 
                false, // no STOP is true
                timeout_msec * 1e3
            );
            if (ret < 0)
            {
                return -1;
            }
            if (read_buff[0] != 1)
            {
                return -1;
            }
            // skip ready byte
            for (int i=1; i<I2C_READ_LEN; i++)
            {
                buffer_.push_back(read_buff[i]);
            }

            // debug
            //printf("i2c read: ");
            //for (int i=0; i<ret; i++)
            //{
            //    printf("%#x ", read_buff[i]);
            //}
            //printf("\n");
        }
        auto b = buffer_.front();
        buffer_.pop_front();
        return b;
    }

    virtual void write_bytes(const uint8_t* data, int len) override
    {
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
            printf("i2c wrote %i bytes \n", ret);
            
            // debug
            //printf("i2c wrote %i bytes: ", ret);
            //for (int i=0; i<len; i++)
            //{
            //    printf("%#x ", data[i]);
            //}
            //printf("\n");
        }
    }

private:
    i2c_inst_t *i2c_;
    int scl_;
    int sda_;

    std::deque<uint8_t> buffer_;

    /**
     * @return true if got ready byte
     * @note works fine without... probably being lucky on timing so keep using !
     */
    bool wait_for_ready_byte(unsigned int timeout_msec)
    {
        uint8_t ready_byte;
        absolute_time_t start_time = get_absolute_time();
        absolute_time_t now = get_absolute_time();
        while (absolute_time_diff_us(now, start_time) < timeout_msec * 1e3)
        {
            int ret = i2c_read_timeout_us(
                i2c_, PN532_I2C_ADDRESS, 
                &ready_byte, 1, 
                false, // no STOP if true
                timeout_msec * 1e3
            );
            if (ret != sizeof(ready_byte))
            {
                printf("Error reading i2c\n");
                sleep_us(100);
                //return false;
            }
            else if (ready_byte != 1)
            {
                sleep_us(100);
                //return false;
            }
            else
            {
                printf("Got ready byte %i\n", ready_byte);
                return true;
            }
            now = get_absolute_time();
        }
        return false;
    }
};