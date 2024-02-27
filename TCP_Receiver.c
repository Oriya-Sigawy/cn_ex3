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
#include "sock.h"

char * reno= "reno";
char * cubic= "cubic";
bool algoritm;

int main(int argc, char *argv[])
{
    algoritm= argv[2];

    struct sockaddr_in receiverAddr, senderAddr;
    char senderAddrArr[INET_ADDRSTRLEN];
    socklen_t senderAddrLen;

    char* buff = NULL;
    int fSize;
    int received = 0;

    int numOfSends = 0; 

    int my_sock = -1, sender_sock = -1;
   
    struct timeval start, end;
    double *times = (double*) malloc(10 * sizeof(double));              //array for the time calculating, every cell for one send of the information    

    int can_be_reused=1;
    printf("Starting Receiver...\n");
    if ((my_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("couldn't create socket");
        exit(1);
    }
    memset(&receiverAddr, 0, sizeof(receiverAddr));
    receiverAddr.sin_family = AF_INET;
    receiverAddr.sin_addr.s_addr = INADDR_ANY;
    receiverAddr.sin_port = htons(RECEIVER_PORT);

    if (setsockopt(my_sock, SOL_SOCKET, SO_REUSEADDR, &can_be_reused, sizeof(can_be_reused)) == -1)
    {
        printf("setsockopt failed\n");
        exit(1);
    }

    if (bind(my_sock, (struct sockaddr *) &receiverAddr, sizeof(&receiverAddr)) == -1)
    {
        printf("bind failed\n");
        exit(1);
    }

    if (listen(my_sock, MAX_SENDERS) == -1)
    {
        printf("listen() failed\n");  
        exit(1);
    }

    renoOrCubic(my_sock, algoritm);

    printf("Waiting for TCP connection...\n");
    memset(&senderAddr, 0, sizeof(senderAddr));

    senderAddrLen = sizeof(senderAddr);
    sender_sock = accept(my_sock, (struct sockaddr *) &senderAddr, &senderAddrLen);

     if (sender_sock == -1)
    {
        printf("didn't accept the socket\n");
        exit(1);
    }
        
    inet_ntop(AF_INET, &(senderAddr.sin_addr), senderAddrArr, INET_ADDRSTRLEN);

    printf("Sender connected, receiving the file's size...\n");
    getInformation(sender_sock, &fSize, sizeof(int));
    adjust(sender_sock, true);
    printf("received size successfuly\n");

    buff = malloc(fSize * sizeof(char));

    if (buff == NULL)
    {
        printf("malloc failed\n");
        exit(1);
    }

    printf("beginning to receive files...\n");

    int temp=5;
    while (true)
    {
        int receivedPart;
        if (!received)
        {
            memset(buff, 0, fSize);
            gettimeofday(&start, NULL);
            
        }

        receivedPart = getInformation(sender_sock, buff + received, fSize - received);

        if (!receivedPart)
        {
            break;
        }
        
        received+=receivedPart;

        if(received==fSize)     //received the entire file
        {

        gettimeofday(&end, NULL);
        times[numOfSends]= ((end.tv_sec - start.tv_sec)*1000) + (((double)(end.tv_usec - start.tv_usec))/1000);
        numOfSends++;

        if (numOfSends >= temp)     //more memory needs to be allocate cause the num of sends is bigger than what we already allocated
            {
                temp += 5;
                double* p = (double*) realloc(times, (temp * sizeof(double)));
                times=p;
            }

        }

        printf("File transfer complete\n");

        if (!adjust(sender_sock, false))        //check the user's desicion whether the sender will send the file again or not
        {
                    break;
        }

        adjust(sender_sock, true);
        received=0;
    }
    close(sender_sock);
    printf("Sender sent exit message\n");

    timeCalc(times, numOfSends, fSize);
    usleep(1000);

    close(my_sock);                         //receiver socket closed

    free(times);                            //free memory
    free(buff);
    
    return 0;
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
        printf("receive failed");
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
 *  This func sends the information to the sender. 
 *  sender_sock: the sender's socket descriptor (representing the socket's endpoint)
 *  buf: contains the information to send
 *  len: length of the information to send
 *  returns: the num of bytes who succesfuly sent.
*/
int sendInformation(int sender_sock, void* buf, int len)
{
    int sentInf = send(sender_sock, buf, len, 0);

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
*   This func changes the TCP Congestion control algoritm.
*   my_sock: the receiver's socket descriptor (representing the socket's endpoint)
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
 *  this func has 2 uses:
 *  1. this func helps to adjust the times between the server and the receiver and make a correct calculation of time.
 *  2. this func sends a confirmation when all the segments of the information accapted.
 *  my_sock: the receiver's socket descriptor (representing the socket's endpoint)
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

   printf("Waiting for sender response...");
    x = recv(my_sock, &c, sizeof(char), 0);

    if (x == -1)
    {
        perror("recv error");
        exit(1);
    }


    return x;
}

/**
 * This func calculates and prints requaired data about the times that the receiver saved during his connection with the sender.
 * times: array of the saved times, one cell for every time that the sender sended the information.
 * numOfSends: the number of sends that the senders sended the information
 * fSize: the size of the sended file
*/
void timeCalc(double* times, int numOfSends, int fSize) {
    double sum = 0;
    // double avg;
    // double rate;

    for (int i = 0; i < numOfSends; ++i)
    {
        sum += times[i];
        printf("Run # %d Data: Time= %0.3lf ms; Speed=\n", (i+1), times[i]);
    }

    // if (numOfSends > 0)
    // {
    //     avg = (sum / (double)numOfSends);
    // }

    // rate = ((fSize / 1024) / (avg / 1000));
    /////////////////////need to add prints////////////
}