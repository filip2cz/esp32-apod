# esp32-apod

## This project was vibecoded with Gemini Pro 3.1
https://share.gemini.google/rOalVXdOklmJ

## Hardware I used (it can work with other hardware tho)

### [ESP32-WROOM-32D USB-C](https://www.aliexpress.com/item/1005007160771703.html)
docs: [https://documentation.espressif.com/esp32-wroom-32d_esp32-wroom-32u_datasheet_en.pdf](https://documentation.espressif.com/esp32-wroom-32d_esp32-wroom-32u_datasheet_en.pdf)

### [LaskaKit_cz 2.2" 240x320 TFT display, ILI9341, SPI](https://www.laskakit.cz/240x320-barevny-lcd-tft-displej-2-2--spi/)

## Wiring

| Display pin   | ESP32 pin |
| ------------- | --------- |
| VCC           | VIN       |
| GND           | GND       |
| CS            | D5        |
| RESET         | D22       |
| DC            | D21       |
| SDI(MOSI)     | D23       |
| SCK           | D18       |
| LED           | 3V3       |
| SDO(MISO)     | D19       |
