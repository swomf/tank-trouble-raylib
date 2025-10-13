CC = gcc
CFLAGS = -Wall -std=c99 -O2 -Iinclude
LIBS = -lraylib -lm -lpthread -ldl -lrt -lX11

SRC = src/main.c
OBJ = $(SRC:.c=.o)

TARGET = tanktrouble

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)
