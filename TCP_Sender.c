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
#include "sock.h"

char* fileName= "file to send";
char* reno="reno";
char* cubic="cubic";
bool algoritm;

char* util_generate_random_data(unsigned int size)
{
    char *buffer = NULL;
    if (size==0)
    {
        return NULL;
    }
    buffer=(char*) calloc(size, sizeof(char));
    // if (buffer==NULL)
    // {
    //     return NULL;
    // }
    for (unsigned int i = 0; i < size; i++)
    {
        *(buffer+1)=((unsigned int) rand()%256);
    }
    return buffer;
}


int main(int argc, char *argv[])
{
    algoritm= argv[2];
    unsigned int size= 2048*1024;
    char *fileInfo=util_generate_random_data(size);
    if(fileInfo==NULL)
    {
        printf("util_generate_random_data() failed");
        return -1;
    }
    printf("file of random data created successfuly\n");
    printf("Starting SENDER...\n");
    int fileSize=0;
    struct sockaddr_in receiverAddr;                    //contains the connection details of the receiver
    struct sockaddr_in * preceiverAddr= &receiverAddr;

    printf("Reading the created file...\n");
    // fileInfo= readFile(&fileSize, buf);

    int my_sock= -1;
    printf("Creating the socket...\n");
    if((my_sock=socket(AF_INET, SOCK_STREAM, 0)==-1))
    {
        printf("couldn't create socket\n");
        return -1;
    }
    memset(preceiverAddr, 0, sizeof(*preceiverAddr));
    receiverAddr.sin_family = AF_INET;
    receiverAddr.sin_port = htons(RECEIVER_PORT);
    if(inet_pton(AF_INET, (const char*) RECEIVER_IP_ADDRESS, &receiverAddr.sin_addr)<0)
    {
        printf("inet_pton failed\n");
        return -1;
    }

    printf ("Start a TCP connection to %s, %d...\n", RECEIVER_IP_ADDRESS, RECEIVER_PORT);
    if(connect(my_sock, (struct sockaddr*) &receiverAddr, sizeof(receiverAddr))==-1)
    {
        printf("connection failed\n");
        exit(1);
    }


    printf("Sending the file's size\n");
    sendInformation(my_sock, &fileSize, sizeof(int));

    adjust(my_sock,0);          //to check that the receiver can receive the file
    
    while (true)
    {
        int choice;        //represent the user's choice whether to send the file again or not

        printf("Sending the info itself...\n");
        sendInformation(my_sock, fileInfo, fileSize);

        renoOrCubic(my_sock, algoritm);          //set the TCP Congestion control algoritm (0 for cubic, 1 for reno)
        adjust(my_sock, false);             //adjust the times between the sender and the receiver

        printf("Do you want me to send the file again?\n 1 for yes, 0 for no and exit\n");
        scanf("%d", &choice);
        
        
        adjust(my_sock, true);
        adjust(my_sock, false);

        if (!choice)        //exit
        {
            break;
        }          
    }

    printf("Closing the connection");
    close(my_sock);
    free(fileInfo);
    return 0;
}

/**
 * This func read the file that contains the information we want to send. 
 *  size: the size of the information's file.
 *  returns: array that contains the information.
*/
// char* readFile(int* size, char* buf)
// {
//     FILE * filePointer= NULL;
//     filePointer= fopen(buf, "r");
//     if (filePointer == NULL)
//     {
//         printf("ERROR! File is empty\n");
//         exit(1);
//     }
//     //alocate memory for the file by the size.
//     fseek(filePointer, 0L, SEEK_END);
//     *size= ftell(filePointer);
//     fseek(filePointer, 0L, SEEK_SET);
//     char* f= (char*) malloc(*size*sizeof(char)); 
//     fread(f, sizeof(char), *size, filePointer);
//     fclose(filePointer);
//     return f;
// }

/**
*   This func changes the TCP Congestion control algoritm.
*   my_sock: the sender's socket descriptor (representing the socket's endpoint)
*   rOc: algoritm to switch to, 1 for RENO, 0 for CUBIC
*/
 void renoOrCubic(int my_sock, int rOc)
{
    if(rOc)
    {
        socklen_t renoLen=strlen(reno);
        if(setsockopt(my_sock, IPPROTO_TCP, TCP_CONGESTION, reno, renoLen)!=0)
        {
            perror("setsockopt");
            exit(1);
        }
    }
    else
    {
        socklen_t cubicLen=strlen(cubic);
        if(setsockopt(my_sock, IPPROTO_TCP, TCP_CONGESTION, cubic, cubicLen)!=0)
        {
            perror("setsockopt");
            exit(1);
        }
    }
}

/**
 *  This func sends the information to the receiver. 
 *  my_sock: the sender's socket descriptor (representing the socket's endpoint)
 *  buf: contains the information to send
 *  len: length of the information to send
 *  returns: the num of bytes who succesfuly sent.
*/
int sendInformation(int my_sock, void* buf, int len)
{
    int sentInf = send(my_sock, buf, len, 0);

    if (sentInf == -1)
    {
        printf("ERROR! send failed\n");
        exit(1);
    }

    else if (!sentInf)
        printf("ERROR! Receiver didn't accept my request\n");

    else if (sentInf < len)
        printf("ERROR! send only part of the information\n");

    else
        printf("send success");

    return sentInf;
}

/**
 *  this func has 2 uses:
 *  1. this func helps to adjust the times between the server and the receiver and make a correct calculation of time.
 *  2. this func prevents the sender from sending more information before he received 
        confirmation from the receiver that the previous information sent has fully arrived.
 *  my_sock: the sender's socket descriptor (representing the socket's endpoint)
 *  loss: 1 if some of the packets got lost in their way, 0 if not
 * returns: 1 for succesfuly send, 0 if the socket was closed, exit(1) if were a problem in the sending.
*/
int adjust(int my_sock, bool loss)
{
    char c = 0;
    int x = 0;

    if (loss)
    {
        x = send(my_sock, &c, sizeof(char), 0);

        if (x == -1)
        {
            perror("send");
            exit(1);
        }

        return x;
    }

    //Waiting for approvement from receiver
    x = recv(my_sock, &c, sizeof(char), 0);

    if (x == -1)
    {
        perror("recv error");
        exit(1);
    }

   //can cuntinue

    return x;
}
