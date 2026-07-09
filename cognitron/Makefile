# Brain Seed Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O2 -lm -lpthread -D_GNU_SOURCE
LDFLAGS = -lm -lpthread

TARGET = brain_seed
SRC = brain_seed.c
HDR = brain_seed.h

all: $(TARGET)

$(TARGET): $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET)

run: all
	./$(TARGET)

.PHONY: all clean run
