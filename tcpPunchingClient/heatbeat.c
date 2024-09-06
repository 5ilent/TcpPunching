#include "heatbeat.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>

int heatBeat(int socketfd) {
    printf("[ info ] heat beat start ...\n");
    char heatBeatPayload[] = "B:hello";
    for(;;) {
        write(socketfd, heatBeatPayload, strlen(heatBeatPayload));
        sleep(10);
    }
}