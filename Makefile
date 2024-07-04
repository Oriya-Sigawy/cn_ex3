CFLAGS= -Wall -g
CC = gcc
.PHONY: all clean

all: TCP_Receiver TCP_Sender

TCP_Receiver: TCP_Receiver.c
	$(CC) $(CFLAGS) $^ -o $@

TCP_Sender: TCP_Sender.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f *.o TCP_Receiver TCP_Sender
