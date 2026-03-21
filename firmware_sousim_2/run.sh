#!/bin/bash

VCPKG_TOOLCHAIN="C:/Repos/External/vcpkg/scripts/buildsystems/vcpkg.cmake"

# Handle clean flag
if [[ "$1" == "--clean" || "$1" == "-c" ]]; then
    echo "Cleaning build directory..."
    cmake --build build --target clean --config Debug 2>/dev/null || rm -rf build
    echo "Clean complete."
    exit 0
fi

# Configure if no cache exists
if [ ! -f "build/CMakeCache.txt" ]; then
    echo "Configuring project..."
    cmake -B build -S . "-DCMAKE_TOOLCHAIN_FILE=${VCPKG_TOOLCHAIN}"
    if [ $? -ne 0 ]; then
        echo "CMake configuration failed."
        exit 1
    fi
fi

# Rebuild the project
cmake --build build --config Debug

# Check if build was successful
if [ $? -eq 0 ]; then
    echo "Build successful. Running application..."
    # Determine the executable path based on known output locations
    # For multi-configuration generators (like Visual Studio), it's in build/Debug
    # For single-configuration generators (like Ninja), it might be directly in build or bin
    
    if [ -f "./build/Debug/lvgl_mockup.exe" ]; then
        ./build/Debug/lvgl_mockup.exe
    elif [ -f "./build/lvgl_mockup.exe" ]; then
        ./build/lvgl_mockup.exe
    else
        echo "Error: Executable not found. Checked ./build/Debug/lvgl_mockup.exe and ./build/lvgl_mockup.exe"
        exit 1
    fi
else
    echo "Build failed."
    exit 1
fi
