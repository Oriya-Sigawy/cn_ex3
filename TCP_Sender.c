/**
 * this file represents a sender that sends files to the receiver over TCP
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

char *fileName = "file to send";            //name of the sending file

//two options of congestion control algoritms
char *reno = "reno";
char *cubic = "cubic";
char *algoritm;     //represents the algoritm we are using

/**
 * this func was givven in the assignment, it creates a file of random data.
 * size: the desired size of the file
 * returns: buffer of the random data in the desired size
*/
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


int main(int argc, char *argv[])
{
    //save the receiver details; IP and PORT
    char *RECEIVER_IP_ADDRESS;      
    int RECEIVER_PORT;

    char *FIN = "I would like to end the connection please";        //massage to end the connection between the receiver and the sender

    for (size_t i = 0; i < argc; i++)               //this loop receives the receiver ip, port, and the cc algoritm from the user
    {
        if (!strcmp(argv[i], "-p"))
        {
            RECEIVER_PORT = atoi(argv[i + 1]);
        }
        if (!strcmp(argv[i], "-algo"))
        {
            algoritm = argv[i + 1];
        }
        if (!strcmp(argv[i], "-ip"))
        {
            RECEIVER_IP_ADDRESS = argv[i + 1];
        }
    }

    printf("Creating the socket...\n");
    //defines the receiver socket
    struct sockaddr_in receiverAddress;
    memset(&receiverAddress, 0, sizeof(receiverAddress));

    receiverAddress.sin_family = AF_INET;       //ipv4
    receiverAddress.sin_port = htons(RECEIVER_PORT);    //receiver port
    socklen_t algo_len = strlen(algoritm);

    int sock = socket(AF_INET, SOCK_STREAM, 0);         //creating the socket over TCP
    setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, algoritm, algo_len);  
    int rval = inet_pton(AF_INET, (const char *)RECEIVER_IP_ADDRESS, &receiverAddress.sin_addr);    
    if (rval <= 0)
    {
        perror("inet_pton");
        return -1;
    }

    printf("connecting to %s:%d...\n", RECEIVER_IP_ADDRESS, RECEIVER_PORT);

    if (connect(sock, (struct sockaddr *)&receiverAddress, sizeof(receiverAddress)) == -1)  //connecting to receiver
    {
        perror("connect()");
    }

    printf("connected to receiver\n");
    unsigned int size = 2048 * 1024;    //size required in the assignment
    char *fileInfo = util_generate_random_data(size);
    if (fileInfo == NULL)
    {
        perror("util_generate_random_data");
        return -1;
    }
    printf("file of random data created successfuly\n sending the file's size\n");
    send(sock, &size, sizeof(int), 0);  //send the file size to receiver
    
    char choise;        //saves the user's choise for sending the file again
    int bytesSent;      //num of the bytes that sent to the receiver
    do
    {
        bytesSent = send(sock, fileInfo, size, 0);  //sending the file
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
        printf("do you want me to send the file again?\n press y for yes, n for no\n");     // ask for the user's choise
        choise = getchar();     //receive the user's choise
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