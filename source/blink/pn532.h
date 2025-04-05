#ifndef PN532_H
#define PN532_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/uart.h"

#define PN532_MIFARE_ISO14443A 0x00

#define PN532_UART_ID uart1  // Use UART1
#define PN532_UART_TX_PIN 4   // TX pin (GPIO 4)
#define PN532_UART_RX_PIN 5   // RX pin (GPIO 5)

typedef struct {
    uart_inst_t *uart;
} PN532;

void pn532_uart_init();
uint32_t pn532_getFirmwareVersion(PN532 *dev);
void pn532_SAMConfig(PN532 *dev);
bool pn532_readPassiveTargetID(PN532 *dev, uint8_t cardbaudrate, uint8_t *uid, uint8_t *uidLength);

#endif // PN532_H
