CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c2x # -O3
LIBS = -lcheck -lsubunit -lm -lpthread -lrt
TARGET = test_runner

SRC = tests.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) *.o

.PHONY: all run clean