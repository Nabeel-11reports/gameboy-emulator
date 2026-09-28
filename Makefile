CC ?= gcc
PKG_CONFIG ?= pkg-config
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2 -Isrc
SDL_CFLAGS := $(shell $(PKG_CONFIG) --cflags sdl2)
SDL_LIBS := $(shell $(PKG_CONFIG) --libs sdl2)

SRC := $(wildcard src/*.c)
TARGET ?= gameboy

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(SRC) -o $@ $(SDL_LIBS) -lm

clean:
	rm -f $(TARGET) gameboy.exe
