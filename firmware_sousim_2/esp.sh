#!/bin/bash
# ESP-IDF helper — delegates to esp.ps1 (ESP-IDF v6 requires PowerShell on Windows).
#
# Usage:
#   ./esp.sh                  — build
#   ./esp.sh flash [PORT]     — build + flash  (default COM3)
#   ./esp.sh monitor [PORT]   — open serial monitor
#   ./esp.sh flash-monitor [PORT] — flash then monitor
#   ./esp.sh menuconfig       — open interactive config menu
#   ./esp.sh clean            — clean build artefacts
#   ./esp.sh update-deps      — update managed components
#   ./esp.sh size             — image size summary
#   ./esp.sh size-components  — size by component
#   ./esp.sh size-files       — size by object/source file
#   ./esp.sh build-size       — build then print all size reports

CMD="${1:-build}"
PORT="${2:-COM14}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PS1_WIN="$(cygpath -w "${SCRIPT_DIR}/esp.ps1" 2>/dev/null || echo "${SCRIPT_DIR}/esp.ps1")"

exec powershell.exe -NoProfile -ExecutionPolicy Bypass \
    -File "${PS1_WIN}" \
    -Command "${CMD}" \
    -Port "${PORT}"
