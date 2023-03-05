# SouSim DC electronic load control with ESP32

## Install

Add library AsyncElegantOTA

```
cd Arduino/libraries
git clone https://github.com/me-no-dev/ESPAsyncWebServer.git
git clone https://github.com/me-no-dev/AsyncTCP.git
git clone https://github.com/eModbus/eModbus.git
```

Update eModbus file RTUutils.cpp to fix it for ESP32-S2:

```c
    if ((void *)&serial == (void *)&Serial) {
      uart_num = 0;
      uart = &UART0;
    } else {
      if ((void *)&serial == (void *)&Serial1) {
        uart_num = 1;
        uart = &UART1;
      } else {
        #if SOC_UART_NUM > 2
        if ((void *)&serial == (void *)&Serial2) {
          uart_num = 2;
          uart = &UART2;
        }
        #endif
      }
    }
```

Use https://github.com/jandrassy/my_boards for boards location.

With custom partition table:

```
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x5000,
otadata,  data, ota,     0xe000,  0x2000,
app0,     app,  ota_0,   0x10000, 0x180000,
app1,     app,  ota_1,   0x190000,0x180000,
spiffs,   data, spiffs,  0x310000,0x90000,
```

```
esp32s2ucp.menu.PartitionScheme.ucp=UCP (1.5MB APP with OTA/0.56MB SPIFFS)
esp32s2ucp.menu.PartitionScheme.ucp.build.partitions=ucp
esp32s2ucp.menu.PartitionScheme.ucp.upload.maximum_size=1572864
```


### Update

go to Sketch > Export Compiled Binary. A .bin file will be generated from your sketch. Open your device address /update and upload. Dont' forget to upload LittleFS with https://github.com/lorol/arduino-esp32fs-plugin.

### Built with

- https://emodbus.github.io/
- LVGL 7.11.0
- TFT_eSPI 2.3.70

#### Random links:

- https://docs.espressif.com/projects/espressif-esp-iot-solution/en/latest/hw-reference/ESP-Prog_guide.html
- LV_USE_ASSERT_MEM_INTEGRITY https://forum.lvgl.io/t/lv-draw-label-exception-in-esp32/3811/15
- https://docs.espressif.com/projects/esp-idf/en/v4.2.3/esp32s2/api-guides/tools/idf-monitor.html#automatic-address-decoding

# SouSim protocol

## Holding registers

Read command: 0x03
Write a single hold register: 0x06

|   |   |   |
|---|---|---|
| 0x0000 | 40001  | Set slave address (1-255) |
| 0x0001 | 40002  | Mode setting (see below) |
| 0x0002 | 40003  | Enable (0: off, 1: on) |
| 0x0003 | 40004  | Set voltage [10mV] (20000 default =200.00V) |
| 0x0004 | 40005  | Set current [mA] |
| 0x0005 | 40006  | Set CP power [100mW] ? ?  HR9 Over current setting (5500= 55A) ?  |
| 0x0006 | 40007  | Set CR resistance [100mOhm] |
| 0x0007 | 40008  | Functions (see below) |
| 0x0008 | 40009  | Calibration: voltage offset? (def: 4)  |
| 0x0009 | 40010  | Calibration: voltage scale? (def: 10100) |
| 0x000A | 40011  | Calibration: current offset? (def: 14750) |
| 0x000B | 40012  | Calibration: current scale? (def: 13674) |

Modes (constant):

- 0: voltage
- 1: current
- 2: power
- 3: resistance
- 4: voltage and current

Functions detail description:

- 1 fan low
- 2 fan mid
- 3 fan high
- 4 buzzer on
- 5 mAh reset
- 6 Wh reset
- 10 voltage tracking off
- 11 voltage tracking on
- 12 current tracking off
- 13 current tracking on
- can modify other standard baud rate: 4800,9600,14400,19200,38400,56000,57600 (default is 9600) ?

## Input registers

Read command: 0x04

|   |   |   |
|---|---|---|
| 0x0000 | 30001  | slave address |
| 0x0001 | 30002  | current mode (0..4) |
| 0x0002 | 30003  | enabled (0..1) |
| 0x0003 | 30004  | temperature 1 (0..100C) |
| 0x0004 | 30005  | EXT temperature 2 (N/C) |
| 0x0005 | 30006  | EXT DVM measurement voltage [mV] |
| 0x0006 | 30007  | voltage [10mV] |
| 0x0007 | 30008  | current [mA] |
| 0x0008 | 30009  | power [mW] |
| 0x0009 | 30010  | current resistance ?  |
| 0x000A | 30011  | current capacity [mAh] ?   |
| 0x000B | 30012  | current capacity [WAh] ?  |
| 0x000C | 30013  | running time [s] ?  |


## Example RTU request

```
01 03 00 00 00 0D 84 0F
```

01 - slave address
03 - read holding registers
00 00 - The address of the first register (40108-40108 = 0x0000 )
00 0D - number of requrired registers (13)
84 0F - CRC checksum
