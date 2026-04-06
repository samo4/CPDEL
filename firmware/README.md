# Sousim 2-CH DC Load aka CPDEL

This a firmeware for the control part of a 2-channel DC electronic load, built from [cheap chinese DC load modbus modules](SOUSIM.md). CPDAL stands for "Completely Pointless DC Electronic Load". The project is pushing what can be [squeezed](MEMORY.md) into an ESP32-S2.

## TODO

- [x] wireless edit ssid and password (store in nvme)
- [x] connect to modbus
- [x] rethink queues
- [ ] rrdtool-like graph [where do we put it?](MEMORY.md)
- [x] OTA.
- [ ] read SCPI_FLAG_ERROR and display in UI and react on SCPI
- [ ] crashes if you go back from OTA screen
- [ ] prevent OTA until fetch is happy
- [ ] heap fragmentation prevents webserver to start, even after restart: E (4260) main.c: (could be anywhere) Failed to allocate 1626 bytes (caps: 0x0000080c) in heap_caps_malloc — free: 2212, largest: 1216

### Ideas

- Implement testing, perhaps with [Unity](https://github.com/ThrowTheSwitch/Unity)
- consider using a [real SCPI parser](https://www.jaybee.cz/scpi-parser/basic/instrument/)

## Development

### How to start development

#### Simulator

- vcpkg
- Visual Studio with C++ workload
- `vcpkg install sdl2`

```
./sim.sh           # build & run
./sim.sh --clean   # clean build
```

#### Actual ESP32-S2 hardware

- ESP-IDF
  - `winget install Espressif.EIM-CLI`
  - `eim install` (in elevated cmd.exe, not bash)
- `+` on terminal to add ESP-IDF PowerShell terminal
  - `./esp.ps1 flash-monitor`

### Test

Install `pip install -U pytest`.
Run with `pytest tests/ --host 192.168.88.117`

Some SCPI commands to run:

```
*IDN?
OUTP1:STAT ON
OUTP1:STAT OFF
SOUR1:FUNC POW
SOUR1:FUNC VOLT
SOUR1:FUNC CURR
SOUR1:FUNC?
SOUR1:VOLT 2.2
SOUR1:VOLT?
MEAS:VOLT:CONT ON (@1)
MEAS:VOLT:CONT OFF (@1)
```

#### Development notes

To make sure `sdkconfig.default` is really used:

```bash
idf.py fullclean
idf.py reconfigure
./esp.ps1 flash-monitor
```

### How to update firmware

Other than `./esp.ps1 flash-monitor`, you can also update the firmware OTA via the UI (Settings -> Firmware Update). Currently, this is using [Surge](https://surge.sh/) to host the firmware binary on http (https is hard). The process is:

- you build the image locally with `./esp.ps1 build`
- you upload with `./esp.ps1 ota` (make sure the printed URL is the same as hardcoded in `ota.c`)
- on device: Settings -> Firmware Update
- note the slot and other details
- update
- check if everything is working (especially if you don't have console access)
- confirm the update in Settings -> Firmware Update

There's also the build task with github actions, which builds if you tag the commit `v*`. But github pages are only for public repos. And we have a script to increase version, commit, tag, push: `./release.sh `.
