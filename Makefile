#MAC MAKEFILE BEGIN

# CFLAGS=-std=c17 -Wall -Wextra

# .PHONY: c8

# c8:
# 	clang chip8.c -o c8 -I include -L lib -l SDL2-2.0.0 $(CFLAGS)

#MAC MAKEFILE END

CFLAGS=-std=c17 -Wall -Wextra

all:
	gcc chip8Emu_emulation.c chip8Emu_initialization.c chip8Emu.c -o build/chip8Emu $(CFLAGS) `sdl2-config --cflags --libs` -I/usr/include/SDL2 -lSDL2_ttf
