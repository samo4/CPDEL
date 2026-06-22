# CPDEL

## Completely Pointless DC Electronic Load

> This a 2-channel DC electronic load, built from [cheap chinese DC load modbus modules](./firmware/SOUSIM.md).

Features:

- 2 channels: 200V, 10A
- 4 modes (CC, CV, CR, CP) + battery discharge mode
- Control:
  - 2.8" SPI display with capacitive touch
  - telnet SCPI interface (port 5025)
  - REST SCPI interface (port 80)
  - responsive web interface (with sockets)
- rudimentary charting
- OTA updates with CI/CD integration
- not-unreasonable level of test coverage
- UDP logging for debugging

![CPDEL final result](docs/images/front.jpg)

## Development

You need a working installation of Python 3.x, [ESP-IDF v6.x](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/windows-setup.html) and a working C compiler (the Xtensa one and a native for your host system - tested only with Visual studio build system). Use vscode to open the firmware folder, which contains the `.vscode` settings, including the most important: initialization of ESP-IDF terminal.

In ESP-IDF terminal (folder `firmware/esp`) you can then run `./esp.ps1`

## Key takeaways

- don't use http servers for OTA that you don't fully control, because you have no control over TLS chain.. updates to chain can break your OTA
- don't use SPI display if you want full screen animations, it's too slow
- don't use single pole switches for mains...
- keep LVGL memory as low as possible, to crash early when we have a leak?

## LLM disclaimer

Hardware and first few versions of software were created the old fashined way. It worked, but only the exact features that I needed at the time. With the help of LLM, the code was rewritten to use ESP-IDF (compared to original Arduino FreeRTOSnstein) directly and implement basicly all of the features that one might expect from a DC load instrument. LLM was used heavily especially in the UI, http/ws/socket server code and test writing.

"Copilot Is For Entertainment Purposes Only" is written in the Copilot terms. Well, I was entertained.
