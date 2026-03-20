# LVGL PC Mockup for 2-CH DC Load

## detailed Windows Prerequisites

**1. C++ Compiler:**

- Since you have **Visual Studio**, you already have the compiler (MSVC).
- Ensure you have the **"Desktop development with C++"** workload installed in the Visual Studio Installer.
- **Note:** `vcpkg` does _not_ install a valid compiler; it strictly manages libraries.

**2. SDL2 (Graphics Library):**
The simulator needs SDL2 to create a window on Windows. The easiest way is via `vcpkg`.
Since your previous command failed, you likely need to install `vcpkg` first:

1.  Open a terminal (PowerShell or Command Prompt).
2.  Clone vcpkg: `git clone https://github.com/microsoft/vcpkg`
    _(If you don't have git, download from [git-scm.com](https://git-scm.com))_
3.  Initialize it: `.\vcpkg\bootstrap-vcpkg.bat`
4.  Install SDL2: `.\vcpkg\vcpkg install sdl2`
5.  **Important**: Make it visible to everything: `.\vcpkg\vcpkg integrate install`

**3. LVGL:**

- **You do NOT need to download LVGL manually.**
- The `CMakeLists.txt` file in this project is configured to **automatically download** the correct version of LVGL + Drivers from GitHub when you first build the project.

## Building and Running with VS Code

1.  Open this folder in VS Code.
2.  Install the **"CMake Tools"** extension by Microsoft.
3.  Reload VS Code. It should ask to configure the project.
4.  When asked to **"Select a Kit"**, choose **Visual Studio Community/Professional 20xx Release - amd64**.
5.  Press **F7** (or the "Build" button in the generic status bar) to build.
    - _The first build will take a moment to download LVGL._
6.  Press **Shift+F5** (or the "Run" button) to launch `lvgl_mockup.exe`.

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
