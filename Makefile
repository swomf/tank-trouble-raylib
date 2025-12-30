CFLAGS = -Wall -std=c99 -O2 -pedantic
LDLIBS = -lraylib -lm -lpthread -ldl -lrt -lX11

SRC = src/main.c
OBJ = $(SRC:.c=.o)
TARGET = tanktrouble

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

$(OBJ): src/config.h src/types.h

run: $(TARGET)
	./$(TARGET)

clean:
	$(RM) -f $(OBJ) $(TARGET)

.PHONY: all clean run
