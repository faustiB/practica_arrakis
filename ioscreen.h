//Define Guard
#ifndef _IOSCREEN_H
#define _IOSCREEN_H
#define _GNU_SOURCE

//Llibreries del sistema
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

//Procedimens i funcions
char * IOSCREEN_readUntilIntro(int fd, char caracter, int i);
char * IOSCREEN_readDelimiter(int fd, char delimiter);
char * IOSCREEN_read_until(int fd, char end);
int IOSCREEN_isEmpty(const char * string);

#endif //_IOSCREEN_H