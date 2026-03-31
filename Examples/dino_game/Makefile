# Makefile for T-Rex Dino Game (SDL2 PC simulator)
#
# Usage:
#   make          — build the SDL2 simulator
#   make run      — build and run
#   make clean    — remove build artifacts

CC      ?= gcc
CFLAGS  := -Wall -Wextra -O2 -std=c99
LDFLAGS := -lSDL2 -lm

TARGET  := dino_sdl
SRCS    := main_sdl.c dino_game.c
OBJS    := $(SRCS:.c=.o)

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c dino_game.h
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)
