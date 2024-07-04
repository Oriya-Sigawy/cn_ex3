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

#define SIZE_DATA 2097152

/*
this is a struct for 2 arrays in a dynamic size.
times- array that save in every cell i the sending time in send i+1
speeds- array that save in every cell i the sending speed in send i+1
size- number of sends (full cells)
capacity- number of cells in every array (including the one we hav'nt used yet)
*/
typedef struct
{
  double *times;
  double *speeds;
  int size;
  int capacity;
} DynamicArray;

/**
 * this func is used to add data to the speeds and times arrays
 * the func checks if the size of darr is  equal to its capacity (meaning that the arrays are full).
 *  if it is, the func doubles the capacity and add the data, and if it isn't, the func just add the data.
 *
 * darr- variable of type dynamicArray (the struct above)
 * time- the data we want to add to darr's times array
 * speed- the data we want to add to darr's speeds array
 */
void AddToDArr(DynamicArray *darr, double time, double speed)
{
  if (darr->size >= darr->capacity)
  {
    darr->capacity *= 2;
    darr->times = (double *)realloc(darr->times, darr->capacity * sizeof(double));
    darr->speeds = (double *)realloc(darr->speeds, darr->capacity * sizeof(double));
  }
  darr->times[darr->size] = time;
  darr->speeds[darr->size] = speed;
  darr->size++;
}
/*
this func frees all the memory was used for the dynamic array.
*/
void free_arr(DynamicArray *arr)
{
  free(arr->speeds);
  free(arr->times);
  free(arr);
}

/**
 * This func calculates and prints requaired times about the times that the receiver saved during his connection with the sender.
 * times: array of the saved times, one cell for every time that the sender sended the information.
 * numOfSends: the number of sends that the senders sended the information
 * fSize: the size of the sended file
 */
void print_times(DynamicArray *arr)
{
  double times_sum = 0;
  double avg = 0;
  double bandwidth = 0;
  double speeds_sum = 0;
  printf("* statistics: *\n");
  for (int i = 0; i < arr->size; i++)
  {
    times_sum += arr->times[i];
    speeds_sum += arr->speeds[i];
    printf("Run #%d Data: Time=%fms, Speed=%f MB/s\n", i + 1, arr->times[i], arr->speeds[i]);
  }
  if (arr->size > 0)
  {
    avg = (times_sum / (double)arr->size);
    bandwidth = (speeds_sum / (double)arr->size);
  }

  printf("Average time: %0.3lf ms\n", avg);
  printf("Average speed: %0.3lf MB/s\n", bandwidth);
}

/**
 * this func converts bytes to mega bytes
 * bytes: number of bytes to convert
 * returns: number of bytes in mega bytes
 */
int byteToMegabyte(int bytes)
{
  return bytes / (1024 * 1024);
}

int main(int argc, char *argv[])
{
  // save the receiver PORT
  int RECEIVER_PORT;
  for (int i = 0; i < argc; i++) // //this loop receives the receiver port
  {
    if (!strcmp(argv[i], "-p"))
    {
      RECEIVER_PORT = atoi(argv[i + 1]);
    }
  }
  // create dynamicArray for this connection
  DynamicArray *times_arr = malloc(sizeof(DynamicArray));
  times_arr->size = 0;     // no data exists yet
  times_arr->capacity = 1; // ready to get one set of data (time and speed), will be increased if necessary
  times_arr->times = (double *)malloc(sizeof(double));
  times_arr->speeds = (double *)malloc(sizeof(double));

  printf("Starting up reciever\n");
  int socket = rudp_socket();
  int receive_state = rudp_get_con(socket, RECEIVER_PORT); // getting the connection

  if (receive_state == 0)
  {
    printf("Couldn't get a connection\n");
    return -1;
  }
  if (receive_state == -1)
  {
    close(socket);
    return -1;
  }
  char *buff = NULL; // to save the sender's info file

  // prepering buff to receive the data
  buff = malloc(sizeof(RUDP));
  if (buff == NULL)
  {
    printf("malloc failed\n");
    exit(1);
  }
  // to calculate the send time
  clock_t start;
  clock_t end;
  do
  {
    printf("getting the file data...\n");
    start = clock(); // start measure the time
    memset(buff, 0, sizeof(RUDP));
    int data_len = 0;
    int received = 0;
    while (received <= SIZE_DATA) // check if the intire file received or if received a FIN massage from the user
    {                             // the info may come in segments, save the num of bytes received in every segment
      receive_state = rudp_receive(socket, &buff, &data_len);
      received += data_len;

      if (receive_state == -5) // means sender closed the connection
      {
        break;
      }
      else if (receive_state == -1)
      {
        printf("Error receiving the data\n");
        close(socket);
        return -1;
      }
    }
    if (receive_state == -5) // means sender closed the connection
    {
      break;
    }
    end = clock(); // stop measure the time
    printf("File transfer complete\n");
    // save the calculations on our dynamicArr
    int mb = byteToMegabyte(received);
    double t_in_s = ((double)(end - start) / CLOCKS_PER_SEC);
    AddToDArr(times_arr, t_in_s * 1000, mb / t_in_s);
  } while (receive_state != -5);

  printf("Receiver closing the connection...\n");
  close(socket);
  // print the calculations
  print_times(times_arr);
  // freeing the using memory
  free(buff);
  free_arr(times_arr);
  return 0;
}
