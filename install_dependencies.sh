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
    if [ "$(uname)" == "Darwin" ]; then
         echo "macOS detected."
         
         if ! command -v brew >/dev/null 2>&1; then
             echo "Error: Homebrew is not installed. Please install Homebrew first: https://brew.sh/"
             exit 1
         fi

         MISSING_PACKAGES=""

         # Check for qt formula (which provides qt6)
         if ! brew list qt >/dev/null 2>&1 && ! brew list qt@6 >/dev/null 2>&1; then
              echo "Missing: qt"
              MISSING_PACKAGES="$MISSING_PACKAGES qt"
         fi
         
         if [ -n "$MISSING_PACKAGES" ]; then
             echo "Installing dependencies..."
             brew install $MISSING_PACKAGES
             echo "Dependencies installed."
         else
             echo "Qt dependency appears to be installed."
         fi
         
         # Qt 6 is keg-only, check if it's in the path or suggest adding it
         # Use brew --prefix qt to finding the path
         QT_PATH=$(brew --prefix qt 2>/dev/null)
         if [ -d "$QT_PATH/bin" ]; then
             if ! echo "$PATH" | grep -q "$QT_PATH/bin"; then
                 echo ""
                 echo "NOTE: Qt 6 is keg-only. You may need to add it to your PATH to build."
                 echo "Run the following command:"
                 echo "  export PATH=\"$QT_PATH/bin:\$PATH\""
                 echo ""
             fi
         fi

    else
        echo "This script currently supports Debian-based systems and macOS."
        echo "Please ensure you have Qt6 (Core, Gui, Widgets, Pdf) development libraries installed manually."
        exit 1
    fi
fi

echo "Ready to build."
