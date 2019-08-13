include config.mk

CFLAGS = -g -Wall -Wextra -Wpedantic -Werror -Wno-unused-function -O3
LDFLAGS = -g
override CFLAGS += -std=c99 $(shell pkg-config --cflags x11) $(shell pkg-config --cflags xi)
override LDLIBS += $(shell pkg-config --libs x11) $(shell pkg-config --libs xi) -lm

ifneq ($(XFT_TEXT),)
	override CFLAGS += -DXFT_TEXT $(shell pkg-config --cflags xft)
	override LDLIBS += $(shell pkg-config --libs xft)
endif

BINS = charade
OBJS = charade.o geometry.o

.PHONY: all clean

all: $(BINS)

clean:
	$(RM) $(BINS) $(OBJS)

charade: charade.o geometry.o

charade.o: charade.h geometry.h

geometry.o: geometry.h
