#include <arpa/inet.h> // For the in_addr structure and the inet_pton function
#include <stdbool.h>   // For the boolean signs flags
#include <stdio.h>
#include <stdlib.h>
#include <string.h>     // For the memset function
#include <sys/socket.h> // For the socket function
#include <sys/time.h>   // For tv struct
#include <time.h>
#include <unistd.h> // For the close function

#include "RUDP_API.h"

#define DATA_SIZE 2097152 // 2mb

int connection(char *ip, int port);
char *util_generate_random_data(unsigned int size);

int main(int argc, char *argv[])
{
  char *RECEIVER_IP_ADDRESS;
  int RECEIVER_PORT;

  for (int i = 0; i < argc; i++) // this loop receives the sender ip, port
  {
    if (!strcmp(argv[i], "-p"))
    {
      RECEIVER_PORT = atoi(argv[i + 1]);
    }
    if (!strcmp(argv[i], "-ip"))
    {
      RECEIVER_IP_ADDRESS = argv[i + 1];
    }
  }
  char *random_data = util_generate_random_data(DATA_SIZE); // generating 2mb data

  int socket = connection(RECEIVER_IP_ADDRESS, RECEIVER_PORT);

  if (socket == -1)
  {
    return 1; // faild to create the socket
  }

  char choice;
  int byteSent;

  do
  {
    printf("sending the file...\n");
    byteSent = rudp_send(socket, random_data, DATA_SIZE);
    if (byteSent == 0)
    {
      printf("Error sending the data...\n");
      rudp_close(socket);
      return 1;
    }
    printf("file was successfuly sent.\n Do you want to send it again? (y/n): \n");
    choice = getchar(); // receive the user's choise
    while (choice != 'n' && choice != 'y')
    {
      choice = getchar();
    }
  } while (choice != 'n');

  printf("Closing the connection...\n");
  rudp_close(socket);

  printf("Connection closed\n");
  free(random_data);

  return 0;
}

int connection(char *ip, int port)
{
  int socket = rudp_socket(); // creating the socket
  if (socket == -1)
  {
    return -1; // returning error
  }
  if (rudp_connect(socket, ip, port) <= 0)
  {
    close(socket);
    return -1;
  }
  return socket;
}

char *util_generate_random_data(unsigned int size)
{
  char *buffer = NULL;
  // Argument check.
  if (size == 0)
    return NULL;
  buffer = (char *)calloc(size, sizeof(char));
  // Error checking.
  if (buffer == NULL)
    return NULL;
  // Randomize the seed of the random number generator.
  srand(time(NULL));
  for (unsigned int i = 0; i < size; i++)
    *(buffer + i) = ((unsigned int)rand() % 255) + 1;
  return buffer;
}