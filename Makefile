CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g

TARGET = memory_sim
SRCS = main.c memory_manager.c evaluation.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c memory_manager.h evaluation.h
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all run clean
