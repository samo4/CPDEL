cmake_minimum_required(VERSION 3.16)
project(lvgl_mockup LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Optimize build time by disabling unused components
set(LV_BUILD_EXAMPLES OFF CACHE BOOL "Disable examples" FORCE)
set(LV_BUILD_DEMOS OFF CACHE BOOL "Disable demos" FORCE)
# Also commonly used
set(LV_USE_DEMO_WIDGETS OFF CACHE BOOL "Disable demo widgets" FORCE)

# Fetch LVGL
include(FetchContent)
FetchContent_Declare(
    lvgl
    GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG v8.3.10
)
FetchContent_MakeAvailable(lvgl)

# Fetch LVGL Drivers (SDL2 support)
FetchContent_Declare(
    lv_drivers
    GIT_REPOSITORY https://github.com/lvgl/lv_drivers.git
    GIT_TAG v8.2.0
)
FetchContent_MakeAvailable(lv_drivers)

# Fetch FreeRTOS-Kernel (Windows MSVC simulation port)
# freertos_config interface library must be created BEFORE FetchContent_MakeAvailable
add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

FetchContent_Declare(
    freertos_kernel
    GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel.git
    GIT_TAG V11.1.0
)
set(FREERTOS_PORT "MSVC_MINGW" CACHE STRING "FreeRTOS port" FORCE)
set(FREERTOS_HEAP "4" CACHE STRING "FreeRTOS heap" FORCE)
FetchContent_MakeAvailable(freertos_kernel)

# Configure imported targets to use our local config files
target_include_directories(lvgl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE)

target_include_directories(lv_drivers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(lv_drivers PUBLIC LV_CONF_INCLUDE_SIMPLE USE_SDL)

# Find SDL2 (Required for PC simulation)
find_package(SDL2 CONFIG QUIET)
if(NOT SDL2_FOUND)
    message(FATAL_ERROR "SDL2 not found! Please install it via vcpkg:\n  > .\\vcpkg\\vcpkg install sdl2\n  > .\\vcpkg\\vcpkg integrate install")
endif()

# Link SDL2 to LVGL and Drivers so they can find headers
target_link_libraries(lvgl PUBLIC SDL2::SDL2)
target_link_libraries(lv_drivers PUBLIC SDL2::SDL2)

# List sources
add_executable(lvgl_mockup
    main.c
    ../src/ui/ui.c
    ../src/ui/ui_channel_detail.c
    ../src/ui/ui_graph.c
    ../src/ui/ui_main.c
    ../src/ui/ui_numpad.c
    ../src/ui/ui_keyboard.c
    ../src/ui/ui_ota.c
    ../src/ui/ui_settings.c
    ../src/ui/ui_wireless.c
)

# Include directories
target_include_directories(lvgl_mockup PRIVATE
    .
    ../src
    ../src/ui
    ${lvgl_SOURCE_DIR}
    ${lv_drivers_SOURCE_DIR}
)

# Link
target_link_libraries(lvgl_mockup PRIVATE
    lvgl::lvgl
    lv_drivers
    SDL2::SDL2
    SDL2::SDL2main
    freertos_kernel
)

# Configuration definitions
target_compile_definitions(lvgl_mockup PRIVATE
    LV_CONF_INCLUDE_SIMPLE
    LV_LVGL_H_INCLUDE_SIMPLE
    USE_SDL
)
