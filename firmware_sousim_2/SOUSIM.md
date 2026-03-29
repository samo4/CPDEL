# SouSim protocol

## Holding registers

Read command: 0x03
Write a single hold register: 0x06

|        |       |                                                                 |
| ------ | ----- | --------------------------------------------------------------- |
| 0x0000 | 40001 | Set slave address (1-255)                                       |
| 0x0001 | 40002 | Mode setting (see below)                                        |
| 0x0002 | 40003 | Enable (0: off, 1: on)                                          |
| 0x0003 | 40004 | Set voltage [10mV] (20000 default =200.00V)                     |
| 0x0004 | 40005 | Set current [mA]                                                |
| 0x0005 | 40006 | Set CP power [100mW] ? ? HR9 Over current setting (5500= 55A) ? |
| 0x0006 | 40007 | Set CR resistance [100mOhm]                                     |
| 0x0007 | 40008 | Functions (see below)                                           |
| 0x0008 | 40009 | Calibration: voltage offset? (def: 4)                           |
| 0x0009 | 40010 | Calibration: voltage scale? (def: 10100)                        |
| 0x000A | 40011 | Calibration: current offset? (def: 14750)                       |
| 0x000B | 40012 | Calibration: current scale? (def: 13674)                        |

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

|        |       |                                  |
| ------ | ----- | -------------------------------- |
| 0x0000 | 30001 | slave address                    |
| 0x0001 | 30002 | current mode (0..4)              |
| 0x0002 | 30003 | enabled (0..1)                   |
| 0x0003 | 30004 | temperature 1 (0..100C)          |
| 0x0004 | 30005 | EXT temperature 2 (N/C)          |
| 0x0005 | 30006 | EXT DVM measurement voltage [mV] |
| 0x0006 | 30007 | voltage [10mV]                   |
| 0x0007 | 30008 | current [mA]                     |
| 0x0008 | 30009 | power [mW]                       |
| 0x0009 | 30010 | current resistance ?             |
| 0x000A | 30011 | current capacity [mAh] ?         |
| 0x000B | 30012 | current capacity [WAh] ?         |
| 0x000C | 30013 | running time [s] ?               |

## Example RTU request

```
01 03 00 00 00 0D 84 0F
```

01 - slave address
03 - read holding registers
00 00 - The address of the first register (40108-40108 = 0x0000 )
00 0D - number of requrired registers (13)
84 0F - CRC checksum
