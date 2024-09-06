#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include "asm-generic/socket.h"
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include "master.h"
#include "util.h"
#include "global.h"
#include "peer.h"
#include "heatbeat.h"

char *calleeClientIds[100];
char *getAllPeerInfoRes;

int interactWithServer(int socketfd, char *input, char *output) {
    memset(output, 0, 100);
    write(socketfd, input, strlen(input));
    // printf("[ info ] write numbers: %d\n", strlen(input));
    int n = read(socketfd, output, 100);
    n = strlen(output);
    // printf("[ info ] read numbers: %d\n", n);
    return n;

}

int connectMaster(char *ip, int port) {
    printf("-[connect to master]-----------------------------------------------\n");
    int masterClientFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    printf("[ + ] master client socket fd: %d\n", masterClientFd);

    int curError;

    int reUseValue = 1;
    if(setsockopt(masterClientFd, SOL_SOCKET, SO_REUSEADDR, &reUseValue, sizeof(reUseValue)) !=0) {
        printf("[ err ] <master client> set SO_REUSEADDR fail: [%d] %s\n", errno, strerror(errno));
        return -1;
    } else {
        printf("[ + ] <master client> set SO_REUSEADDR success\n");
    }

    if(setsockopt(masterClientFd, SOL_SOCKET, SO_REUSEPORT, &reUseValue, sizeof(reUseValue)) !=0) {
        printf("[ err ] <peer client> set SO_REUSEPORT fail: [%d] %s\n", errno, strerror(errno));
        return -1;
    } else {
        printf("[ + ] <master client> set SO_REUSEPORT success\n");
    }

    struct sockaddr_in localAddr;
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(LOCAL_PORT);
    curError = bind(masterClientFd, (struct sockaddr *)&localAddr,sizeof(localAddr));
    if(curError != 0) {
        printf("[ err ] <master client:> bind local addr fail: [%d] %s\n", errno, strerror(errno));
        return -1;
    } else {
        printf("[ + ] <master client> bind local addr success\n");
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &serverAddr.sin_addr);
    serverAddr.sin_port = htons(port);

    curError = connect(masterClientFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if(curError != 0) {
        printf("[ err ] <master client> connect to serer fail: [%d] %s\n", errno, strerror(errno));
        return -1;
    } else {
        printf("[ + ] <master client> connect to server success\n");
    }

    return masterClientFd;
}

int CallerBehive(int socketfd) {

    char res[100];
    int n, result;

     // 2. get peer info
    int calleeNum = 0;
    char getPeerInfoPayload[50];
    sprintf(getPeerInfoPayload, "G:%s", selfClientId);
    for (;;) {
        sleep(2);
        char *getPeerRes = (char *)malloc(3500);
        memset(getPeerRes, 0, 3500);
        write(socketfd, getPeerInfoPayload, strlen(getPeerInfoPayload));
        n = read(socketfd, getPeerRes, 3500);
        if (strcmp("NoPeer", getPeerRes) == 0) {
            printf("[ - ] get no peer info\n");
        } else {
            memset(calleeClientIds, 0, 100*sizeof(char*));
            calleeNum = splitString(getPeerRes, ":", calleeClientIds);
            
            printf("[ + ] callee total: %d\n", calleeNum);
            for(int i = 0; i < calleeNum; i++) {
                printf("[ callee-%d ] %s\n", i+1, calleeClientIds[i]);
            }
            break;
        }
    }


    // connect each callee in loop
    for(int i = 0; i < calleeNum; i++) {

        // 2. get single callee ip:port
        char dIp[20];
        char dPort[6];
        memset(dIp, 0, 20);
        memset(dPort, 0, 6);
        char getSinglePeerPayload[50];
        memset(getSinglePeerPayload, 0, 50);
        sprintf(getSinglePeerPayload, "g:%s", calleeClientIds[i]);
        n = interactWithServer(socketfd, getSinglePeerPayload, res);
        char *gStrs[5];
        splitString(res, ":", gStrs);
        strcpy(dIp, gStrs[1]);
        strcpy(dPort, gStrs[2]);

        // 3. require to connect to target peer
        char connectPayload[80];
        memset(connectPayload, 0, 80);
        sprintf(connectPayload,"C:%s:%s", selfClientId, calleeClientIds[i]);
        printf("[ + ] start require to connect to target ...\n");
        n = interactWithServer(socketfd, connectPayload, res);
        
        // test
        // write(socketfd, connectPayload, strlen(connectPayload));
        // strcpy(res, "Ready");
        // sleep(3);

        if (strcmp("Ready", res) == 0) {

            // 4. connect to target peer
            int pid = fork();
            if(pid < 0) {
                printf("[ err ] fork connect to peer fail\n");
                exit(-1);
            } else if(pid == 0) {
                printf("[ sys ] <child proc> connect to peer\n");
                char payload[] = "msg from Caller";
                int rtn = connectToPeer(dIp, atoi(dPort), payload);
                if(rtn != 0) {
                    exit(-1);
                } else {
                    exit(0);
                }
            } else {
                printf("[ system ] <main proc> waiting for child proc (Caller connect to Callee)\n");
                int status;
                wait(&status);
                if(WIFEXITED(status)) {
                    printf("[ sys ] <main proc> child proc exit(Caller connect to Callee)\n");
                }
            }
        }
    }

    return 0;
}

int CalleeBehive(int socketfd) {
    // 3. wait for connect signal
    int n, result;
    char signal[100];
    memset(signal, 0, 100);
    while((n = read(socketfd, signal, 100)) > 0) {
        printf("[ + ] signal: %s\n", signal);

        // parse signal
        char *strs[5];
        splitString(signal, ":", strs);
        strcpy(targetClientId, strs[1]);
        strcpy(targetIp, strs[2]);
        strcpy(targetPort, strs[3]);
        printf("[ info ] Caller clientId: %s\n", targetClientId);
        printf("[ info ] Caller ip: %s\n", targetIp);
        printf("[ info ] Caller port: %s\n", targetPort);


        int peerServerPid, peerClientPid;
        int peerServerStatus, peerClientStatus;

        // 4. start peer server 
        // peerServerPid = fork();
        // if(peerServerPid < 0) {
        //     printf("[ err ] fork start peer server fail\n");
        //     return -1;
        // } else if(peerServerPid == 0) {
        //     printf("[ sys ] <child proc> start peer server\n");
        //     int res = StartPeerServer();
        //     if(res != 0) {
        //         exit(-1);
        //     } else {
        //         exit(0);
        //     }
        // } else {
        //     printf("[ sys ] <main proc> waiting for child proc (peer server)\n");
        //     waitpid(peerServerPid, &peerServerStatus, WNOHANG);
        // }
    

        // 5. connect to Caller
        peerClientPid = fork();
        if(peerClientPid < 0) {
            printf("[ err ] fork connect to peer fail\n");
        } else if(peerClientPid == 0) {
            printf("[ sys ] <child proc> connect to peer\n");
            char payload[] = "mustFail";
            int rtn = connectToPeer(targetIp, atoi(targetPort), payload);
            if(rtn != 0){
                exit(-1);
            } else {
                exit(0);
            }
        } else {
            // 6. send reply to master
            char answerPayload[50];
            sprintf(answerPayload, "A:%s", targetClientId);
            write(socketfd, answerPayload, strlen(answerPayload));

            printf("[ sys ] <main proc> waiting for child proc (Callee connect to Caller)\n");
            int status;
            wait(&status);
            if(WIFEXITED(status)) {
                printf("[ sys ] <main proc> child proc exit(Callee connect to Caller)\n");
                return 0;
            }          
        }
    }
}

int communicateWithMaster(char *serverIp, int serverPort) {

    // connect to master server
    masterfd = connectMaster(serverIp, serverPort);

    // fork heatbeat
    int pid = fork();
    if(pid < 0) {
        printf("[ err-%d ] fork heatbeat fail: %s\n", errno, strerror(errno));
    } else if(pid == 0) {
        heatBeat(masterfd);
        exit(0);
    }


    char res[100];
    int n;


    // 1. Register
    printf("--[ Register ]--------------------------------\n");
    char registerPaylod[10] = "R:egister";
    n = interactWithServer(masterfd, registerPaylod, res);
    char *registerStrs[5];
    splitString(res, ":", registerStrs);
    strcpy(selfClientId, registerStrs[0]);
    strcpy(selfIp, registerStrs[1]);
    strcpy(selfPort, registerStrs[2]);
    printf("[ info ] self clientId: %s\n", selfClientId);
    printf("[ info ] self ip: %s\n", selfIp);
    printf("[ info ] self port: %s\n", selfPort);

    if(strcmp("full", selfClientId) == 0) {
        printf("[ - ] client is full\n");
        return 0;
    }


    // 2. wait for Role in loop
    // for(;;) {

        memset(res, 0, 100);
        n = read(masterfd, res, 100);
        printf("--[ Role ]-----------------------------------------\n");
        char *roleStrs[10];
        // R:Caller / R:Callee
        n = splitString(res, ":", roleStrs);
        memset(role, 0, 10);
        strcpy(role, roleStrs[1]);
        printf("[ info ] peer role: %s\n", role);
    
        if(strcmp("Caller", role) == 0) {
            printf("[ + ] wait for 10s ...\n");
            fflush(stdout);
            sleep(13);
            CallerBehive(masterfd);
        } else if(strcmp("Callee", role) == 0) {
            CalleeBehive(masterfd);
        }

    // }
    

    close(masterfd);
    return 0;
}

