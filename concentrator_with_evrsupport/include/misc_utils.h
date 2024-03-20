/* 
    A selection of debug tools and functions under test 
*/

#ifndef MISC_UTILS_H
#define MISC_UTILS_H

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
//#define DEBUG_ON        1
// #define TESTCODE

// #ifdef DEBUG_ON
//     #define print_debug_info(format, ...)  fprintf (stderr, format, ##__VA_ARGS__)
// #endif

/************************************
 * FUNCTION PROTOTYPES
 ************************************/
int mygetch (void);
void fancy_print(int boolean);
char *humanise(int boolean);
double get_elapsed_time_usec(struct timespec *tic, struct timespec *toc);
int select_affinity(int core_id);
int check_sudo();

#ifdef TESTCODE
    void alarm_handler(int s);
    void hexdumpunformatted(void *ptr, int buflen);
#endif

#endif
