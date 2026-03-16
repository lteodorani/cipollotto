#common
CC=/usr/bin/gcc
C_VERSION=-std=gnu17

SRC_DIR=src
BUILD_DIR=build

#debug
DEBUG_DIR=$(BUILD_DIR)/debug
DEBUG_BIN=$(DEBUG_DIR)/cipollotto
DEBUG_CFLAGS=$(C_VERSION) -g -O0 -Wall -lSDL3

#release
RELEASE_DIR=$(BUILD_DIR)/release
RELEASE_BIN=$(RELEASE_DIR)/cipollotto
RELEASE_CFLAGS=$(C_VERSION) -O3 -Wall -Wextra -lSDL3


#source files
SRC=$(SRC_DIR)/main.c \
    $(SRC_DIR)/ui_SDL.c \
    $(SRC_DIR)/cipollotto.c


#make targets
debug: $(SRC)
	mkdir -p $(DEBUG_DIR)
	$(CC) $(SRC) $(DEBUG_CFLAGS) -o $(DEBUG_BIN)

release: $(SRC)
	mkdir -p $(RELEASE_DIR)
	$(CC) $(SRC) $(RELEASE_CFLAGS) -o $(RELEASE_BIN)
