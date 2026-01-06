#!/bin/bash

# Function to check if a package is installed
is_installed() {
    dpkg -s "$1" >/dev/null 2>&1
}

echo "Checking system dependencies..."

if [ -f /etc/debian_version ]; then
    echo "Debian-based system detected."
    
    MISSING_PACKAGES=""

    # Check for Qt6 Base Development files
    if ! is_installed "qt6-base-dev"; then
        echo "Missing: qt6-base-dev"
        MISSING_PACKAGES="$MISSING_PACKAGES qt6-base-dev"
    fi

    # Check for Qt6 PDF Development files
    if ! is_installed "qt6-pdf-dev"; then
        echo "Missing: qt6-pdf-dev"
        MISSING_PACKAGES="$MISSING_PACKAGES qt6-pdf-dev"
    fi
    
    # Check for make
    if ! command -v make >/dev/null 2>&1; then
        echo "Missing: make"
        MISSING_PACKAGES="$MISSING_PACKAGES build-essential"
    fi

    if [ -n "$MISSING_PACKAGES" ]; then
        echo "The following packages are missing: $MISSING_PACKAGES"
        echo "Installing dependencies..."
        
        # specific check for sudo availability
        if command -v sudo >/dev/null 2>&1; then
             sudo apt update
             sudo apt install -y $MISSING_PACKAGES
        else
             echo "Error: 'sudo' not found. Please install the following packages manually as root:"
             echo "apt install $MISSING_PACKAGES"
             exit 1
        fi
        
        echo "Dependencies installed successfully."
    else
        echo "All dependencies are satisfied."
    fi

else
    echo "This script currently supports Debian-based systems (Debian, Ubuntu, etc.)."
    echo "Please ensure you have Qt6 (Core, Gui, Widgets, Pdf) development libraries installed manually."
    exit 1
fi

echo "Ready to build."
