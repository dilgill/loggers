CC := gcc
CFLAGS := -g -Wall -pedantic -Werror -Wextra

SOURCES := $(wildcard *.c) $(wildcard */*.c)
HEADER_DIR := ./

flash.out: $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o build/flash.out

# flash.out1:
# 	echo $(SOURCES)