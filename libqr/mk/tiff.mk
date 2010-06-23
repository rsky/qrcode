ZLIB_PREFIX ?= /usr/local

MYOBJECTS += qrcnv_tiff.o
DEFS += -DQR_ENABLE_TIFF

CPPFLAGS += -I$(ZLIB_PREFIX)/include
LDFLAGS += -L$(ZLIB_PREFIX)/lib
LIBS += -lz
