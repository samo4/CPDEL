#!/bin/bash

# ESP-IDF build, flash, and monitor helper.
# Run from the repo root.

PORT="${1:-COM3}"
ESP_DIR="esp"

# Check IDF_PATH is set
if [ -z "${IDF_PATH}" ]; then
    echo "Error: IDF_PATH is not set. Install ESP-IDF and set IDF_PATH to its root directory."
    echo "  https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/"
    exit 1
fi

# Source ESP-IDF environment if idf.py is not already on PATH
if ! command -v idf.py &>/dev/null; then
    echo "idf.py not on PATH — sourcing ${IDF_PATH}/export.sh ..."
    # shellcheck disable=SC1091
    . "${IDF_PATH}/export.sh" || { echo "Error: failed to source ${IDF_PATH}/export.sh"; exit 1; }
fi

set -e

cd "${ESP_DIR}"

# Set target on first run (creates sdkconfig)
if [ ! -f "sdkconfig" ]; then
    echo "Setting target to esp32s3..."
    idf.py set-target esp32s3
    echo "Fetching dependencies (LVGL)..."
    idf.py update-dependencies
fi

echo "Building..."
idf.py build

echo "Flashing to ${PORT} and opening monitor..."
idf.py -p "${PORT}" flash monitor
