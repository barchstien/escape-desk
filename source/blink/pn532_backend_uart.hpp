#pragma once

#include "pn532_backend.h"

#include <hardware/uart.h>
#include <pico/stdlib.h>

#include <stdio.h>

#define BAUD_RATE 115200

struct pn532_backend_uart_t : public pn532_backend_t
{
    virtual void init(int uart_num, int rx_pin, int tx_pin) override
    {
        uart_ = uart_get_instance(uart_num);
        rx_pin_ = rx_pin;
        tx_pin_ = tx_pin;
        int b = uart_init(uart_, BAUD_RATE);
        printf("UART %i set baud to %i \n", uart_num, b);
        uart_set_translate_crlf(uart_, false);
        //gpio_set_function(PN532_UART_TX_PIN, GPIO_FUNC_UART);
        gpio_set_function(tx_pin_, UART_FUNCSEL_NUM(uart_, tx_pin_));
        //gpio_set_function(PN532_UART_RX_PIN, GPIO_FUNC_UART);
        gpio_set_function(rx_pin_, UART_FUNCSEL_NUM(uart_, rx_pin_));

        uart_set_hw_flow(uart_, false, false);
        uart_set_format(uart_, 8, 1, UART_PARITY_NONE);
        //uart_set_fifo_enabled(uart_, false);
        sleep_ms(1);

        bool enabled = uart_is_enabled(uart_);
        printf("UART %i is enabled: %u\n", uart_num, enabled);

        wakeup();
    }

    virtual int16_t read_byte(unsigned int timeout_msec) override
    {
        if (uart_is_readable_within_us(uart_, timeout_msec * 1000))
        {
            //return uart_getc(uart_);
            uint8_t c = uart_getc(uart_);
            //printf("read_byte: %#x \n", c);
            return c;
        }
        return -1;
    }

    virtual void write_bytes(const uint8_t* data, int len) override
    {
        //
        uart_write_blocking(uart_, data, len);
        uart_tx_wait_blocking(uart_);
        //sleep_ms(10);
    }

private:
    uart_inst_t *uart_;
    int rx_pin_;
    int tx_pin_;

    void wakeup()
    {
        const uint8_t wakeup_char = 0x55;
        const int wakeup_len = 100;
        uint8_t* wakeup_frame = (uint8_t*)malloc(wakeup_len);
        memset(wakeup_frame, wakeup_len, wakeup_char);
        write_bytes(wakeup_frame, wakeup_len);
        free(wakeup_frame);
    }
};