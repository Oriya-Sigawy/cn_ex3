#include "RUDP_API.h"
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h> // For the boolean signs flags
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

int wait_for_acknowledgement(int socket, int sequal_num, clock_t s);
int send_acknowledgement(int socket, RUDP *rudp);
int set_time(int socket, double time);
int seq_num = 0;

// we get this func in the assignment
unsigned short int calculate_checksum(RUDP *massage)
{
  int size = sizeof(RUDP);
  unsigned short int *data_pointer = (unsigned short int *)massage;
  unsigned int total_sum = 0;
  while (size > 1)
  {
    total_sum += *data_pointer++;
    size -= 2;
  }
  if (size > 0)
  {
    total_sum += *((unsigned char *)data_pointer);
  }
  while (total_sum >> 16)
  {
    total_sum = (total_sum & 0xFFFF) + (total_sum >> 16);
  }
  return (~(unsigned short int)total_sum);
}

int rudp_socket()
{
  int sock = socket(AF_INET, SOCK_DGRAM, 0); // Creating the socket in UDP format

  if (sock == -1)
  {
    perror("socket(2)");
    return -1;
  }
  return sock;
}

// only the sender use that func
int rudp_connect(int socket, const char *ip, int port)
{
  if (set_time(socket, 0.1) == -1)
  {
    return -1;
  }

  // Setting up the address in the socket struct
  struct sockaddr_in serveradd;
  memset(&serveradd, 0, sizeof(serveradd));

  serveradd.sin_family = AF_INET;   // for ipv4 address
  serveradd.sin_port = htons(port); // we're getting the port from the user

  int val = inet_pton(AF_INET, ip, &serveradd.sin_addr);

  if (val <= 0)
  {
    perror("inet_pton()");
    return -1;
  }

  if (connect(socket, (struct sockaddr *)&serveradd, sizeof(serveradd)) == -1)
  {
    perror("Connection()");
    return -1;
  }
  printf("connection succeed\n");
  RUDP *rudp = malloc(sizeof(RUDP)); // creating the struct and allocating memory for the data
  memset(rudp, 0, sizeof(RUDP));
  rudp->flags.isSynchronized = 1; // setting the syncronzie to 1
  int tries = 0;                  // checking how many tries we had for sending the data
  RUDP *recv = NULL;

  while (tries < 3) // we will try to send the data 3 times
  {
    int sendRes = sendto(socket, rudp, sizeof(RUDP), 0, NULL, 0); // using sendto of UDP protocol
    if (sendRes == -1)                                            // faild to sent the data
    {
      perror("send()");
      free(rudp); // freeing the allocated memory
      return -1;
    }

    // start measure the time
    clock_t start = clock();

    do
    {
      recv = malloc(sizeof(RUDP));
      memset(recv, 0, sizeof(RUDP));
      int get_data_res = recvfrom(socket, recv, sizeof(RUDP), 0, NULL, 0); // receiving ack on the last send

      if (get_data_res == -1) // means the we've failed getting the data
      {
        // perror("recvFrom()");
        // need to free both struct
        free(rudp);
        free(recv);
        return -1;
      }

      if (recv->flags.isSynchronized && recv->flags.acknowledgeFlag) // if they are both 1, we connected successfuly
      {
        printf("Are connected\n");
        free(rudp);
        free(recv);
        return 1;
      }
      else
      {
        printf("connection failed");
      }
    } while ((double)(clock() - start) / CLOCKS_PER_SEC < 1); // 1 second is our timeout
    tries++;                                                  // Incrementing the total tries number
  }

  printf("Couldn't connect");
  free(rudp);
  free(recv);
  return 0;
}

// only the receiver uses this func
int rudp_get_con(int socket, int port)
{
  struct sockaddr_in serveradd;
  memset(&serveradd, 0, sizeof(serveradd));

  // Putting the IP address an port to the socket
  serveradd.sin_family = AF_INET; // Ipv4
  serveradd.sin_port = htons(port);
  serveradd.sin_addr.s_addr = htonl(INADDR_ANY); // binding the socket to any available network

  int bind_res = bind(socket, (struct sockaddr *)&serveradd, sizeof(serveradd));

  if (bind_res == -1)
  {
    perror("bind()");
    return -1;
  }

  // receiving SYN
  struct sockaddr_in clientadd;
  socklen_t clientaddLen = sizeof(clientadd);
  memset((char *)&clientadd, 0, sizeof(clientadd));

  RUDP *rudp = malloc(sizeof(RUDP));
  memset(rudp, 0, sizeof(RUDP));

  int recv_length_bytes = recvfrom(socket, rudp, sizeof(RUDP), 0, (struct sockaddr *)&clientadd, &clientaddLen);

  if (recv_length_bytes == -1)
  {
    // perror("recvfrom()");
    free(rudp);
    return -1;
  }

  if (connect(socket, (struct sockaddr *)&clientadd, clientaddLen) == -1)
  {
    perror("connect()");
    free(rudp);
    return -1;
  }

  if (rudp->flags.isSynchronized == 1)
  {
    RUDP *reply = malloc(sizeof(RUDP));
    memset(rudp, 0, sizeof(RUDP));
    // Setting the flags to 1
    reply->flags.isSynchronized = 1;
    reply->flags.acknowledgeFlag = 1;

    int send_res = sendto(socket, reply, sizeof(RUDP), 0, NULL, 0);

    if (send_res == -1)
    {
      perror("sendto()");
      free(rudp);
      free(reply);
      return -1;
    }

    // set_time(socket, 1);
    free(rudp);
    free(reply);
    return 1; // connection succeed
  }
  return 0;
}

int rudp_send(int socket, const char *data, int length)
{
  // calculating the number of packets and last packet size
  int number_of_packets = length / MAX_SIZE; // the num of packets is the length of the data we want to send devide by the maximum size of each packet
  int last_one_size = length % MAX_SIZE;

  RUDP *rudp = malloc(sizeof(RUDP));
  for (int i = 0; i < number_of_packets; i++)
  {
    memset(rudp, 0, sizeof(RUDP));
    rudp->sequalNum = i;                                  // resenting which packet are we now
    rudp->flags.isDataPacket = 1;                         // the packet contains part of the data itself
    if (i == number_of_packets - 1 && last_one_size == 0) // if we send all the data
    {
      rudp->flags.finishFlag = 1;
    }
    // copying the data into the rudp struct, according to the place in the memory (data + i * MAX_SIZE) --> data[i]
    memcpy(rudp->data, data + i * MAX_SIZE, MAX_SIZE);
    rudp->dataLength = MAX_SIZE;
    rudp->checksum = 0;
    rudp->checksum = calculate_checksum(rudp);
    int num_of_tries = 0;
    do
    {
      num_of_tries++;
      int send_res = sendto(socket, rudp, sizeof(RUDP), 0, NULL, 0);
      if (send_res == -1)
      {
        perror("sendto()");
        return -1;
      }
    } while (wait_for_acknowledgement(socket, i, clock()) <= 0 && num_of_tries < 20);
  }

  if (last_one_size > 0)
  {
    memset(rudp, 0, sizeof(RUDP));
    rudp->sequalNum = number_of_packets; // last packet
    rudp->flags.isDataPacket = 1;        // Still sending data
    rudp->flags.finishFlag = 1;          // finishing sending the whole data

    memcpy(rudp->data, data + number_of_packets * MAX_SIZE, last_one_size); // copying the lats packet data to our struct,allocating only it's amount, don't need the whole MAX_SIZE, and accessing it by the pointer

    rudp->dataLength = last_one_size;
    rudp->checksum = 0;
    rudp->checksum = calculate_checksum(rudp);
    int num_of_tries = 0;
    do // Trying to send it again if needed
    {
      num_of_tries++;
      int send_last = sendto(socket, rudp, sizeof(RUDP), 0, NULL, 0);
      if (send_last == -1)
      {
        perror("sendto()");
        free(rudp);
        return -1;
      }
    } while (wait_for_acknowledgement(socket, number_of_packets, clock()) <= 0 && num_of_tries < 20);
    free(rudp);
  }
  return 1;
}

int rudp_receive(int socket, char **buffer, int *length)
{
  // initializing the rudp protocol packet income
  RUDP *rudp = malloc(sizeof(RUDP));
  memset(rudp, 0, sizeof(RUDP));
  int recv_len = recv(socket, rudp, sizeof(RUDP), 0);
  if (recv_len == -1)
  {
    // perror("recvfrom()");
    free(rudp);
    return -1;
  }
  if (calculate_checksum(rudp) != 0) // means we didn't get the whole info
  {
    free(rudp);
    return -1;
  }

  if (send_acknowledgement(socket, rudp) == -1)
  {
    free(rudp);
    return -1;
  }

  if (rudp->flags.isSynchronized == 1)
  {
    printf("have a request for connection\n");
    free(rudp);
    return 0;
  }

  if (rudp->sequalNum == seq_num)
  {
    if (rudp->sequalNum == 0 && rudp->flags.isDataPacket == 1)
    {
      set_time(socket, 5);
    }
    if (rudp->flags.finishFlag == 1 && rudp->flags.isDataPacket == 1) // means we handling the last packet now
    {
      memcpy(*buffer, rudp->data, rudp->dataLength);
      *length = rudp->dataLength;
      free(rudp);  // finished copying the last packet
      seq_num = 0; // reseting the sequal number for the next connection
      set_time(socket, 100);
      return 5; // returning a number for the end of the connection
    }

    if (rudp->flags.isDataPacket == 1) // this packet contains some of the wanted data
    {
      memcpy(*buffer, rudp->data, rudp->dataLength);
      *length = rudp->dataLength; // the length of the data
      free(rudp);
      seq_num++; // for the next packet tag
      return 1;
    }
  }

  else if (rudp->flags.isDataPacket == 1)
  {
    free(rudp);
    return 0;
  }

  if (rudp->flags.finishFlag == 1) // finished sending and closing the connection
  {
    free(rudp);

    // sending an ack and waiting to check if the sender is closed
    set_time(socket, 1);

    // means it's still open
    rudp = malloc(sizeof(RUDP));
    time_t finish_time = time(NULL);

    while ((time(NULL) - finish_time) < 1)
    {
      memset(rudp, 0, sizeof(RUDP));
      recv(socket, rudp, sizeof(RUDP), 0);
      if (rudp->flags.finishFlag == 1)
      {
        if (send_acknowledgement(socket, rudp) == -1)
        {
          free(rudp);
          return -1;
        }
        finish_time = time(NULL);
      }
    }

    free(rudp);
    return -5; // means sender closed the connection
  }
  free(rudp);
  return 0;
}

int rudp_close(int socket)
{
  RUDP *fin_massage = malloc(sizeof(RUDP));
  memset(fin_massage, 0, sizeof(RUDP));
  fin_massage->flags.finishFlag = 1; // Finished so closing the connection
  fin_massage->sequalNum = -1;
  fin_massage->checksum = 0;
  fin_massage->checksum = calculate_checksum(fin_massage);
  int num_of_tries = 0;
  do
  {
    num_of_tries++;
    int res_send = sendto(socket, fin_massage, sizeof(RUDP), 0, NULL, 0);
    if (res_send == -1)
    {
      perror("sendto()");
      free(fin_massage);
      return -1;
    }
  } while (wait_for_acknowledgement(socket, -1, clock()) <= 0 && num_of_tries < 20);
  close(socket);
  free(fin_massage);
  return 1;
}

int set_time(int socket, double time)
{
  // Setting for the socket a timeout, using timeval struct
  struct timeval timeout;
  timeout.tv_sec = (int)time;
  timeout.tv_usec = (time - timeout.tv_sec) * 1e6;

  if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
  {
    perror("setsockopt()");
    return -1;
  }
  return 1;
}

int wait_for_acknowledgement(int socket, int sequal_num, clock_t s)
{
  RUDP *reply = malloc(sizeof(RUDP));
  while ((double)(clock() - s) / CLOCKS_PER_SEC < 1)
  {
    int length_recv = recv(socket, reply, sizeof(RUDP), 0);
    if (length_recv == -1)
    {
      // perror("recvfrom()");
      free(reply);
      return -1;
    }
    if (reply->sequalNum == sequal_num && reply->flags.acknowledgeFlag == 1)
    {
      free(reply);
      return 1;
    }
  }
  free(reply);
  return 0;
}

int send_acknowledgement(int socket, RUDP *rudp)
{
  RUDP *rudp_ack = malloc(sizeof(RUDP));
  memset(rudp_ack, 0, sizeof(RUDP));
  rudp_ack->flags.acknowledgeFlag = 1;

  if (rudp->flags.finishFlag == 1) // finished to send the data
  {
    rudp_ack->flags.finishFlag = 1;
  }
  if (rudp->flags.isDataPacket == 1)
  {
    rudp_ack->flags.isDataPacket = 1;
  }
  rudp_ack->sequalNum = rudp->sequalNum;
  rudp_ack->checksum = 0;
  rudp_ack->checksum = calculate_checksum(rudp_ack);

  int res_send = sendto(socket, rudp_ack, sizeof(RUDP), 0, NULL, 0);
  if (res_send == -1)
  {
    perror("sendto()");
    free(rudp_ack);
    return -1;
  }

  free(rudp_ack);
  return 1;
}
