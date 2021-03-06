//Define Guard
#ifndef _FRAME_CONFIG_H
#define _FRAME_CONFIG_H
#define _GNU_SOURCE

//Llibreries del sistema
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

//Tipus propis
typedef struct {
    char origin[15];
    char type;
    char data[240];
} Frame;

typedef struct {
    char file_name[30];
    int file_size;
    char file_md5[35];
    int photo_fd;
} Photo;

//Procedimens i funcions
char * FRAME_CONFIG_generateFrame(int origin);
Frame FRAME_CONFIG_receiveFrame(int fd);
char * FRAME_CONFIG_getMD5(char * file);
char * FRAME_CONFIG_generateCustomFrame(int origin, char type, int isOk);

#endif //_FRAME_CONFIG_H
