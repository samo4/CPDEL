cmake_minimum_required(VERSION 3.16)
project(dc_load_gui VERSION 1.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# --------------------------------------------------------------------------
# LVGL — downloaded automatically at configure time via FetchContent.
# On first "cmake .." CMake clones LVGL v8.3.11 into build/_deps/lvgl-src/.
# No manual git clone or submodule needed.
# --------------------------------------------------------------------------
include(FetchContent)

# Point LVGL to our lv_conf.h before it is compiled.
set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/lv_conf.h" CACHE STRING "" FORCE)

FetchContent_Declare(
    lvgl
    GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG        v8.3.11
    GIT_SHALLOW    TRUE   # minimal download — no full history
)
FetchContent_MakeAvailable(lvgl)

# --------------------------------------------------------------------------
# SDL2 — installed once via vcpkg (see README)
# --------------------------------------------------------------------------
find_package(SDL2 REQUIRED)

# --------------------------------------------------------------------------
# Source files
# --------------------------------------------------------------------------
file(GLOB_RECURSE SOURCES
    "src/*.c"
    "src/*.cpp"
)

add_executable(dc_load_gui ${SOURCES})

target_include_directories(dc_load_gui PRIVATE
    src
    ${lvgl_SOURCE_DIR}   # populated by FetchContent
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(dc_load_gui
    lvgl
    ${SDL2_LIBRARIES}
)

# Windows: SDL2 needs winmm
if(WIN32)
    target_link_libraries(dc_load_gui winmm)
endif()
