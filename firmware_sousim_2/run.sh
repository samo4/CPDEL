#!/bin/bash
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
