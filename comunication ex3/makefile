CC = gcc
CFLAG = -Wall -g
AR = ar
AFLAGS = rcs

.PHONY: all clean

all: RUDP_Sender RUDP_Receiver

RUDP_Receiver: RUDP_Receiver.o RUDP_API.a
	$(CC) $(CFLAGS) $^ -o $@

RUDP_Receiver.o: RUDP_Receiver.c RUDP_API.h
	$(CC) $(CFLAGS) -c $<

RUDP_Sender: RUDP_Sender.o RUDP_API.a
	$(CC) $(CFLAGS) $^ -o $@

RUDP_Sender.o: RUDP_Sender.c RUDP_API.h
	$(CC) $(CFLAGS) -c $<

# Creating a library for the API
RUDP_API.a: RUDP_API.o
	$(AR) $(AFLAGS) $@ $<

RUDP_API.o: RUDP_API.c RUDP_API.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o *.a RUDP_Sender RUDP_Receiver