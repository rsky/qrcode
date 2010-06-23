GDLIB_CONFIG ?= gdlib-config

MYOBJECTS += qrcnv_gd.o
DEFS += -DQR_ENABLE_GD

CPPFLAGS += -I`$(GDLIB_CONFIG) --includedir`
LDFLAGS += -L`$(GDLIB_CONFIG) --libdir`
LIBS += -lgd
