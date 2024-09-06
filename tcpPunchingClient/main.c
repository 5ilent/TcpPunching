#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "master.h"
#include "peer.h"
#include "global.h"

/*
usage:
    ./client <targetIp> <role>
example:
    targetIp: 123.123.123.123
    Role: Caller / Callee
*/
int main(int argc, char **argv) {
    printf("[ * ] start ...\n");

    strcpy(serverIp, argv[1]);
    int serverPort = 5555;
    printf("[ + ] server addr: %s:%d\n", serverIp, serverPort);
    communicateWithMaster(serverIp, serverPort);
    return 0;
}

