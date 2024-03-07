/**
 * this file represents a receiver that receive files from the sender
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

//two options of congestion control algoritms
char *reno = "reno";
char *cubic = "cubic";
char *algoritm;     //represents the algoritm we are using

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
 *  This func gets the information from the sender.
 *  sender_sock: the sender's socket descriptor (representing the socket's endpoint)
 *  buf: will contain the accapted information
 *  len: length of the information
 */
int getInformation(int sender_sock, void *buff, int len)
{
    int receive = recv(sender_sock, buff, len, 0);

    if (receive == -1)
    {
        perror("receive");
        exit(1);
    }

    else if (!receive)
    {
        printf("connection is close");
        return 0;
    }

    return receive;
}

/**
 *   This func changes the TCP Congestion control algoritm.
 *   my_sock: the receiver's socket descriptor (representing the socket's endpoint)
 *   rOc: algoritm to switch to, 1 for RENO, 0 for CUBIC
 */
void renoOrCubic(int my_sock, char *rOc)
{
    if (rOc)
    {
        socklen_t renoLen = strlen(reno);
        if (setsockopt(my_sock, IPPROTO_TCP, TCP_CONGESTION, reno, renoLen) != 0)
        {
            perror("setsockopt");
            exit(1);
        }
    }
    else
    {
        socklen_t cubicLen = strlen(cubic);
        if (setsockopt(my_sock, IPPROTO_TCP, TCP_CONGESTION, cubic, cubicLen) != 0)
        {
            perror("setsockopt");
            exit(1);
        }
    }
}

/**
 * This func calculates and prints requaired times about the times that the receiver saved during his connection with the sender.
 * times: array of the saved times, one cell for every time that the sender sended the information.
 * numOfSends: the number of sends that the senders sended the information
 * fSize: the size of the sended file
 */
void print_times(DynamicArray *arr, double fSize)
{
    double times_sum = 0;
    double avg = 0;
    double bandwidth = 0;
    double speeds_sum = 0;
    printf("* statistics: *\n");
    for (size_t i = 0; i < arr->size; i++)
    {
        times_sum += arr->times[i];
        speeds_sum += arr->speeds[i];
        printf("Run #%zu Data: Time=%fms, Speed=%f MB/s\n", i + 1, arr->times[i], arr->speeds[i]);
    }
    if (arr->size > 0)
    {
        avg = (times_sum / (double)arr->size);
        bandwidth = (speeds_sum / (double)arr->size);
    }

    printf("Average time: %0.3lf ms\n", avg);
    printf("Average speed: %0.3lf ms\n", bandwidth);
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
    //save the receiver PORT
    int RECEIVER_PORT;
    for (size_t i = 0; i < argc; i++)       // //this loop receives the receiver ip, port, and the cc algoritm from the user
    {
        if (!strcmp(argv[i], "-p"))
        {
            RECEIVER_PORT = atoi(argv[i + 1]);
        }
        if (!strcmp(argv[i], "-algo"))
        {
            algoritm = argv[i + 1];
        }
    }
    char *FIN = "I would like to end the connection please";     //massage to end the connection between the receiver and the sender
    signal(SIGPIPE, SIG_IGN); // on linux to prevent crash on closing socket
    //create dynamicArray for this connection
    DynamicArray *times_arr = malloc(sizeof(DynamicArray));
    times_arr->size = 0;    //no data exists yet
    times_arr->capacity = 1;    //ready to get one set of data (time and speed), will be increased if necessary
    times_arr->times = (double *)malloc(sizeof(double));
    times_arr->speeds = (double *)malloc(sizeof(double));

    printf("Starting up reciever\n");

    //create the listening socket
    int listeningSocket = -1;

    if ((listeningSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
    }

    int enableReuse = 1;        // Reuse the address 
    if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0)
    {
        perror("setsockopt");
    }

    printf("Creating the receiver socket...\n");
    struct sockaddr_in receiverAddress;
    memset(&receiverAddress, 0, sizeof(receiverAddress));

    receiverAddress.sin_family = AF_INET;
    receiverAddress.sin_addr.s_addr = INADDR_ANY;
    receiverAddress.sin_port = htons(RECEIVER_PORT); // network order

    // Bind the socket to the port with any IP at this port
    if (bind(listeningSocket, (struct sockaddr *)&receiverAddress, sizeof(receiverAddress)) == -1)
    {
        perror("bind");
        close(listeningSocket);
        return -1;
    }

    // Make the socket listening; actually mother of all client sockets.
    if (listen(listeningSocket, 500) == -1) // 500 is a Maximum size of queue connection requests
                                            // number of concurrent connections
    {
        perror("listen");
        close(listeningSocket);
        return -1;
    }

    // Accept and incoming connection
    printf("Waiting for incoming TCP-connections...\n");
    
    struct sockaddr_in senderAddress;
    socklen_t senderAddressLen = sizeof(senderAddress);
    memset(&senderAddress, 0, sizeof(senderAddress));
    senderAddressLen = sizeof(senderAddress);
    //create a socket for our sender by accepting the connection using the listening socket
    int senderSocket = accept(listeningSocket, (struct sockaddr *)&senderAddress, &senderAddressLen);
    if (senderSocket == -1)
    {
        perror("listen");
        close(listeningSocket);
        close(senderSocket);
        return -1;
    }
    char *buff = NULL;  //to save the sender's info file
    int fSize = 0;      //saves size of the sender's info file
    int received = 0;   //saves how many bytes were received from the sender

    printf("Sender connected, receiving the file's size...\n");
    getInformation(senderSocket, &fSize, sizeof(int));  //gets the size of the sender's info file
    printf("received size successfuly\n");

    //prepering buff to receive the data
    buff = malloc(fSize * sizeof(char));
    if (buff == NULL)
    {
        printf("malloc failed\n");
        exit(1);
    }
    //to calculate the send time
    clock_t begin;
    clock_t end;
    do
    {
        printf("getting the file info...\n");
        received = 0;
        int receivedPart;   //the info may come in segments, save the num of bytes received in every segment
        begin = clock();    //start measure the time
        memset(buff, 0, fSize);
        while (received != fSize && strcmp(buff, FIN) != 0) //check if the intire file received or if received a FIN massage from the user
        {
            receivedPart = getInformation(senderSocket, buff, fSize - received);    //receive the data

            if (!receivedPart)  //connection is close
            {
                printf("ERROR! connection is close");
                break;
            }

            if (receivedPart < 0)   //receive failed
            {
                 return 1;
            }

            received += receivedPart;
        }
        if (strcmp(buff, FIN) == 0) //check if received the FIN massage
        {
            break;
        }
        end = clock();  //stop measure the time
        printf("File transfer complete\n");

        if (!receivedPart)
        {
            break;
        }
        //save the calculations on our dynamicArr
        int mb = byteToMegabyte(received);  
        double t = ((double)(end - begin) / CLOCKS_PER_SEC);
        AddToDArr(times_arr, t, mb / (t / 1000));

    } while (strcmp(buff, FIN) != 0);

    printf("Receiver closing the connection...\n");
    //close the sockets
    close(listeningSocket);
    close(senderSocket);
    //print the calculations
    print_times(times_arr, fSize);
    //freeing the using memory
    free(buff);
    free_arr(times_arr);
    return 0;
}
