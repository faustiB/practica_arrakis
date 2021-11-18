#ifndef _FREMEN_H
#define _FREMEN_H
#define _GNU_SOURCE


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/wait.h>
#include <signal.h>

//Define
#define printF(x) write(1, x, strlen(x))

typedef struct {
    int seconds_to_clean;
    char *ip;
    int port;
    char *directory;
} Config;

#endif //_FREMEN_H
