#ifndef _SOCK_H
#define _SOCK_H

#define MAX_SENDERS 1           //defines the number of senders that can connect to the receiver at the same time

#define RECEIVER_IP_ADDRESS "127.0.0.1"
#define RECEIVER_PORT 8888

char* util_generate_random_data(unsigned int);
char* readFile(int*, char*);
void renoOrCubic(int, int);
int sendInformation(int, void*, int);
int getInformation(int, void*, int);
int adjust(int, bool);
void timeCalc(double*, int, int);  

#endif
