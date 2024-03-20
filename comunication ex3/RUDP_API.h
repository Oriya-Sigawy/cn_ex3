
#ifndef RUDP_API_H
#define RUDP_API_H
#define MAX_SIZE 60000 // will be for max size of data each time    

#include <stdbool.h> // For the boolean signs flags
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Creating the socket
 * Returns a file descriptor for the new socket (from UDP func that creates a socket), or -1 if fails.
 **/
int rudp_socket();

/**
 * sending the data
 *  returning 1 if succeeds 0 if fails
It will wait for acknowledgment packet
**/
int rudp_send(int socket, const char *data, int length);

/*
Receiving the data through the conncetion.
returning 1 if succeeds 0 if fails
the func gets the data's length and the data itself, and puts the data in a buffer.
*/
int rudp_receive(int socket, char **buffer, int *length);

/*
closes the connection
returns -1 if the fails, 1 if succeeds.
*/
int rudp_close(int socket);

/*
the func opens the connection
returns 0 if fails, -1 if one of the UDP func we are using in failes, 1 if connection succeeded
*/
int rudp_connect(int socket, const char *ip, int port);

/**
Listening for incoming connection
returning 1 if succeeds 0 if fails
*/
int rudp_get_con(int socket, int port);

// We're using boolean signs flag, their value is 1 for true and 0 for false,
// That will help us to know for each packet what's the state of the connection

struct Flags
{
  bool isSynchronized; // Synchronize flag to know if someone want to establish connection

  bool finishFlag; // To know if we've finished to send everything and can end the connection

  bool acknowledgeFlag; // Acknowledge to ensure that we've got the whole data

  bool isDataPacket; // To know if the packet is carrying a data payload
};

// This struct will represent a UDP packet, with the flags and the data
typedef struct _RUDP
{
  struct Flags flags;
  int sequalNum;
  short checksum;
  int dataLength;
  char data[MAX_SIZE];
} RUDP;

unsigned short int calculate_checksum(RUDP *massage);

#endif