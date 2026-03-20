# Sousim 2-CH DC Load

## Prerequisites (Windows)

- vcpkg
- Visual Studio with C++ workload
- `vcpkg install sdl2`
- `CMakeLists.txt` wil automatically download LVGL

## Building

- `cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="C:/Repos/External/vcpkg/scripts/buildsystems/vcpkg.cmake"`
- `cmake --build build`
- run with `build\Debug\lvgl_mockup.exe`

## Project Structure

- `src/ui/`: Contains the UI logic (Target independent).
  - `ui.c/h`: Main entry point and global state.
  - `ui_main.c`: The main dashboard screen.
  - `ui_channel_detail.c`: The detailed channel control screen.
  - `ui_misc.c`: Graph and Settings screens.
- `src/main.c`: PC simulator entry point (initializes SDL, LVGL).
- `CMakeLists.txt`: Build configuration fetching LVGL and Drivers.

## Integrating with ESP32-S2

1. Copy the `src/ui` folder to your ESP-IDF component or Arduino sketch.
2. Call `ui_init()` after initializing your display driver.
3. Use the `channel_data_t channels[2]` array in `ui.h` to update values from your hardware backend.
