# my_presenter

This repository contains a C++ Qt project.

## Repository Structure

- `src/`: Contains source code files (`.cpp`).
- `include/`: Contains header files (`.h`, `.hpp`).
- `obj/`: Used for intermediate object files (`.o`, `.moc`, etc.) generated during the build process.
- `bin/`: Contains the final executable binaries.
- `Makefile`: The build wrapper script.
- `my_presenter.pro`: The Qt project configuration file.

## Prerequisites

- **Qt Development Libraries**: This project requires Qt 6.
  - Specifically: `Widgets`, `Gui`, `Core`, and `Pdf` modules.
  - On Debian/Ubuntu: `sudo apt install qt6-base-dev qt6-pdf-dev`
- **qmake6**: The build system relies on `qmake6` (usually included in `qt6-base-dev` or `qt6-base-dev-tools`).

## Makefile Usage

The `Makefile` wraps `qmake` to simplify the build process.

- `make` or `make all`: Generates the Makefile using `qmake6`, compiles the source files, runs `moc`, and links the executable in `bin/app`.
- `make clean`: Removes the `obj` and `bin` directories and temporary build files.

To run the application after building:
```bash
./bin/app
```