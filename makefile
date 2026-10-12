kCC = gcc

CFLAGS = -Wall -Wextra -Iinclude

LDFLAGS = -lreadline

TARGET = shellforge

OBJS = src/main.o \
       src/token.o \
       src/lexer.o \
       src/parser.o \
       src/expand.o \
       src/history.o \
       src/builtin.o \
       src/executor.o


all: $(TARGET)


$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)


src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f $(OBJS) $(TARGET)
