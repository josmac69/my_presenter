# my_presenter

A C++ PDF presentation tool built with Qt6, inspired by tools like **Pympress**. It features a dual-window "Presenter Console" designed for professional use with support for Beamer notes and presentation timing.

## What's New
- **Presentation Timers**: Added Wall Clock and Elapsed Presentation Time to the console.
- **Beamer Notes Support**: Added logic to handle split-page PDFs (Left=Slide, Right=Notes). Toggle with `S`.
- **Improved Console**: Layout updated to include timer bar and toggleable visual notes.

## Features

- **Dual-Window Mode**:
    - **Audience View**: Automatically opens on a Secondary Screen (if detected) or as a separate window. Shows only the slides in fullscreen.
    - **Presenter Console**: Runs on the Primary Screen. Includes:
        - **Current Slide**: Large view of what the audience sees.
        - **Next Slide Preview**: Shows the upcoming slide.
        - **Timers**:
            - **Current Time**: Standard wall clock.
            - **Elapsed Time**: Tracks duration since the presentation started.
        - **Notes View**:
            - Standard Mode: Text area for separate notes.
            - **Beamer Mode**: Visual view of the right-half of the slide (for split LaTeX Beamer slides).
        - **Table of Contents**: Navigate using PDF bookmarks (chapters).
- **Control & Navigation**:
    - Intuitive keyboard controls.
    - Clickable TOC sidebar.
    - Clickable TOC sidebar.
- **Enhanced Laser Pointer** (Toggle with `L`):
    - Switch colors with **R** (Red), **G** (Green), **B** (Blue).
    - Resize pointer with **+** / **-**.
    - Configurable **Size**, **Opacity**, and **Color** from the console.
- **Zoom Pointer** (Toggle with `Z`):
    - Magnifying glass effect.
    - Resize with **+** / **-**.
    - Configurable **Size** and **Magnification**.

## Screenshots

| Presenter Console | Audience View (Laser) |
| :---: | :---: |
| ![Presenter Console](screenshots/console_features.png) | ![Audience View](screenshots/audience_laser.png) |

## Prerequisites

- **Qt 6**: Requires Qt 6 Core, Gui, Widgets, and Pdf modules.
- **Build Tools**: `make`, `g++`, `qmake6`.

## Installation & Build

### 1. Install Dependencies
We provide a helper script for Debian-based systems (apt) and macOS (Homebrew) to check and install dependencies:

```bash
./install_dependencies.sh
```

### 2. Build the Application
```bash
make
```

This will compile the application into a standard `bin` directory (updated project config).

## Usage

Run the application:

```bash
./bin/app
```

*Note: For macOS, use `open build_macos/bin/app.app`.*

### Controls

| Key | Action |
| :--- | :--- |
| **Right** / **Down** / **Space** | Next Slide |
| **Left** / **Up** / **Backspace** | Previous Slide |
| **Home** / **End** | First / Last Slide |
| **L** | Toggle **Laser Pointer** |
| **Z** | Toggle **Zoom Pointer** |
| **N** | Reset Cursor (Normal) |
| **R** / **G** / **B** | Switch Laser Color (Red/Green/Blue) |
| **+** / **-** | Increase / Decrease Pointer Size |
| **S** | Toggle **Split View** (Beamer Mode) |
| **T** / **P** | Toggle Presentation Timer |
| **Q** / **Esc** | Exit Application |

### Modes
- **Standard Mode**: Assumes single-page slides. Notes are text-only.
- **Split View (Beamer Mode) ('S')**: Assumes double-width slides (common in LaTeX Beamer with notes).
    - **Left Half**: Projected to Audience.
    - **Right Half**: Shown in Presenter Console as Notes.

**Note**: To control the presentation, ensure the **Presenter Console** window has focus.

## Repository Structure

- `src/`: Source code (`main.cpp`, `mainwindow.cpp`, `presentationdisplay.cpp`).
- `include/`: Header files.
- `bin/`: Compiled executable (`app`).
- `screenshots/`: UI screenshots.
- `slides/`: Dummy presentation PDFs and generation scripts.
- `my_presenter.pro`: Qt project configuration.
- `Makefile`: Build wrapper script.