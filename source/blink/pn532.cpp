#include "pn532.h"

#include <deque>
//#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

//#include <pico/malloc.h>
#include <pico/stdlib.h>

#include "pn532_backend_i2c.hpp"
#include "pn532_backend_uart.hpp"


#define GET_FW_VERSION      0x02
#define GET_FW_ANSWER_LEN   5
#define SAM_CONFIG              0x14
#define SAM_CONFIG_ANWSER_LEN   1
#define IN_LIST_PASSIVE_TARGET  0x4a

// Using read/write register to first poll values, then set
//#define RF_CONFIG 0x32
//#define RF_CONFIG_ITEM_ANALOG_106_TYPE_A 0x0a

#define READ_REGISTER   0x06
#define WRITE_REGISTER  0x08
#define CIU_RFCfg       0x6316
#define CIU_GsNOn0x     0x6317
#define CIU_CWGsP       0x6318

#define READ_TIMEOUT_MSEC 20
#define WRITE_PREAMBLE_LEN 10//20


pn532_t::pn532_t(int dev_num, int p1, int p2, backend be)
{
    if (be == pn532_t::uart)
    {
        backend_ = std::make_shared<pn532_backend_uart_t>();
    }
    else if (be == pn532_t::i2c)
    {
        backend_ = std::make_shared<pn532_backend_i2c_t>();
    }
    else
    {
        printf("pn532, wrong backend type: %i", be);
        exit(51);
    }
    backend_->init(dev_num, p1, p2);
    
    //wakeup();
    //sleep_ms(10);

    // SAM config to normal mode
    const uint8_t samcfg[] = {SAM_CONFIG, 0x01, 0x14 /* 1sec */, 0x00};
    write_frame(samcfg, sizeof(samcfg), WRITE_PREAMBLE_LEN);
    //sleep_ms(10);

    auto frame = read_frame();
    //printf("got frame: ");
    //hexdump(frame);
    //printf("\n");
    if (pn532_t::is_ack(frame))
    {
        printf("SAM Config ACK\n");
    }
    else
    {
        if (frame.size() == 0)
        {
            printf("SAM Config answer time out\n");
        }
        else
        {
            printf("SAM Config NACK.....\n");
        }
    }

    // read answer expects:
    // SAM_CONFIG +1 = 0x15
    auto data = read_frame();
    if (data.size() == 0)
    {
        printf("SAM config response timed out\n");
    }
    else
    {
        //hexdump(data);
        //printf("\n");
        if (data[0] != SAM_CONFIG + 1)
        {
            printf("SAM config failed... got %#x\n", data[0]);
        }
        else
        {
            printf("PN532 initialized\n");
        }
    }

#if 0
    // Disabled coz no increased range
    // Increase RF RX gain to max
    uint8_t cfg_reg = read_reg(CIU_RFCfg);
    // Se gain to Max, 33db --> 48db
    //
    //write_reg(CIU_RFCfg, 0x78);
    cfg_reg = read_reg(CIU_RFCfg);
    printf("----------\n");
#endif
#if 0
    // Disabled coz no increased range
    // Increase N-driver conductance
    cfg_reg = read_reg(CIU_GsNOn0x);
    write_reg(CIU_GsNOn0x, 0xf8);
    cfg_reg = read_reg(CIU_GsNOn0x);
    printf("----------\n");

    // Increase P-driver conductance
    cfg_reg = read_reg(CIU_CWGsP);
    write_reg(CIU_CWGsP, 0x3f);
    cfg_reg = read_reg(CIU_CWGsP);
    printf("----------\n");
#endif
}

uint32_t pn532_t::version()
{
    uint8_t cmd = GET_FW_VERSION;
    write_frame(&cmd, 1, WRITE_PREAMBLE_LEN);
    // read ACK
    auto frame = read_frame();
    if (pn532_t::is_ack(frame))
    {
        printf("Version ACK\n");
    }
    else
    {
        if (frame.size() == 0)
        {
            printf("Version answer time out\n");
        }
        else
        {
            printf("Version NACK.....\n");
        }
    }

    // read answer expects:
    // GET_FW_VERSION +1 | IC(0x32) | Ver | Rev | flags
    auto data = read_frame();
    if (data.size() != 5)
    {
        printf("version answer expected to be 5 bytes but got %i\n", data.size());
        return 0;
    }
    if (data[0] != GET_FW_VERSION + 1)
    {
        printf("bad TFI\n");
        return 0;
    }
    return data[1] << 24 | data[2] << 16 | data[3] << 8 | data[4];
}

void pn532_t::loop_for_tag()
{
    unsigned int cnt = 0;
    // TODO allow to return result

    // 0x00  // BrTy baud 0:106 kbps type A (ISO/IEC14443 Type A)
    // 0x03  // BrTy baud 0:106 kbps type type B (ISO/IEC14443-3B)
    const uint8_t get_tag_cmd[] = {
        IN_LIST_PASSIVE_TARGET, // Init etc
        0x01, // MaxTg [1; 2]
        0x00  // BrTY see above
        //, 0x1f, 0x49, 0x5e, 0x1e //< To detect a known tag
    };
    write_frame(get_tag_cmd, sizeof(get_tag_cmd), 1);
    while (true)
    {
        auto frame = read_frame();
        if (pn532_t::is_nack(frame))
        {
            printf("%%%% got NACK !!!!\n");
            sleep_ms(1000);
        }
        else if (pn532_t::is_ack(frame))
        {
            //printf("%%%% got ACK !!!!\n");
        }
        else if (frame.size() == 0)
        {
            // nothing
            //printf(".");
        }
        // starting from here, frame is considered well formed
        else if (frame[0] == IN_LIST_PASSIVE_TARGET + 1)
        {
            // re-arm
            write_frame(get_tag_cmd, sizeof(get_tag_cmd), 1);
            std::vector<uint8_t> id = std::vector<uint8_t>(
                frame.begin() + frame.size() - 4,
                frame.end()
            );
            printf("%3i got ID: ", cnt);
            hexdump(id);
            printf("\n");
            cnt++;
        }
    }
}


void pn532_t::write_frame(const uint8_t* data, int len, int preamble_len)
{
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

#if 0
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
#endif
    backend_->write_bytes(frame, i);
    //uart_write_blocking(uart_, frame, i);
    //uart_tx_wait_blocking(uart_);
    free(frame);
}

std::vector<uint8_t> pn532_t::read_frame()
{
    std::deque<uint8_t> frame;
    
    // read until getting expected start sequence 
    int16_t c = backend_->read_byte(READ_TIMEOUT_MSEC);
    while (c >= 0)
    //while (uart_is_readable_within_us(uart_, READ_TIMEOUT_MSEC))
    {
        frame.push_back(c);
        if (frame.size() == 2)
        {
            if (frame[0] == 0x00 && frame[1] == 0xff)
            {
                break;
            }
            // pass until we get start code 0x00 0xff
            frame.pop_front();
        }
        c = backend_->read_byte(READ_TIMEOUT_MSEC);
    }
    if (frame.size() != 2)
    {
        if (frame[0] != 0 || frame[1] != 0xff)
        {
            return std::vector<uint8_t>();
        }
    }
    frame.clear();

    // normal frame follows with:
    //  LEN + LCS + {LEN bytes} + DCS + POST = 4 + {LEN} bytes
    // ACL/NACK frame follows with: 
    //   0x00 0xff (ACK)  | 0x00 (postamble)
    //   0xff 0x00 (NACK) | 0x00 (postamble)
    // ACK/NACK postamble is ignored. It will be consumed as next frame preamble
    int bytes_to_read = 4;

    c = backend_->read_byte(READ_TIMEOUT_MSEC);
    while (c >= 0)
    {
        //printf("read process %#x \n", c);
        frame.push_back(c);
        //printf("read post: %#x\n", frame.back());
        if (frame.size() == 2)
        {
            // 2 first bytes are either LEN + LCS or ACK/NACK
            if (
                frame[0] == 0x00 && frame[1] == 0xff ||
                frame[0] == 0xff && frame[1] == 0x00
            )
            {
                // ACK/NACK
                printf("-- ACK/NACK\n");
                return std::vector<uint8_t>(frame.begin(), frame.end());
            }
            // got LEN and its checksum
            if ((uint8_t)(frame[0] + frame[1]) != 0)
            {
                printf("bad LEN %#x %#x \n", frame[0], frame[1]);
                return std::vector<uint8_t>();
            }
            // length
            bytes_to_read += frame[0];
            //printf("Got LEN %i \n", bytes_to_read);
        }
        if (frame.size() == bytes_to_read)
        {
            break;
        }
        c = backend_->read_byte(READ_TIMEOUT_MSEC);
    }

    if (frame.size() != bytes_to_read)
    {
        return std::vector<uint8_t>();
    }
    uint8_t checksum = 0;
    for (int k=0; k<frame[0]; k++)
    {
        checksum += frame[k+2];
    }
    checksum = 0x100 - checksum;
    if (checksum != frame[frame[0]+2])
    {
        printf("received bad checksum expect %#x got %#x \n", checksum, frame[frame[0]+2]);
        return std::vector<uint8_t>();
    }
    if (frame[bytes_to_read - 1 ] != 0)
    {
        printf("received bad postamble expect 0x00 got %#x \n", frame[bytes_to_read - 1]);
        return std::vector<uint8_t>();
    }

    // strip LEN, LCS, TFI
    frame.pop_front();
    frame.pop_front();
    frame.pop_front();
    // strip DCS, postamble
    frame.pop_back();
    frame.pop_back();
    return std::vector<uint8_t>(frame.begin(), frame.end());
}

bool pn532_t::is_ack(std::vector<uint8_t> frame)
{
    if (frame.size() < 2)
    {
        return false;
    }
    return frame[0] == 0x00 && frame[1] == 0xff;
}

bool pn532_t::is_nack(std::vector<uint8_t> frame)
{
    if (frame.size() < 2)
    {
        return false;
    }
    return frame[0] == 0xff && frame[1] == 0x00;
}

uint8_t pn532_t::read_reg(uint16_t reg)
{
    const uint8_t read_reg_cmd[] = {
        READ_REGISTER, 
        (uint8_t)(reg >> 8), 
        (uint8_t)(reg)
    };
    write_frame(read_reg_cmd, sizeof(read_reg_cmd), WRITE_PREAMBLE_LEN);
    // read ACK
    auto frame = read_frame();
    if (frame.size() == 0)
    {
        printf("Read reg ACK time out\n");
    }
    else if (pn532_t::is_ack(frame) != true)
    {
        printf("Read reg Not an ACK.....\n");
    }

    // read response TODO
    frame = read_frame();
    printf("got Reg: ");
    hexdump(frame);
    printf("\n");

    return frame[1];
}

void pn532_t::write_reg(uint16_t reg, uint8_t value)
{
    const uint8_t write_reg_cmd[] = {
        WRITE_REGISTER, 
        (uint8_t)(reg >> 8), 
        (uint8_t)(reg),
        value
    };
    write_frame(write_reg_cmd, sizeof(write_reg_cmd), WRITE_PREAMBLE_LEN);
    // read ACK
    auto frame = read_frame();
    if (frame.size() == 0)
    {
        printf("Write reg ACK time out\n");
    }
    else if (pn532_t::is_ack(frame) != true)
    {
        printf("Write reg Not an ACK.....\n");
    }

    // read response TODO
    frame = read_frame();
    printf("got Reg: ");
    hexdump(frame);
    printf("\n");
}
