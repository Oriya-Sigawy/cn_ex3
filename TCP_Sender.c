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

char *fileName = "file to send";
char *reno = "reno";
char *cubic = "cubic";
char *algoritm;

char *util_generate_random_data(unsigned int size)
{
    char *buffer = NULL;
    if (size == 0)
    {
        return NULL;
    }
    buffer = (char *)calloc(size, sizeof(char));
    if (buffer == NULL)
    {
        return NULL;
    }
    for (unsigned int i = 0; i < size; i++)
    {
        *(buffer + 1) = ((unsigned int)rand() % 256);
    }
    return buffer;
}

/**
 *   This func changes the TCP Congestion control algoritm.
 *   my_sock: the sender's socket descriptor (representing the socket's endpoint)
 *   rOc: algoritm to switch to, 1 for RENO, 0 for CUBIC
 */
void renoOrCubic(int my_sock, char *renoOrCubic)
{
    if (!strcmp(renoOrCubic, reno))
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

#define RECEIVER_IP_ADDRESS "127.0.0.1"
#define RECEIVER_PORT 8888      //TODO- receive this detauls from user

int main(int argc, char *argv[])
{
    char *FIN = "I would like to end the connection please";
    algoritm = argv[2];

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("socket");
    }

    printf("Creating the socket...\n");
    struct sockaddr_in receiverAddress;
    memset(&receiverAddress, 0, sizeof(receiverAddress));

    receiverAddress.sin_family = AF_INET;
    receiverAddress.sin_port = htons(RECEIVER_PORT);
    socklen_t algo_len = strlen(algoritm);
    setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, algoritm, algo_len);
    int rval = inet_pton(AF_INET, (const char *)RECEIVER_IP_ADDRESS, &receiverAddress.sin_addr);
    if (rval <= 0)
    {
        perror("inet_pton");
        return -1;
    }

    printf("connecting to %s:%d...\n", RECEIVER_IP_ADDRESS, RECEIVER_PORT);

    if (connect(sock, (struct sockaddr *)&receiverAddress, sizeof(receiverAddress)) == -1)
    {
        perror("connect()");
    }

    printf("connected to receiver\n");

    unsigned int size = 2048 * 1024;
    char *fileInfo = util_generate_random_data(size);
    if (fileInfo == NULL)
    {
        perror("util_generate_random_data");
        return -1;
    }
    printf("file of random data created successfuly\n sending the file's size\n");
    send(sock, &size, sizeof(int), 0);
    char choise;
    int bytesSent;
    do
    {
        bytesSent = send(sock, fileInfo, size, 0);
        if (-1 == bytesSent)
        {
            perror("send()");
        }
        else if (0 == bytesSent)
        {
            printf("peer has closed the TCP connection prior to send().\n");
        }
        else if (size > bytesSent)
        {
            printf("sent only %d bytes from the required %d.\n", size, bytesSent);
        }
        else
        {
            printf("message was successfully sent .\n");
        }
        printf("do you want me to send the file again?\n press y for yes, n for no\n");
        choise = getchar();
        while (choise != 'n' && choise != 'y')
        {
            choise = getchar();
        }

    } while (choise != 'n');

    send(sock, FIN, strlen(FIN) + 1, 0);
    printf("Closing the connection...\n");
    close(sock);

    return 0;
}