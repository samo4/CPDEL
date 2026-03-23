#!/bin/bash

# Check if we are in a Bash shell (Git Bash/MinGW)
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
    echo -e "\033[1;31m❌ ERROR: ESP-IDF commands clash with Git Bash (MinGW).\033[0m"
    echo -e "\033[1;33m👉 Please use the dedicated ESP-IDF PowerShell terminal.\033[0m"
    echo ""
    echo -e "\033[1;32mHOW TO OPEN IT:\033[0m"
    echo -e "  1. Click the \033[1;36m[+]\033[0m dropdown in the VS Code terminal panel."
    echo -e "  2. Select \033[1;36m'ESP-IDF PowerShell'\033[0m."
    echo ""
    echo "Usage once inside PowerShell:"
    echo "  esp                  — build"
    echo "  esp flash [PORT]     — build + flash"
    echo "  esp monitor          — serial monitor"
    exit 1
fi
