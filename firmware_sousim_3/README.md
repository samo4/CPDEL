# DC Electronic Load GUI — LVGL Mockup

Windows simulator for a 2-channel DC electronic load GUI.  
Target hardware: **ESP32-S2** + **4DLCD-24320240** (IL9341, 240×320, FT6236 touch).

---

## Screens

| Screen             | Description                                                                                                         |
| ------------------ | ------------------------------------------------------------------------------------------------------------------- |
| **Main**           | Overview of both channels (mode badge, setpoint, V/I/P, on/off, settings access)                                    |
| **Channel Detail** | Full detail for one channel: setpoint ±, V/I/P meters, LV cutoff toggle + threshold, mini chart, link to full graph |
| **Graph**          | Full-screen rolling voltage + current chart for one channel                                                         |
| **Settings**       | Brightness, auto-off, beep, fan speed, CH mode (CC/CV) selectors                                                    |

---

## Building on Windows

### Prerequisites

You already have **Visual Studio** — it provides the MSVC compiler, the linker, and (since VS 2019 16.x) a bundled CMake. Nothing below installs a compiler; they are libraries and tools only.

| #   | What                 | How to get                                                                                                                                                                   | Notes                                                                                    |
| --- | -------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------- |
| 1   | **C/C++ compiler**   | Visual Studio 2019 or 2022 — workload _"Desktop development with C++"_                                                                                                       | Already installed. Make sure the workload is ticked.                                     |
| 2   | **CMake ≥ 3.16**     | Bundled inside VS (`C:\Program Files\Microsoft Visual Studio\...\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin`) **or** standalone from https://cmake.org/download/ | Run `cmake --version` in a Developer Command Prompt to confirm.                          |
| 3   | **Git**              | https://git-scm.com/download/win                                                                                                                                             | Required by CMake `FetchContent` to download LVGL automatically on first configure.      |
| 4   | **vcpkg**            | `git clone https://github.com/microsoft/vcpkg C:\vcpkg` then `C:\vcpkg\bootstrap-vcpkg.bat`                                                                                  | One-time setup. Installs SDL2 headers + pre-built DLLs. Does **not** install a compiler. |
| 5   | **SDL2** (via vcpkg) | `C:\vcpkg\vcpkg install sdl2:x64-windows` then `C:\vcpkg\vcpkg integrate install`                                                                                            | Adds SDL2 to every MSBuild/CMake project automatically.                                  |
| 6   | **LVGL v8**          | Nothing — CMake downloads it automatically the first time you run `cmake ..`                                                                                                 | Stored in `build/_deps/lvgl-src/`, not committed to the repo.                            |

### Build

Open a **x64 Native Tools Command Prompt for VS** (or any terminal where `cl.exe` is on PATH), then:

```cmd
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
:: ^^^ First run: CMake downloads LVGL v8.3.11 automatically into build/_deps/
cmake --build . --config Release
Release\dc_load_gui.exe
```

> **Tip — Visual Studio IDE:** Open the `firmware_sousim_3` folder directly in VS 2019/2022 via _File → Open → Folder_. VS auto-detects the CMakeLists.txt. In _Project → CMake Settings_, add the toolchain file path under "CMake toolchain file" and VS handles everything.

If SDL2 was installed to a custom path instead of vcpkg, pass:

```
-DSDL2_DIR=C:/path/to/SDL2/cmake
```

---

## Porting to ESP32-S2

### Build system differences

The Windows simulator uses **vanilla CMake** with `FetchContent`.  
The ESP32 target uses **ESP-IDF**, which wraps CMake with its own component system (`idf.py`). `FetchContent` and `find_package(SDL2)` are not used. Instead, dependencies are declared in an `idf_components.yml` manifest and downloaded automatically by `idf.py build` — same concept, different tool.

```
Windows simulator           ESP32-S2 target
─────────────────────       ──────────────────────────────
CMakeLists.txt              CMakeLists.txt (2-liner, ESP-IDF style)
  FetchContent → LVGL         idf_components.yml → LVGL (auto-downloaded)
  find_package(SDL2)          (no SDL2 — real display driver instead)
src/main.c (main())         main/main.c (app_main(), FreeRTOS task)
src/hal_sdl.c               main/hal_esp32.c  ← only this file changes
src/screen_*.c   ────────────────────────────► main/screen_*.c  (UNCHANGED)
src/app_data.c   ────────────────────────────► main/app_data.c  (tick() reads HW)
src/lv_conf.h    ────────────────────────────► main/lv_conf.h   (tune LV_MEM_SIZE)
```

### ESP32 project structure

```
dc_load_esp32/
├── CMakeLists.txt          ← top-level (3 lines — just calls ESP-IDF)
├── idf_components.yml      ← declares LVGL; idf.py downloads it automatically
├── sdkconfig               ← generated by menuconfig
└── main/
    ├── CMakeLists.txt      ← lists source files, requires lvgl + driver
    ├── main.c              ← app_main() + FreeRTOS task
    ├── hal_esp32.c/h       ← IL9341 SPI flush + FT6236 I2C read callbacks
    ├── app_data.c/h        ← tick() reads real ADC / protocol instead of sim
    ├── screen_manager.c/h
    ├── screen_main.c/h
    ├── screen_channel.c/h
    ├── screen_graph.c/h
    ├── screen_settings.c/h
    └── lv_conf.h
```

### ESP32 CMakeLists.txt (top-level, 3 lines)

```cmake
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(dc_load_esp32)
```

### ESP32 main/CMakeLists.txt

```cmake
idf_component_register(
    SRCS
        "main.c"
        "hal_esp32.c"
        "app_data.c"
        "screen_manager.c"
        "screen_main.c"
        "screen_channel.c"
        "screen_graph.c"
        "screen_settings.c"
    INCLUDE_DIRS "."
    REQUIRES lvgl esp_timer driver spi_flash
)
```

### idf_components.yml — auto-downloads LVGL

```yaml
dependencies:
  lvgl/lvgl:
    version: ">=8.3.11,<9.0.0"
```

Running `idf.py build` fetches LVGL from the ESP Component Registry on first build — equivalent to `FetchContent` in the Windows build.

### What actually changes for ESP32

| File               | Change                                                                                                                      |
| ------------------ | --------------------------------------------------------------------------------------------------------------------------- |
| `hal_esp32.c`      | **New file.** Register `lv_disp_drv` with IL9341 SPI flush callback; register `lv_indev_drv` with FT6236 I2C read callback. |
| `main.c`           | `main()` → `app_main()`; main loop becomes a FreeRTOS task with `vTaskDelay`.                                               |
| `app_data.c`       | `app_data_tick()` reads real ADC / UART / SPI data instead of `sinf()` simulation.                                          |
| `lv_conf.h`        | Lower `LV_MEM_SIZE` (e.g. 48 KB SRAM or use PSRAM); disable fonts not needed.                                               |
| `screen_*.c`       | **No changes.** Pure LVGL API — portable as-is.                                                                             |
| `screen_manager.c` | **No changes.**                                                                                                             |
| `app_data.h`       | **No changes.**                                                                                                             |

---

## File Structure

```
firmware_sousim_3/
├── CMakeLists.txt       ← FetchContent pulls LVGL v8.3.11 automatically
├── README.md
└── src/                 (lib/ and lvgl/ are NOT committed; they live in build/_deps/)
    ├── lv_conf.h       ← LVGL configuration (display res, fonts, widgets)
    ├── app_data.h/.c   ← Shared state + demo simulation
    ├── hal_sdl.h/.c    ← SDL2 display + touch HAL (swap for ESP32)
    ├── screen_manager.h/.c
    ├── screen_main.h/.c
    ├── screen_channel.h/.c
    ├── screen_graph.h/.c
    ├── screen_settings.h/.c
    └── main.c
```
