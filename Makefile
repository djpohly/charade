CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -Werror -g

BINS = gesture

.PHONY: all clean

all: $(BINS)

clean:
	$(RM) $(BINS)

gesture: gesture.h

gesture: CFLAGS+=$(shell pkg-config --cflags x11) $(shell pkg-config --cflags xft) $(shell pkg-config --cflags xi)
gesture: LDLIBS+=$(shell pkg-config --libs x11) $(shell pkg-config --libs xft) $(shell pkg-config --libs xi)
