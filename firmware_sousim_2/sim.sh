#!/bin/bash

VCPKG_TOOLCHAIN="C:/Repos/External/vcpkg/scripts/buildsystems/vcpkg.cmake"
BUILD_DIR="sim/build"

# Handle clean flag
if [[ "$1" == "--clean" || "$1" == "-c" ]]; then
    echo "Cleaning build directory..."
    rm -f "${BUILD_DIR}/CMakeCache.txt"
    cmake --build "${BUILD_DIR}" --target clean --config Debug 2>/dev/null || rm -rf "${BUILD_DIR}"
    echo "Clean complete."
    exit 0
fi

# Configure if no cache exists
if [ ! -f "${BUILD_DIR}/CMakeCache.txt" ]; then
    echo "Configuring project..."
    cmake -B "${BUILD_DIR}" -S sim "-DCMAKE_TOOLCHAIN_FILE=${VCPKG_TOOLCHAIN}"
    if [ $? -ne 0 ]; then
        echo "CMake configuration failed."
        exit 1
    fi
fi

# Rebuild the project
cmake --build "${BUILD_DIR}" --config Debug

# Check if build was successful
if [ $? -eq 0 ]; then
    echo "Build successful. Running application..."
    # Determine the executable path based on known output locations
    # For multi-configuration generators (like Visual Studio), it's in sim/build/Debug
    # For single-configuration generators (like Ninja), it might be directly in sim/build

    if [ -f "./${BUILD_DIR}/Debug/lvgl_mockup.exe" ]; then
        ./${BUILD_DIR}/Debug/lvgl_mockup.exe
    elif [ -f "./${BUILD_DIR}/lvgl_mockup.exe" ]; then
        ./${BUILD_DIR}/lvgl_mockup.exe
    else
        echo "Error: Executable not found. Checked ./${BUILD_DIR}/Debug/lvgl_mockup.exe and ./${BUILD_DIR}/lvgl_mockup.exe"
        exit 1
    fi
else
    echo "Build failed."
    exit 1
fi
