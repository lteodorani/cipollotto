CC=gcc
DEBUG_DIR=./build/debug
DEBUG_BIN=$(DEBUG_DIR)/cipollotto
debug: main.c ui_SDL.c cipollotto.c
	[ -d $(DEBUG_DIR) ] || mkdir -p $(DEBUG_DIR)
	$(CC) main.c ui_SDL.c cipollotto.c -g -O3 -Wall -lSDL3 -o $(DEBUG_BIN)