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
SRC = src/generator/window.c src/generator/program.c src/generator/countriesMask.c src/BGR.c src/bmpLoader.c
E_SRC = src/editor/provinceEditor.c
EDGE_SRC = src/edges/edgeDetection.c src/BGR.c src/bmpLoader.c

.PHONY: all clean

all: province edge editor

province:
	$(CC) $(CFLAGS) $(SRC) -o $@$(EXT) -Llib/ $(LIBS)
	$(STRIP) $@$(EXT)

edge:
	$(CC) -Isrc/ $(EDGE_SRC) -o $@$(EXT)
	$(STRIP) $@$(EXT)

editor:
	$(CC) $(CFLAGS) $(E_SRC) -o $@$(EXT) -Llib/ $(LIBS)

clean:
	rm -f province$(EXT) edge$(EXT)