#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "asm-generic/socket.h"
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "global.h"


int process(int connfd, char *data) {
    char buf[50];
    memset(buf, 0, 50);
    int n;
    int idx = 0;
    while ((n = read(connfd, buf, 50)) > 0)
    {
        idx++;
        printf("[ %d ] Callee recv: %s\n", idx, buf);
        write(connfd, data, strlen(data));
        memset(buf, 0, 50);
        if(strcmp("Q", buf) == 0) {
            break;
        }
    }
    return 0;
    
}


int connectToPeer(char *dIp, int dPort, char *payload) {

    printf("-[connect to peer - %s]-----------------------------------------------\n", role);
    int peerClientfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    printf("[ + ] <%s> peer client socket fd: %d\n", role, peerClientfd);


    // don't set ttl
    // if (strcmp("Callee", role) == 0) {
    //     printf("[ ttl ] %d\n", CalleeTtl);
    //     if (setsockopt(peerClientfd, IPPROTO_IP, IP_TTL, (void*)&CalleeTtl, sizeof(CalleeTtl)) != 0) {
    //         printf("[ err ] <peer client> set IP_TTL fail: [%d] %s\n", errno, strerror(errno));
    //         return -1;
    //     } else {
    //         printf("[ + ] <peer client> set IP_TTL success\n");
    //     }
    // }

    int reUseValue = 1;
    if(setsockopt(peerClientfd, SOL_SOCKET, SO_REUSEADDR, &reUseValue, sizeof(reUseValue)) !=0) {
        printf("[ err ] <peer client - %s> set SO_REUSEADDR fail: [%d] %s\n", role, errno, strerror(errno));
        close(peerClientfd);
        return -1;
    } else {
        printf("[ + ] <peer client - %s> set SO_REUSEADDR success\n", role);
    }

    if(setsockopt(peerClientfd, SOL_SOCKET, SO_REUSEPORT, &reUseValue, sizeof(reUseValue)) !=0) {
        printf("[ err ] <peer client - %s> set SO_REUSEPORT fail: [%d] %s\n", role, errno, strerror(errno));
        close(peerClientfd);
        return -1;
    } else {
        printf("[ + ] <peer client - %s> set SO_REUSEPORT success\n", role);
    }

    int res;
    
    struct sockaddr_in localAddr;
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(LOCAL_PORT);
    res = bind(peerClientfd, (struct sockaddr *)&localAddr,sizeof(localAddr));
    if(res != 0) {
        printf("[ err ] <peer client - %s> bind fail: [%d] %s\n", role, errno, strerror(errno));
        close(peerClientfd);
        return -1;
    }  else {
        printf("[ + ] <peer client - %s> bind success\n", role);
    }

    struct sockaddr_in targetAddr;
    targetAddr.sin_family = AF_INET;
    inet_pton(AF_INET, dIp, &targetAddr.sin_addr);
    targetAddr.sin_port = htons(dPort);

    int flag = 0;
    int retryTimes = 500;
    int delay = 500000;
    if(strcmp("Caller", role) == 0) {
        printf("[ + ] Caller wait for 2s\n");
        retryTimes = 500;
        delay = 500000;
        sleep(2);
    }
    
    for(int i = 1; i <= retryTimes; i++) {
        // void usleep(microseconds);
        usleep(delay);
        res = connect(peerClientfd, (struct sockaddr *)&targetAddr, sizeof(targetAddr));
        if(res != 0) {
            // printf("[ err--%d ] <peer client - %s> connect to peer fail: [%d] %s\n", i, role, errno, strerror(errno));
            printf("*");
            fflush(stdout);
            flag = -1;
        }  else {
            printf("\n[ + ] <peer client - %s> connect to peer success\n", role);
            printf("[ + ] <peer client - %s> peer clientId: %d\n", role, peerClientfd);
            flag = 0;
            break;
        }
    }


    if(strcmp("Callee", role) == 0) {
        char notePayload[200];
        memset(notePayload, 0, 200);
        if(flag == 0) {
            // send connect fail msg to master
            sprintf(notePayload, "N:%s:%s:%s:%s:%s:%s:Success", targetClientId, targetIp, targetPort, selfClientId, selfIp, selfPort);
        } else if(flag == -1) {
            sprintf(notePayload, "N:%s:%s:%s:%s:%s:%s:Fail", targetClientId, targetIp, targetPort, selfClientId, selfIp, selfPort);
        }
        write(masterfd, notePayload, strlen(notePayload));
    }

    if(flag == -1) {
        close(peerClientfd);
        return -1;
    }
    
    if(strcmp("Callee", role) == 0) {
        printf("[ + ] Callee will process connection.\n");
        char callbak[] = "msg from Callee";
        process(peerClientfd, callbak);

    } else if (strcmp("Caller", role) == 0) {
        for(int i = 1; i <= 5; i++) {
            sleep(1);
            printf("[ + ] Caller send: %s\n", payload);
            write(peerClientfd, payload, strlen(payload));
            char buf[100];
            memset(buf, 0, 100);
            read(peerClientfd, buf, 50);
            printf("[ %d ] Caller recv : %s\n", i, buf);
        }
        char exitSignal[] = "Q";
        write(peerClientfd, exitSignal, strlen(exitSignal));
    }

    close(peerClientfd);
    return 0;
}


int StartPeerServer() {
    printf("-[start peer server]-------------------------------------------------\n");
    int peerServerfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    printf("[ + ] peer server socket fd: %d\n", peerServerfd);

    int reUseValue = 1;
    if(setsockopt(peerServerfd, SOL_SOCKET, SO_REUSEADDR, &reUseValue, sizeof(reUseValue)) !=0) {
        printf("[ err ] <peer server> set SO_REUSEADDR fail: [%d] %s\n", errno, strerror(errno));
        return -1;
    } else {
        printf("[ + ] <peer server> set SO_REUSEADDR success\n");
    }

    if(setsockopt(peerServerfd, SOL_SOCKET, SO_REUSEPORT, &reUseValue, sizeof(reUseValue)) !=0) {
        printf("[ err ] <peer server> set SO_REUSEPORT fail: [%d] %s\n", errno, strerror(errno));
        return -1;
    } else {
        printf("[ + ] <peer server> set SO_REUSEPORT success\n");
    }

    int res;
    struct sockaddr_in localAddr;
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(LOCAL_PORT);
    res = bind(peerServerfd, (struct sockaddr *)&localAddr,sizeof(localAddr));
    if(res != 0) {
        printf("[ err ] <peer server> bind local addr fail: [%d] %s\n", errno, strerror(errno));
        return -1;
    } else {
        printf("[ + ] <peer server> bind local addr success\n");
    }

    res = listen(peerServerfd, 5);
    if(res != 0) {
        printf("[ err ] <peer server> listen fail: [%d] %s\n", errno, strerror(errno));
        return -1;
    } else {
        printf("[ + ] <peer server> listen at: %d\n", ntohs(localAddr.sin_port));
    }

    int connfd;
    for (;;){
        struct sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);
        connfd = accept(peerServerfd, (struct sockaddr *)&clientAddr, &clientAddrLen);
        printf("[ + ] <peer server> accept client peer, connfd: %d\n", connfd);
        char echo[] = "msg from Callee";
        process(connfd, echo);
    }
    close(connfd);
}