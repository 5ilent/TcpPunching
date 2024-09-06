#include "global.h"

int LOCAL_PORT = 8888;
int masterfd;
char serverIp[20];
char role[10];

char selfClientId[50];
char selfIp[20];
char selfPort[6];
char targetClientId[50];
char targetIp[20];
char targetPort[6];