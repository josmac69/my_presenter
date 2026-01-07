# Wrapper Makefile for qmake build

# Detect qmake6 or qmake
QMAKE := $(shell command -v qmake6 2> /dev/null)
ifndef QMAKE
    QMAKE := $(shell command -v qmake 2> /dev/null)
endif

# Detect OS
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    BUILD_DIR := build_macos
else
    BUILD_DIR := build_linux
endif

all: check_qmake
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && $(QMAKE) ../my_presenter.pro
	cd $(BUILD_DIR) && $(MAKE)

clean:
	rm -rf $(BUILD_DIR)

check_qmake:
ifndef QMAKE
	$(error "qmake not found. Please install qt6-base-dev and qt6-pdf-dev")
endif

.PHONY: all clean check_qmake
