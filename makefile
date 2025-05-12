UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
$(info Linux system)
	EXT =
	LIBS = -lraylib
	STRIP = strip
else ifeq ($(UNAME_S),Darwin)
$(info Mac system)
	EXT =
	LIBS = -lraylib
	STRIP = strip
else
$(info Windows system)
	EXT = .exe
	LIBS = -lraylib.dll
	STRIP = strip
endif

CC = gcc
CFLAGS = -Iinclude/
SRC = src/window.c src/program.c src/countriesMask.c src/BGR.c src/bmpLoader.c
EDGE_SRC = src/edgeDetection.c src/BGR.c src/bmpLoader.c

.PHONY: all clean

all: province edge

province:
	$(CC) $(CFLAGS) $(SRC) -o $@$(EXT) -Llib/ $(LIBS)
	$(STRIP) $@$(EXT)

edge:
	$(CC) -Isrc/ $(EDGE_SRC) -o $@$(EXT)
	$(STRIP) $@$(EXT)

clean:
	rm -f province$(EXT) edge$(EXT)