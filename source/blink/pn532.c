#include "pn532.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

void pn532_uart_init(PN532 *dev) {
    uart_init(PN532_UART_ID, 115200);
#if 0
    gpio_set_function(PN532_UART_TX_PIN, UART_FUNCSEL_NUM(uart1, PN532_UART_TX_PIN));
    gpio_set_function(PN532_UART_RX_PIN, UART_FUNCSEL_NUM(uart1, PN532_UART_RX_PIN));
#else
    gpio_set_function(PN532_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(PN532_UART_RX_PIN, GPIO_FUNC_UART);
#endif
    uart_set_hw_flow(PN532_UART_ID, false, false);
    uart_set_format(PN532_UART_ID, 8, 1, UART_PARITY_NONE);
    sleep_ms(100);

    bool enabled = uart_is_enabled(PN532_UART_ID);
    printf("uart_is_enabled: %u\n", enabled);
    
    dev->uart = PN532_UART_ID;
    printf("PN532 initialized over UART.\n");
}

uint32_t pn532_getFirmwareVersion(PN532 *dev) {
    //uint8_t command[] = {0x02};
    //uint8_t response[4];
    uint8_t command[] = {0x00, 0x00, 0xFF, 0x02, 0xFE, 0xD4, 0x02, 0x2A, 0x00};
    uint8_t response[12];

    uart_write_blocking(dev->uart, command, sizeof(command));
    uart_tx_wait_blocking(dev->uart);
    printf("Wrote uart cmd...\n");
    sleep_ms(10);
    //uart_read_blocking(dev->uart, response, sizeof(response));  // No return value, just read the buffer
    int cnt = 0;
    while(
        uart_is_readable_within_us(PN532_UART_ID, 10000) 
        && cnt < sizeof(response)) 
    {
        response[cnt++] = uart_getc(PN532_UART_ID);
    }
    
    return (response[0] << 24) | (response[1] << 16) | (response[2] << 8) | response[3];
}

void pn532_SAMConfig(PN532 *dev) {
    uint8_t command[] = {0x14, 0x01, 0x00, 0x01};
    // Combined sequence to send according to gemini 2.5:
    // 0x00, 0x00, 0xFF, 0x03, 0xFD, 0xD4, 0x14, 0x01, 0x17, 0x00
    uart_write_blocking(dev->uart, command, sizeof(command));
    sleep_ms(10);
}

bool pn532_readPassiveTargetID(PN532 *dev, uint8_t cardbaudrate, uint8_t *uid, uint8_t *uidLength) {
    uint8_t command[] = {0x4A, 0x01, cardbaudrate};
    uint8_t response[8];

    uart_write_blocking(dev->uart, command, sizeof(command));
    sleep_ms(50);
    uart_read_blocking(dev->uart, response, 8);  // Read response buffer
    
    if (response[0] != 1) return false;

    *uidLength = response[1];
    memcpy(uid, &response[2], *uidLength);
    return true;
}
