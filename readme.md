 * use maglock ? Looks easy but akk include a spring to release on power loss. Could that lead to cup not being released ?
   Or use solenoid push/pull ? That requires a mechanical craddle to swing, or push the cup off an edge
   --> in any case, need a relay to switch 12V
   --> and also a 12V PSU

 * use a male 230V out ? Like a grass cutter ?
   Or put a PC-like 230 input ?




RF puzzle list of components :
 * PSU Convertisseur 12 W SKU24490 
   - 12V 800mA 
   - 5V 500mA
 * Maglock
   60Kg 12V 110mA safety lock
   80 x 40 mm
   WARNING use flyback diode
 * Relay
   - Grove 103020005 3.3V control, cut up to 30VDC 10A, 3.3 euro
     25Ohm at 3.3V = 132mA --> too much !
   - Module MOSFET Grove 103020008
     5 to 15V 2A 
 * RF id reader
   - Grove NFC 13,56 MHz 113020006
     i2c or uart, 85mA 3.3V
 * pico pi 2 150MHz 7.4 euro
   micro-usb 3.3V logic, ~15mA
   uart 1, i2c0/1
   provide 3.3V 300mA
 * carrier board
   - terminal screw : Shield à borniers Pico DFR0924 25 euro
     takes 7 to 24V DC input and power pico too
   - robo pico (buzzer + grove to i2c0 i2c1 and uart1, and gpio for relay ctrl)
     18 euro.  Platine Maker Pi Pico Base is 13 euro
     Grove ports give 3.3V 300mA total
   - shield grive pico (like robo pico without buzzer)
     5 euro
   - Module Maker Pi RP2040 14.5 euro
     Does include RPI chip soldered <----
     i2c0/1 uart1 buzzer


Ligh + switch puzzle list of compoenents:
 * LED strip
   each segment is 3 LEDs 2835, each at 0.2W, at 12V -> 17mA per LED
   Which means 50mA per segment
 * 12V PSU (same as for other puzzle?)
 * switch (12V switch)

------

```
#define PICO_I2C_INSTANCE   i2c1
#define PICO_I2C_SDA_PIN    2
#define PICO_I2C_SCL_PIN    3

i2c_init(PICO_I2C_INSTANCE, 400 * 1000);
gpio_set_function(PICO_I2C_SDA_PIN, GPIO_FUNC_I2C);
gpio_set_function(PICO_I2C_SCL_PIN, GPIO_FUNC_I2C);
```

----
From Gemini 2.5

Okay, let's break down the command and reply formats for the PN532 when using UART (HSU) and I2C interfaces. The good news is that the fundamental packet structure for commands and responses is largely the *same* for both interfaces, although the way these packets are exchanged over the physical bus differs slightly.

The PN532 uses a specific frame format for communication:

**1. Standard Information Frame Structure (Commands & Responses)**

This is the main format used for sending commands to the PN532 and receiving responses or data from it.

```
+------------+------------+-----+-----+-----+------------+-----+------------+
|  PREAMBLE  | START CODE | LEN | LCS | TFI |    DATA    | DCS | POSTAMBLE  |
| (0x00)     | (0x00 0xFF)|(1B) |(1B) |(1B) | (n bytes)  |(1B) |  (0x00)    |
+------------+------------+-----+-----+-----+------------+-----+------------+
```

* **PREAMBLE (1 byte):** `0x00` (Sometimes considered part of the Start Code).
* **START CODE (2 bytes):** `0x00 0xFF`. Marks the beginning of a valid frame.
* **LEN (1 byte):** Specifies the number of bytes in the DATA field *plus* the TFI byte. So, `LEN = length(TFI + DATA)`.
* **LCS (1 byte):** Length Checksum. This byte ensures the length field is correct. It's calculated such that `(LEN + LCS) & 0xFF = 0x00`. (It's the 8-bit two's complement of LEN).
* **TFI (1 byte):** Frame Identifier (or Direction indicator).
    * `0xD4`: Frame sent from the Host Controller -> PN532 (a command).
    * `0xD5`: Frame sent from the PN532 -> Host Controller (a response).
* **DATA (n bytes):** The actual payload.
    * For commands (TFI=0xD4), the first byte (`PD0`) is the **Command Code**, followed by command parameters (`PD1` to `PDn`).
    * For responses (TFI=0xD5), the first byte (`PD0`) is usually the **Response Code** (often `Command Code + 1`), followed by response data (`PD1` to `PDn`).
* **DCS (1 byte):** Data Checksum. This checksum covers the TFI and DATA fields. It's calculated such that `(TFI + PD0 + PD1 + ... + PDn + DCS) & 0xFF = 0x00`. (It's the 8-bit two's complement of the sum of TFI and DATA bytes).
* **POSTAMBLE (1 byte):** `0x00`. Marks the end of the frame.

**2. ACK Frame**

This is a short acknowledgement frame sent by the PN532 immediately after receiving a valid command frame, indicating it was received correctly and the PN532 is ready for the next command or is processing the current one.

```
+------------+------------+------------+------------+------------+------------+
|  PREAMBLE  | START CODE |   ACK CODE   |   ACK CODE   | POSTAMBLE  |
| (0x00)     | (0x00 0xFF)|   (0x00)     |   (0xFF)     |  (0x00)    |
+------------+------------+------------+------------+------------+------------+
```
Total Frame: `0x00 0x00 0xFF 0x00 0xFF 0x00`

**3. NACK Frame**

This is a short negative acknowledgement frame sent by the PN532 if it receives an invalid frame (e.g., checksum error, syntax error).

```
+------------+------------+------------+------------+------------+------------+
|  PREAMBLE  | START CODE |  NACK CODE   |  NACK CODE   | POSTAMBLE  |
| (0x00)     | (0x00 0xFF)|   (0xFF)     |   (0x00)     |  (0x00)    |
+------------+------------+------------+------------+------------+------------+
```
Total Frame: `0x00 0x00 0xFF 0xFF 0x00 0x00`

**Interface Specifics:**

* **UART (HSU):**
    * You send the entire frame (Preamble to Postamble) as a sequence of bytes over the TX line.
    * You receive frames (ACK, NACK, or Information Frames) as a sequence of bytes over the RX line.
    * Ensure the baud rate matches between the host and PN532 (often 115200 bps default).
    * A "wake-up" sequence might be needed after reset before the first command (e.g., sending `0x55 0x55 0x00 0x00 0x00 0x00` or similar, followed by a `SAMConfiguration` command).
* **I2C:**
    * The PN532 acts as an I2C slave (check the module's documentation for the address, often `0x48` which corresponds to `0x24` for 7-bit addressing).
    * **Sending Command:** Perform an I2C write transaction to the PN532's address, sending the complete command frame (Preamble to Postamble).
    * **Receiving Response:**
        1.  You usually need to know when the PN532 has data ready. This is done either by monitoring the IRQ pin (if connected) or by **polling**.
        2.  **Polling:** Perform an I2C read transaction of a single byte. The LSB of this byte typically indicates readiness (1 = ready, 0 = not ready).
        3.  Once ready (IRQ triggered or poll successful), perform another I2C read transaction. The PN532 will send the ACK frame first, followed by the Response Information Frame (Preamble to Postamble). You read the necessary number of bytes to get the full response.

In summary, while the packet *content* and structure (including checksums) are the same, I2C requires explicit addressing and a mechanism (polling or IRQ) to know when to read response data, whereas UART treats it as a continuous stream of bytes.