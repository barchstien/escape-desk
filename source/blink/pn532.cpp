#include "pn532.h"

//#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "pico/malloc.h"
#include "pico/stdlib.h"


#define GET_FW_VERSION 0x02
#define GET_FW_ANSWER_LEN 5
#define SAM_CONFIG 0x14
#define SAM_CONFIG_ANWSER_LEN 1

#define READ_TIMEOUT_USEC 20000
#define WRITE_PREAMBLE_LEN 10//20

#define BAUD_RATE 115200

#if 0
 

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
#endif

pn532_t::pn532_t(uart_inst_t* u, int rx_pin, int tx_pin)
    : uart_(u), rx_pin_(rx_pin), tx_pin_(tx_pin)
{
    int b = uart_init(uart_, BAUD_RATE);
    printf("UART set baud to %i \n", b);
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
    printf("uart_is_enabled: %u\n", enabled);

#if 0
    //for (int i=0; i<5; i++)
    if (false)
    {
        // wakeup sequence
        const uint8_t wakeup[] = {0x55, 0x55, 0, 0, 0};//, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};
        printf("sizeof wakeup %i\n", sizeof(wakeup));
        //uart_write_blocking(uart_, wakeup, sizeof(wakeup));
        for (int i=0; i<sizeof(wakeup); i++)
        {
            uart_putc_raw(uart_, wakeup[i]);
            uart_tx_wait_blocking(uart_);
        }
        //uart_tx_wait_blocking(uart_);
        printf("wakeup sent !\n");
        sleep_ms(5);
    }
#endif
    const uint8_t wakeup_char = 0x55;
    const int wakeup_len = 100;
    uint8_t* wakeup_frame = (uint8_t*)malloc(wakeup_len);
    memset(wakeup_frame, wakeup_len, wakeup_char);
    uart_write_blocking(uart_, wakeup_frame, sizeof(wakeup_frame));
    //sleep_ms(3);
#if 0
    while (uart_is_readable_within_us(uart_, 1000))
    {
        // drop
        printf("drain %#x\n", uart_getc(uart_));
        //sleep_ms(10);
    }
#endif
    // SAM config to normal mode
    const uint8_t samcfg[] = {SAM_CONFIG, 0x01, 0x14 /* 1sec */, 0x00};
    write(samcfg, sizeof(samcfg), 10);//WRITE_PREAMBLE_LEN);
    //sleep_ms(10);
    //printf("SAM config sent !\n");

    // debug
    while (true)
    {
        if (uart_is_readable_within_us(uart_, READ_TIMEOUT_USEC))
        {
            printf("read %#x\n", uart_getc(uart_));
        }
    }
#if 0
    // read ACK
    bool timed_out;
    // TODO re-enable
    auto data = read(timed_out, true);
    // read answer expects:
    // SAM_CONFIG +1 = 0x15
    data = read(timed_out);
    if (data.size() == 0)
    {
        printf("SAM config response timed out\n");
    }
    else
    {
        hexdump(data);
        printf("\n");
        if (data[0] != SAM_CONFIG + 1)
        {
            printf("SAM config failed... got %#x\n", data[0]);
        }
        else
        {
            printf("PN532 initialized over UART.\n");
        }
    }
#endif
}

uint32_t pn532_t::version()
{
    uint8_t cmd = GET_FW_VERSION;
    write(&cmd, 1, WRITE_PREAMBLE_LEN);
    // read ACK
    bool timed_out;
    auto data = read(timed_out, true);
    if (timed_out)
    {
        printf("ACK/NACK timed out\n");
        return 0xffffffff;
    }
    if (data[0] != 0x00 || data[1] != 0xff)
    {
        printf("got NACK %#x %#x\n", data[0], data[1]);
    }
    // read answer expects:
    // D5 | GET_FW_VERSION +1 | IC(0x32) | Ver | Rev | flags
    data = read(timed_out);
    if (data[0] != GET_FW_VERSION + 1)
    {
        printf("bad TFI\n");
        return 0;
    }
    if (data.size() != 5)
    {
        printf("bad size expects 5 got %i\n", data.size());
        return 0;
    }
    return data[1] << 24 + data[2] << 16 + data[3] << 8 + data[4];
}

void pn532_t::write(const uint8_t* data, int len, int preamble_len)
{
    //
    //preamble_len = 1;
    uint8_t* frame = (uint8_t*)malloc(len + 7 + preamble_len);
    int i = 0;
    for (int k=0; k<preamble_len; k++)
    {
        frame[i++] = 0x00;
    }
    // 00 FF technically part of preamble, ie minimum preamble
    frame[i++] = 0x00;
    frame[i++] = 0xFF;
    frame[i++] = len + 1;
    frame[i++] = 0x100 - (uint8_t)(len + 1);
    frame[i++] = 0xd4;
    unsigned int checksum = 0xd4;
    for (int k=0; k<len; k++)
    {
        frame[i++] = data[k];
        checksum += data[k];
    }
    frame[i++] = 0x100 - (uint8_t)checksum;
    frame[i++] = 0x00;

    printf("writting: \n");
    for (int k=0; k<i; k++)
    {
        if (k % 10 == 0)
        {
            printf("\n%3d : ", k);
        }
        printf("%2x " , frame[k]);
    }
    printf("\nlen: %i\n", i);

    uart_write_blocking(uart_, frame, i);
    //uart_tx_wait_blocking(uart_);

    //for (int p=0; p<i; p++)
    //{
    //    uart_putc_raw(uart_, frame[p]);
    //    uart_tx_wait_blocking(uart_);
    //    sleep_us(10);
    //}

    printf("Wrote %i bytes\n", i);
}

std::deque<uint8_t> pn532_t::read(bool& timed_out, bool ack_nack)
{
    std::deque<uint8_t> data;
    timed_out = false;;
    
    // read until getting expected sequence 
    // 1. preamble + start = 3 bytes
    while (uart_is_readable_within_us(uart_, READ_TIMEOUT_USEC))
    {
        data.push_back(uart_getc(uart_));
        printf("read pre: %#x\n", data.back());
        if (data.size() >= 3)
        {
            if (data[0] != 0 || data[1] != 0 || data[2] != 0xff)
            {
                printf("bad frame start %#x %#x %#x \n", data[0], data[1], data[2]);
                return std::deque<uint8_t>();
            }
            break;
        }
    }
    if (data.size() != 3)
    {
        timed_out = true;
        return std::deque<uint8_t>();
    }
    data.clear();
    printf("Got head ---- %i\n", data.size());

    // 2. data + postamble = 3 bytes
    // OR LEN + LCS + {LEN bytes} + DCS + POST = 4 + {LEN} bytes
    int bytes_to_read = 0;
    if (ack_nack)
    {
        bytes_to_read = 3;
        printf("expects ack/nack\n");
    }
    else
    {
        // will be incremented after
        bytes_to_read = 4;
    }
    while (uart_is_readable_within_us(uart_, READ_TIMEOUT_USEC))
    {
        data.push_back(uart_getc(uart_));
        printf("read post: %#x\n", data.back());
        if (ack_nack == false && data.size() == 2)
        {
            // got LEC and its checksum
            if ((uint8_t)(data[0] + data[1]) != 0)
            {
                printf("bad LEN %#x %#x \n", data[0], data[1]);
                return std::deque<uint8_t>();
            }
            // length
            bytes_to_read += data[0];
            printf("Got LEN %i \n", bytes_to_read);
        }
        if (data.size() == bytes_to_read)
        {
            break;
        }
    }

    if (data.size() != bytes_to_read)
    {
        timed_out = true;
        return std::deque<uint8_t>();
    }
    uint8_t checksum = 0;
    for (int k=0; k<data[0]; k++)
    {
        checksum += data[k+2];
    }
    checksum = 0x100 - checksum;
    if (checksum != data[data[0]+2])
    {
        printf("received bad checksum expect %#x got %#x \n", checksum, data[data[0]+2]);
        return std::deque<uint8_t>();
    }
    if (data[bytes_to_read - 1 ] != 0)
    {
        printf("received bad postamble expect 0x00 got %#x \n", data[bytes_to_read - 1]);
        return std::deque<uint8_t>();
    }
    printf("Got tail data.size(): %i \n", data.size());

    // strip LEN, LCS, TFI
    data.pop_front();
    data.pop_front();
    data.pop_front();
    // strip DCS, postamble
    data.pop_back();
    data.pop_back();
    return data;
}