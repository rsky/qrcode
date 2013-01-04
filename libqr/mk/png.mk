ZLIB_PREFIX ?= /usr/local

MYOBJECTS += qrcnv_png.o
DEFS += -DQR_ENABLE_PNG

CPPFLAGS += -I$(ZLIB_PREFIX)/include
LDFLAGS += -L$(ZLIB_PREFIX)/lib
LIBS += -lz
