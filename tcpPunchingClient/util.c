#include <string.h>
#include <stdio.h>

int splitString(char *str, char *delim, char **strs) {

    char *token;
    int num = 0;
    memset(strs, 0, 10);
    token = strtok(str, delim);
    while( token != NULL ) {
        strs[num] = token;
        num++;
        token = strtok(NULL, delim);
    }
    // printf("total: %d\n", num);
    return num;
}

