# my_presenter

A simplified C++ PDF presentation tool built with Qt6. This application allows you to present PDF slides in fullscreen mode with a built-in virtual laser pointer.

## Features

- **PDF Rendering**: Opens and displays PDF files using Qt's PDF module.
- **Fullscreen Presentation**: Automatically launches in fullscreen mode for professional presentations.
- **Navigation**: intuitive keyboard controls for slide navigation.
- **Laser Pointer**: specific key toggle to enable a red virtual laser pointer that follows the mouse cursor.

## Prerequisites

- **Qt 6**: Requires Qt 6 Core, Gui, Widgets, and Pdf modules.
- **Build Tools**: `make`, `g++`, `qmake6`.

## Installation & Build

### 1. Install Dependencies
We provide a helper script for Debian-based systems (Debian, Ubuntu, etc.) to automatically check and install missing dependencies:

```bash
./install_dependencies.sh
```

Alternatively, you can manually install the required packages:
```bash
sudo apt update
sudo apt install qt6-base-dev qt6-pdf-dev build-essential
```

### 2. Build the Application
Use the included `Makefile` to build the project. This will automatically invoke `qmake6` to configure the build.

```bash
make
```

To clean build artifacts:
```bash
make clean
```

## Usage

Run the compiled executable from the build directory:

```bash
./bin/app
```

### Controls

| Key | Action |
| :--- | :--- |
| **Right Arrow** / **Down Arrow** | Next Slide |
| **Left Arrow** / **Up Arrow** | Previous Slide |
| **L** | Toggle Laser Pointer |
| **Esc** | Exit Application |

## Repository Structure

- `src/`: Source code (`main.cpp`, `mainwindow.cpp`).
- `include/`: Header files (`mainwindow.h`).
- `bin/`: Compiled executable (`app`).
- `obj/`: Interface object files.
- `my_presenter.pro`: Qt project configuration.
- `Makefile`: Build wrapper script.