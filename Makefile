CC = gcc
CFLAGS = -Wall -std=c99 -O2 -Iinclude
LIBS = -lraylib -lm -lpthread -ldl -lrt -lX11

SRC = src/main.c
OBJ = $(SRC:.c=.o)
CONFIG = src/config.h

TARGET = tanktrouble

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LIBS)

$(OBJ): $(CONFIG)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)
