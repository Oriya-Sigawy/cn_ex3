#include <arpa/inet.h>  // For the in_addr structure and the inet_pton function
#include <stdbool.h>    // For the boolean signs flags
#include <stdio.h>
#include <stdlib.h>
#include <string.h>      // For the memset function
#include <sys/socket.h>  // For the socket function
#include <sys/time.h>    // For tv struct
#include <time.h>
#include <unistd.h>  // For the close function

#include "RUDP_API.h"

#define PORT 5061  // may be changed, temporary port for now
#define SERVER_IP "127.0.0.1"
#define DATA_SIZE 2097152  // 2mb

/***Will act as an client***/

int parse_info(int argc, char *argv[], char **ip, int *port);
int parse_port(char *port);
int connection(char *ip, int port);
char *util_generate_random_data(
    unsigned int size);  // their function for generating data

int main(int argc, char *argv[]) {
  char *ip;
  int port;

  if (!parse_info(argc, argv, &ip, &port)) {
    return 1;  // in main 1 means jumping out from the program, didn't succeed
  }

  char *random_data =
      util_generate_random_data(DATA_SIZE);  // generating 2mb data

  int socket = connection(ip, port);

  if (socket == -1) {
    return 1;  // faild to create the socket
  }

  char choice;

  do {
    printf("sending the file...\n");
    if (rudp_send(socket, random_data, DATA_SIZE) < 0) {
      printf("Error sending the data...\n");
      rudp_close(socket);  // closing the connection
      return 1;
    }
    printf("Do you want to send it again? (y/n): \n");
    scanf(" %c", &choice);  // The space before %c is to skip the white spaces
  } while (choice == 'y');

  printf("Closing the connection...\n");
  rudp_close(socket);

  printf("Connection closed\n");
  free(random_data);

  return 0;
}

int connection(char *ip, int port) {
  int socket = rudp_socket();  // creating the socket
  if (socket == -1) {
    return -1;  // returning error
  }
  if (rudp_connect(socket, ip, port) <= 0) {
    return -1;
  }
  return socket;
}

int parse_port(char *port) {
  char *endpoint;
  long int portNumer =
      strtol(port, &endpoint, 10); /* Convert a string to a long integer.  */

  if (*endpoint != '\0' || portNumer < 0 ||
      portNumer > 65535) {  // if the end of the string is not '\0',
                            // or the port number is to big or negative,
                            // means it's not between the range of port numbers
    return -1;
  }
  return (int)portNumer;
}

// checkong the command input correctness
int parse_info(int argc, char *argv[], char **ip, int *port) {
  if (argc != 5 || strcmp(argv[1], "-ip") != 0 && strcmp(argv[3], "-p") != 0) {
    printf("Invalid command input\n");
    return 0;  // for failed
  }

  *ip = argv[2];
  *port = parse_port(argv[4]);
  if (*port == -1) {
    printf("Invalid port\n");
    return 0;  // for failed
  }
  return 1;  // success parsing the commant to ip address and port number
}

char *util_generate_random_data(unsigned int size) {
  char *buffer = NULL;
  // Argument check.
  if (size == 0) return NULL;
  buffer = (char *)calloc(size, sizeof(char));
  // Error checking.
  if (buffer == NULL) return NULL;
  // Randomize the seed of the random number generator.
  srand(time(NULL));
  for (unsigned int i = 0; i < size; i++)
    *(buffer + i) = ((unsigned int)rand() % 255) + 1;
  return buffer;
}