CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
LDFLAGS = -lreadline

TARGET = shellforge

SRC = src/main.c src/lexer.c src/token.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
