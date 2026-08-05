CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude

SRC := $(wildcard src/*.c)
TARGET = shellforge

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -lreadline -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
