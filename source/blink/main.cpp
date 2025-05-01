#include <stdio.h>
#include <pico/stdlib.h>

#include "pn532.h"

#define PN532_UART_1_TX_PIN 4
#define PN532_UART_1_RX_PIN 5
#define PN532_UART_1_TARGET_TAG 0x251e86ef

#define PN532_UART_0_TX_PIN 16
#define PN532_UART_0_RX_PIN 17
#define PN532_UART_0_TARGET_TAG 0x1e5e491f

#define PN532_I2C_1_SCL_PIN 27
#define PN532_I2C_1_SDA_PIN 26
#define PN532_I2C_1_TARGET_TAG 0x25ca77ef

#define LED_PIN_LOCK_0 0
#define LED_PIN_LOCK_1 1
#define LED_PIN_LOCK_2 2

int main() 
{
    stdio_init_all();
    //stdio_usb_init();
    // Wait a little before starting
    sleep_ms(5000);
    stdio_filter_driver(&stdio_usb);

    std::vector<pn532_t> nfc_read_list;

    nfc_read_list.emplace_back(
        0, // uart 0
        PN532_UART_0_RX_PIN, 
        PN532_UART_0_TX_PIN, 
        pn532_t::uart, 
        PN532_UART_0_TARGET_TAG
    );
    printf("--> %s fw:%#x\n-----------------\n", 
        nfc_read_list.back().name().c_str(), 
        nfc_read_list.back().version());
    
    nfc_read_list.emplace_back(
        1, // uart 1
        PN532_UART_1_RX_PIN, 
        PN532_UART_1_TX_PIN, 
        pn532_t::uart, 
        PN532_UART_1_TARGET_TAG
    );
    printf("--> %s fw:%#x\n-----------------\n", 
        nfc_read_list.back().name().c_str(), 
        nfc_read_list.back().version());
#if 1
    nfc_read_list.emplace_back(
        1, // spi 1
        PN532_I2C_1_SCL_PIN, 
        PN532_I2C_1_SDA_PIN, 
        pn532_t::i2c, 
        PN532_I2C_1_TARGET_TAG
    );
    printf("--> %s fw:%#x\n-----------------\n", 
        nfc_read_list.back().name().c_str(), 
        nfc_read_list.back().version());
#endif
    
    int LED_PIN_LOCK[] = {
        LED_PIN_LOCK_0, LED_PIN_LOCK_1, LED_PIN_LOCK_2
    };
    for (int i=0; i<3; i++)
    {
        gpio_init(LED_PIN_LOCK[i]);
        gpio_set_dir(LED_PIN_LOCK[i], GPIO_OUT);
        gpio_put(LED_PIN_LOCK[i], 1);
        sleep_ms(1000);
        gpio_put(LED_PIN_LOCK[i], 0);
    }

    sleep_ms(1000);

    //pn532_2.loop_for_tag();
    printf("main rewind: \n");
    for (auto& nfc_r : nfc_read_list)
    {
        printf("-- \n");
        nfc_r.rewind();
    }

    bool nfc_lock[] = {false, false, false};

    while (true)
    {
        int cnt = 0;
        for (auto& nfc_r : nfc_read_list)
        {
            nfc_lock[cnt] = nfc_r.has_target_tag();
            if (nfc_lock[cnt])
            {
                gpio_put(LED_PIN_LOCK[cnt], 1);
            }
            else
            {
                gpio_put(LED_PIN_LOCK[cnt], 0);
            }
            cnt++;
        }

        sleep_ms(100);
    }

    return 0;
}

