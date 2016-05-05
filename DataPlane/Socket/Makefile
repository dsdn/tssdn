# Makefile for the socket programming using C

CC = g++
CFLAGS = -O3

SOURCES=sender_c.c receiver_c.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=sender_c receiver_c

all: $(EXECUTABLE)

sender_c: sender_c.o
	$(CC) $(CFLAGS) sender_c.o -o $@

receiver_c: receiver_c.o
	$(CC) $(CFLAGS) receiver_c.o -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< 

.PHONY: clean

clean:
	rm -f *.o $(EXECUTABLE)
