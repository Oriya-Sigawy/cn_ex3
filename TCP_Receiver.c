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

char *reno = "reno";
char *cubic = "cubic";
char *algoritm;

typedef struct
{
    double *times;
    double *speeds;
    int size;
    int capacity;
} DynamicArray;

void AddToDArr(DynamicArray *arr, double time, double speed)
{
    if (arr->size >= arr->capacity)
    {
        arr->capacity *= 2;
        arr->times = (double *)realloc(arr->times, arr->capacity * sizeof(double));
        arr->speeds = (double *)realloc(arr->speeds, arr->capacity * sizeof(double));
    }
    arr->times[arr->size] = time;
    arr->speeds[arr->size] = speed;
    arr->size++;
}
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

int byteToMegabyte(int bytes)
{
    return bytes / (1024 * 1024);
}

/**
 *
 */
int main(int argc, char *argv[])
{
    int RECEIVER_PORT;
    for (size_t i = 0; i < argc; i++)
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
    char *FIN = "I would like to end the connection please";
    signal(SIGPIPE, SIG_IGN); // on linux to prevent crash on closing socket
    DynamicArray *times_arr = malloc(sizeof(DynamicArray));
    times_arr->size = 0;
    times_arr->capacity = 1;
    times_arr->times = (double *)malloc(sizeof(double));
    times_arr->speeds = (double *)malloc(sizeof(double));
    printf("Starting up reciever\n");

    int listeningSocket = -1;

    if ((listeningSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
    }

    // Reuse the address if the server socket on was closed and remains for 45 seconds in TIME-WAIT state till the final removal.
    int enableReuse = 1;
    if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0)
    {
        perror("setsockopt");
    }

    // "sockaddr_in" is the "derived" from sockaddr structure used for IPv4 communication. For IPv6, use sockaddr_in6
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
    int senderSocket = accept(listeningSocket, (struct sockaddr *)&senderAddress, &senderAddressLen);
    if (senderSocket == -1)
    {
        perror("listen");
        close(listeningSocket);
        close(senderSocket);
        return -1;
    }
    char *buff = NULL;
    int fSize = 0;
    int received = 0;
    printf("Sender connected, receiving the file's size...\n");
    getInformation(senderSocket, &fSize, sizeof(int));
    printf("received size successfuly\n");
    buff = malloc(fSize * sizeof(char));
    if (buff == NULL)
    {
        printf("malloc failed\n");
        exit(1);
    }
    clock_t begin;
    clock_t end;
    do
    {
        printf("getting the file info...\n");
        received = 0;
        int receivedPart;
        begin = clock();
        memset(buff, 0, fSize);
        while (received != fSize && strcmp(buff, FIN) != 0)
        {
            receivedPart = getInformation(senderSocket, buff, fSize - received);

            if (!receivedPart)
            {
                break;
            }

            if (receivedPart < 0)
            {
                printf("ERROR! connection is close");
                return 1;
            }

            received += receivedPart;
        }
        if (strcmp(buff, FIN) == 0)
        {
            break;
        }
        end = clock();
        printf("File transfer complete\n");

        if (!receivedPart)
        {
            break;
        }
        int mb = byteToMegabyte(received);
        double t = ((double)(end - begin) / CLOCKS_PER_SEC);
        AddToDArr(times_arr, t, mb / (t / 1000));

    } while (strcmp(buff, FIN) != 0);
    printf("Receiver closing the connection...\n");
    close(listeningSocket);
    close(senderSocket);
    print_times(times_arr, fSize);
    free(buff);
    free_arr(times_arr);
    return 0;

    // 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
    // git remote set-url origin https://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g@github.com/Oriya-Sigawy/cn_ex3.git
    // https://github.com/Oriya-Sigawy/cn_ex3
}
