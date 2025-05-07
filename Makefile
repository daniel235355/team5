CC = gcc
CFLAGS = -Wall -Werror -g -I./include
SRC_DIR = src
TARGET = image_processor

SRCS = main.c $(SRC_DIR)/image.c $(SRC_DIR)/sobel.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
