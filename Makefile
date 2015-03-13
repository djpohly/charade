CFLAGS = -g -std=c99 -Wall -Wextra -Wpedantic -Werror -Wno-unused-function

BINS = charade
OBJS = charade.o

.PHONY: all clean

all: $(BINS)

clean:
	$(RM) $(BINS) $(OBJS)

charade: charade.o
charade: LDLIBS+=$(shell pkg-config --libs x11) $(shell pkg-config --libs xft) $(shell pkg-config --libs xi) -lm

charade.o: charade.h
charade.o: CFLAGS+=$(shell pkg-config --cflags x11) $(shell pkg-config --cflags xft) $(shell pkg-config --cflags xi)
