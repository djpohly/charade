CFLAGS = -g -Wall -Wextra -Wpedantic -Werror -Wno-unused-function -O3
override CFLAGS += -std=c99 $(shell pkg-config --cflags x11) $(shell pkg-config --cflags xft) $(shell pkg-config --cflags xi)
override LDLIBS += $(shell pkg-config --libs x11) $(shell pkg-config --libs xft) $(shell pkg-config --libs xi) -lm

BINS = charade
OBJS = charade.o geometry.o

.PHONY: all clean

all: $(BINS)

clean:
	$(RM) $(BINS) $(OBJS)

charade: charade.o geometry.o

charade.o: charade.h geometry.h

geometry.o: geometry.h
