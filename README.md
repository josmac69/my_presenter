# my_presenter

A robust, C++ based dual-screen PDF presentation tool built with **Qt 6**. Designed for professionals, it emulates the functionality of tools like *Pympress* but with the performance and native look-and-feel of C++.

It features a dedicated **Presenter Console** with wall-clock and elapsed timers, next-slide previews, and support for LaTeX Beamer notes (split-screen mode).

![Presenter Console](screenshots/console_features.png)

## Key Features

- **Dual-Window Architecture**:
    - **Audience View**: A distraction-free fullscreen window for the secondary display.
    - **Presenter Console**: A control center for the primary display containing:
        - Current Slide & Next Slide Preview.
        - **Timers**: Wall Clock and Elapsed Presentation Time.
        - **Notes View**: Supports standard text notes or visual "right-half" notes for Beamer split-slides.
        - **Table of Contents**: Clickable chapters extracted from PDF bookmarks.
- **Advanced Pointers**:
    - **Laser Pointer**: High-visibility dot with configurable color (Red, Green, Blue, **White**) and opacity.
    - **Zoom Pointer**: Magnifying glass effect for detailed diagrams.
    - **Drawing Mode**: Annotate slides directly on screen.
- **Screen Management**:
    - Automatic detection of secondary monitors.
    - **Intelligent Screen Swapping**: Easily switch which screen shows the audience view (`S` key).
    - **Split View Toggle**: Switch between standard single-page slides and double-width (Slide+Note) layouts (`Ctrl+S`).

## Installation & Build

### Prerequisites
- **Operating System**: Linux (Debian/Ubuntu recommended) or macOS.
- **Dependencies**: Qt 6 (Core, Gui, Widgets, Pdf, PdfWidgets), `make`, C++17 compiler (`g++` or `clang++`).

### Quick Start (Linux & macOS)

1. **Install Dependencies**:
   We provide a script to automatically verify and install missing packages for apt-based Linux and Homebrew-based macOS systems.
   ```bash
   ./install_dependencies.sh
   ```

2. **Build**:
   Compile the application.
   ```bash
   make
   ```
   *The binary will be created in the `bin/` directory.*

3. **Run**:
   ```bash
   ./bin/app
   ```
   *On macOS, you can also open the bundle: `open build_macos/bin/app.app`*

## Usage Guide

Launch the application and open a PDF presentation. The application will attempt to place the Audience View on a secondary monitor if detected.

### Control Reference

#### Navigation
| Key | Action |
| :--- | :--- |
| **Right** / **Space** / **Down** | Next Slide |
| **Left** / **Backspace** / **Up** | Previous Slide |
| **Home** | First Slide |
| **End** | Last Slide |
| **Page Up** | Previous Slide |
| **Page Down** | Next Slide |

#### Pointers & Tools
| Key | Action |
| :--- | :--- |
| **L** | Toggle **Laser Pointer** |
| **Z** | Toggle **Zoom (Magnify) Pointer** |
| **D** | Toggle **Drawing Mode** |
| **N** | **Normal Cursor** (Reset all tools) |
| **+** / **=** | **Increase** Pointer/Stroke Size |
| **-** | **Decrease** Pointer/Stroke Size |
| **R** | Set Laser/Pen to **Red** |
| **G** | Set Laser/Pen to **Green** |
| **B** | Set Laser/Pen to **Blue** |
| **W** | Set Laser/Pen to **White** |

#### View & App Controls
| Key | Action |
| :--- | :--- |
| **S** | **Switch Screens** (Swap Console/Audience Monitor) |
| **Ctrl + S** | Toggle **Split View** (Beamer Mode: Left=Slide, Right=Notes) |
| **T** / **P** | Toggle Presentation Timer (Start/Stop) |
| **Q** / **Esc** | Quit Application |

## Repository Structure

- `src/`: Source code (`mainwindow.cpp`, etc.).
- `include/`: Header files.
- `bin/`: Output directory for the executable.
- `slides/`: Example dummy presentations.
- `my_presenter.pro`: Qt 6 QMake project file.

---
*Created by [josmac69](https://github.com/josmac69).*