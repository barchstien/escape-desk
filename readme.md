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