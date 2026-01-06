QT       += core gui widgets pdf pdfwidgets

TARGET   = app
TEMPLATE = app

# Enforce C++17
CONFIG  += c++17

# Output directory for the binary
DESTDIR  = bin

# Source files
SOURCES += src/main.cpp \
           src/mainwindow.cpp \
           src/presentationdisplay.cpp \
           src/screenselectorwidget.cpp

# Header files
HEADERS += include/mainwindow.h \
           include/presentationdisplay.h \
           include/screenselectorwidget.h

# Include paths
INCLUDEPATH += include

# Intermediate build files
OBJECTS_DIR = obj
MOC_DIR     = obj
