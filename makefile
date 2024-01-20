CC := gcc
CFLAGS := -g -Wall -pedantic -Werror -Wextra

# SOURCES := $(wildcard *.c) $(wildcard */*.c)
# SOURCES := event_logger.c \
# 	experiment_logger.c \
# 	fits_in_bits.c \
# 	mockup_flash/flash.c \
# 	mockup_flash/flash_using_driver.c \
# 	mockup_flash/mock_flash_driver.c

SOURCES := experiment_logger.c \
	fits_in_bits.c \
	mockup_flash/flash_using_driver.c \
	mockup_flash/mock_flash_driver.c

# SOURCES := fits_in_bits.c \
# 	mockup_flash/flash_using_driver.c \
# 	mockup_flash/mock_flash_driver.c

HEADER_DIR := ./

flash.out: $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o build/flash.out

list: $(SOURCES)
	echo "Sources:" $(SOURCES)

# flash.out1:
# 	echo $(SOURCES)