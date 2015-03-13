CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -Werror -g

BINS = gesture
OBJS = gesture.o

.PHONY: all clean

all: $(BINS)

clean:
	$(RM) $(BINS) $(OBJS)

gesture: gesture.o
gesture.o: gesture.h

gesture.o: CFLAGS+=$(shell pkg-config --cflags x11) $(shell pkg-config --cflags xft) $(shell pkg-config --cflags xi)
gesture: LDLIBS+=$(shell pkg-config --libs x11) $(shell pkg-config --libs xft) $(shell pkg-config --libs xi)
