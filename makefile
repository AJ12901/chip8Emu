# The flags will give us all, extra error warnings
# -I tells the compiler to look for header files in the include directory and -L tells it to look for linker files in the lib directory

#MAC MAKEFILE BEGIN

# CFLAGS=-std=c17 -Wall -Wextra

# .PHONY: c8

# c8:
# 	clang chip8.c -o c8 -I include -L lib -l SDL2-2.0.0 $(CFLAGS)

#MAC MAKEFILE END

CFLAGS=-std=c17 -Wall -Wextra

all:
	gcc chip8Emu.c -o chip8Emu $(CFLAGS) `sdl2-config --cflags --libs`
