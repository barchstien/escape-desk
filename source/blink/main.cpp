#include <stdio.h>
#include <pico/stdlib.h>

#include "pn532.h"

#define PN532_UART_1_TX_PIN 4
#define PN532_UART_1_RX_PIN 5

#define PN532_UART_0_TX_PIN 16
#define PN532_UART_0_RX_PIN 17

#define PN532_I2C_1_SCL_PIN 27
#define PN532_I2C_1_SDA_PIN 26


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
        pn532_t::uart
    );
    printf("--> %s fw:%#x\n-----------------\n", 
        nfc_read_list.back().name().c_str(), 
        nfc_read_list.back().version());
    
    nfc_read_list.emplace_back(
        1, // uart 1
        PN532_UART_1_RX_PIN, 
        PN532_UART_1_TX_PIN, 
        pn532_t::uart
    );
    printf("--> %s fw:%#x\n-----------------\n", 
        nfc_read_list.back().name().c_str(), 
        nfc_read_list.back().version());
#if 1
    nfc_read_list.emplace_back(
        1, // spi 1
        PN532_I2C_1_SCL_PIN, 
        PN532_I2C_1_SDA_PIN, 
        pn532_t::i2c
    );
    printf("--> %s fw:%#x\n-----------------\n", 
        nfc_read_list.back().name().c_str(), 
        nfc_read_list.back().version());
#endif
    sleep_ms(1000);

    //pn532_2.loop_for_tag();
    printf("main rewind: \n");
    for (auto& nfc_r : nfc_read_list)
    {
        printf("-- \n");
        nfc_r.rewind();
    }

    while (true)
    {
        for (auto& nfc_r : nfc_read_list)
        {
            //printf("-++- get_tag() %s \n", nfc_r.name().c_str());
            int id = nfc_r.get_tag();
            if (id != 0)
            {
                nfc_r.rewind();
                printf("-++- %s got ID: %u \n",
                    nfc_r.name().c_str(), id);
            }
            //sleep_ms(3000);
        }
    }

    return 0;
}

