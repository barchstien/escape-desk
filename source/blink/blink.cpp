#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

#include "pn532.h"

#define PN532_UART_ID 1//uart1
//#define PN532_UART_ID uart1  // Use UART1
#define PN532_UART_TX_PIN 4   // TX pin (GPIO 4)
#define PN532_UART_RX_PIN 5   // RX pin (GPIO 5)

//PN532 pn532;

int main() 
{
    stdio_init_all();
    // Wait a little before starting
    sleep_ms(5000);
    
    //uart_init(UART_ID, BAUD_RATE);
    pn532_t pn532(PN532_UART_ID, PN532_UART_RX_PIN, PN532_UART_TX_PIN);
    //pn532_t pn532(uart1, 17, 16); //< Doesen't work, collides with USB / SERIAL ?
    printf("FW %#x\n", pn532.version());

    //printf("Wait...\n");
    sleep_ms(1000);
    //printf("FW %#x\n", pn532.version());

    pn532.loop_for_tag();
    
#if 0
    printf("Getting FW version...\n");
    uint32_t versiondata = pn532_getFirmwareVersion(&pn532);
    if (!versiondata) {
        printf("Didn't find PN532 board\n");
        while (1){
            printf("no pn532 board\n");
            //look_for_tag();
            sleep_ms(1000);  // Delay before the next scan attempt
        }
    }
    printf("Found PN532 board! Firmware version: 0x%08X\n", versiondata);
    pn532_SAMConfig(&pn532);
#endif

    while (1) {
        //printf("Looking for NFC tags... DUMMY\n");
        //look_for_tag();
        sleep_ms(3000);  // Delay before the next scan attempt
    }

    return 0;
}


#if 0
#include "hardware/uart.h"
#include "pn532.h"


#define UART_ID uart1
#define TX_PIN 4
#define RX_PIN 5
#define BAUD_RATE 115200

// NFC Reader Commands (depending on your Grove NFC Reader)
// These can vary, so refer to your specific NFC Reader's datasheet for more details
#define NFC_CMD_REQUEST 0x0C  // Command for request (Look for tag)
#define NFC_CMD_HALT 0x50     // Command to halt communication with tag

// Send a command to the NFC reader and wait for a response
void send_command(const uint8_t *command, size_t len) {
    uart_write_blocking(UART_ID, command, len);
    sleep_ms(100);

    while (uart_is_readable(UART_ID)) {
        uint8_t response;
        uart_read_blocking(UART_ID, &response, 1);
        printf("Response: 0x%02X\n", response);
    }
}

// Send the request command to the NFC reader to look for a tag
void look_for_tag() {
    uint8_t request_command[] = {NFC_CMD_REQUEST, 0x00, 0x00, 0x00};  // Adjust depending on your NFC module protocol
    send_command(request_command, sizeof(request_command));
}

int main() {
    printf("1...\n");
    stdio_init_all();
    printf("2...\n");
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(RX_PIN, GPIO_FUNC_UART);

    // Wait a little before starting
    sleep_ms(1000);

    while (1) {
        printf("Looking for NFC tags...\n");
        //look_for_tag();
        sleep_ms(500);  // Delay before the next scan attempt
    }

    return 0;
}
#endif