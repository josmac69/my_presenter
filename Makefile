# Wrapper Makefile for qmake build

# Detect qmake6 or qmake
QMAKE := $(shell command -v qmake6 2> /dev/null)
ifndef QMAKE
    QMAKE := $(shell command -v qmake 2> /dev/null)
endif

all: check_qmake
	$(QMAKE) -o Makefile.qt my_presenter.pro
	$(MAKE) -f Makefile.qt

clean: check_qmake
	-$(MAKE) -f Makefile.qt clean
	rm -f Makefile.qt
	rm -rf bin obj

check_qmake:
ifndef QMAKE
	$(error "qmake not found. Please install qt6-base-dev and qt6-pdf-dev")
endif

.PHONY: all clean check_qmake
