/* 
    A selection of debug tools and functions under test 
*/

#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#define _GNU_SOURCE     /* Requirement for scheduler*/
#include <sched.h>
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h> 
#include <fcntl.h>
#include <stdarg.h>
#include <termios.h>
#include <signal.h>

//careful <time.h> here caused an one time issue that never happened before
#include <errno.h> 

#define TIME_LIMIT_USEC 100     /* Stopwatch */
#define DEBUG_ON        1

#if DEBUG_ON
    #define print_debug_info(format, ...)  fprintf (stderr, format, ##__VA_ARGS__)
#endif

/* Print out formatting  */
static const char green[] = "\033[92m";
static const char blue[] = "\033[34m";
static const char red[] = "\033[91m";
static const char black[] = "\033[0m";

static int ST_attention = 1;
static int ST_ignore = 2;
static int ST_warning = 3;

/************************************
 * GLOBAL FUNCTION PROTOTYPES
 ************************************/
int mygetch (void);
void alarm_handler(int s);
void hexdumpunformatted(void *ptr, int buflen);
void humanise(int boolean);
double get_elapsed_time_usec(struct timespec *tic, struct timespec *toc);
int select_affinity(int core_id);
void myprintf(int status, char f, char *yourstring);
void printhelp(void);
int check_sudo(void);

#endif
