# my_presenter

A C++ PDF presentation tool built with Qt6, featuring a dual-window "Presenter Console" designed for professional use.

## Features

- **Dual-Window Mode**:
    - **Audience View**: Automatically opens on a Secondary Screen (if detected) or as a separate window. Shows only the slides in fullscreen.
    - **Presenter Console**: Runs on the Primary Screen. Includes:
        - **Current Slide**: Large view of what the audience sees.
        - **Next Slide Preview**: Shows the upcoming slide.
        - **Notes View**: Basic text area for speaker notes.
        - **Table of Contents**: Navigate using PDF bookmarks (chapters).
- **Control & Navigation**:
    - Intuitive keyboard controls.
    - Clickable TOC sidebar.
- **Laser Pointer**: Toggle a virtual laser pointer on the audience screen (Toggle with `L`).

## Prerequisites

- **Qt 6**: Requires Qt 6 Core, Gui, Widgets, and Pdf modules.
- **Build Tools**: `make`, `g++`, `qmake6`.

## Installation & Build

### 1. Install Dependencies
We provide a helper script for Debian-based systems to checks and install dependencies:

```bash
./install_dependencies.sh
```

### 2. Build the Application
```bash
make
```

## Usage

Run the application:
```bash
./bin/app
```

### Controls (Presenter Console)

| Key | Action |
| :--- | :--- |
| **Right Arrow** / **Down** / **Space** | Next Slide |
| **Left Arrow** / **Up** / **Backspace** | Previous Slide |
| **L** | Toggle Laser Pointer |
| **Esc** | Exit Application |

**Note**: To control the presentation, ensure the **Presenter Console** window has focus.

## Repository Structure

- `src/`: Source code (`main.cpp`, `mainwindow.cpp`, `presentationdisplay.cpp`).
- `include/`: Header files.
- `bin/`: Compiled executable (`app`).
- `my_presenter.pro`: Qt project configuration.
- `Makefile`: Build wrapper script.