# Sousim 2-CH DC Load

## TODO

- [x] wireless edit ssid and password (store in nvme)
- [ ] connect to modbus
- [ ] rethink queues

## vscode

## PC Simulator

### Prerequisites (Windows)

- vcpkg
- Visual Studio with C++ workload
- `vcpkg install sdl2`

```
./sim.sh           # build & run
./sim.sh --clean   # clean build
```

## ESP32-S2

### Prerequisites

- ESP-IDF
  - `winget install Espressif.EIM-CLI`
  - `eim install` (in elevated cmd.exe, not bash)
- `+` on terminal to add ESP-IDF PowerShell terminal

```
./esp.sh COMx      # build, flash, monitor (default port: COM3)
```

**Before first flash:** implement the display flush callback and touch input driver
in [src/main_esp.c](src/main_esp.c) (marked with `TODO`).
