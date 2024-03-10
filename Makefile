CFLAGS= -Wall -g

.PHONY: all clean

all: TCP_Receiver TCP_Sender

TCP_Receiver: TCP_Receiver.c
	gcc -o $(CFLAGS) TCP_Receiver TCP_Receiver.c

TCP_Sender: TCP_Sender.c
	gcc -o $(CFLAGS) TCP_Sender TCP_Sender.c

clean:
	rm -f *.o TCP_Receiver TCP_Sender
