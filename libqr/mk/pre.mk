INSTALL_PREFIX ?= /usr/local
DESTDIR ?=
BINDIR ?= $(INSTALL_PREFIX)/bin
LIBDIR ?= $(INSTALL_PREFIX)/lib
INCDIR ?= $(INSTALL_PREFIX)/include

CC ?= gcc
CFLAGS += -O2 -g -Wall

VMAJOR := 0
VMINOR := 0
VPATCH := 0
LIBVERSION := $(VMAJOR).$(VMINOR).$(VPATCH)

MYOBJECTS := qr.o qrcnv.o qrcnv_bmp.o qrcnv_svg.o
MYHEADERS := qr.h qr_util.h
MYLIBNAME := libqr

all: qr qrs $(MYLIBNAME).a

# If you don't want to compile with TIFF support, comment out next line.
include mk/tiff.mk

# If you don't want to compile with GD support, comment out next line.
include mk/gd.mk
