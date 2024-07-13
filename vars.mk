#    ___          _        _    __   __           
#   | _ \_ _ ___ (_)___ __| |_  \ \ / /_ _ _ _ ___
#   |  _/ '_/ _ \| / -_) _|  _|  \ V / _` | '_(_-<
#   |_| |_| \___// \___\__|\__|   \_/\__,_|_| /__/
#              |__/                               
PROJECT=pt
TARGET=$(shell ${CC} -dumpmachine)

SRC_OBJS=$(patsubst $(ROOT)/src/%.c,$(ROOT)/obj/$(TARGET)/%.c.o,$(wildcard $(ROOT)/src/*.c))
INC+=-I$(ROOT)/inc -I $(ROOT)/gitman_sources/xmath.h/inc
LIB+=
CFLAGS+=-Wall -g -std=c++17 -O3
#-ffast-math
LINK+=-lm
