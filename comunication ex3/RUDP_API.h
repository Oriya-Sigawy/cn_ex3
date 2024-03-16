// A header file for RUDP APIvfunctions

#ifndef RUDP_API_H
#define RUDP_API_H
#define MAX_SIZE 60000  // will be for max size of data each time

#include <stdbool.h>  // For the boolean signs flags
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Creating the socket, return -1 if fails
int rudp_socket();

// Sending the data, returning 1 if succedd 0 if fails
// It'll wait fir some of acknowledgment packet for nowing what the state
int rudp_send(int socket, const char *data, int length);

// Receiving the data from the conncetion returning 1 if succedd 0 if fails
// Putting the data in the char buffern and getting also the length
int rudp_receive(int socket, char **buffer, int *length);

// Close the socket
int rudp_close(int socket);

/**** More help functions ****/
// The idea to make UDP reliable, is to make it like TCP protocol, adding like
// listen and bind functions

// Opening the connection itself between with the socket and the given ip and
// port This is the 2 handshake, returning 1 if succedd 0 if fails
int rudp_connect(int socket, const char *ip, int port);

// Listening for incoming connection, the other side of the 2 handshake
// returning 1 if succedd 0 if fails
int rudp_get_con(int socket, int port);

/*****Helpers structs for checking the data transfer and for the protocol itself
 * ****/

// We're using here boolean signs flag, we'll have 1 for true and 0 for false,
// That will help us to know for each packet what's the state of the connection

struct Flags {
  bool isSynchronized;  // Synchronize flag to know if someone want to establish
                        // // connection

  bool finishFlag;  // To know if we've finished to send everything and can
                    // finish the connection

  bool acknowledgeFlag;  // Acknowledge this for ensure that we've got the whole
                         // data

  bool isDataPacket;  // To know if the packet is carrying a data payload
};

// This struct will represent a UDP packet, with the flags and the data
typedef struct _RUDP {
  struct Flags flags;
  int sequalNum;
  int checksum;
  int dataLength;
  char data[MAX_SIZE];
} RUDP;

#endif