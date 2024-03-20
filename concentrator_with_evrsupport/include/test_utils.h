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
void printhelp();


void printhelp(){
    printf("usage: ./irq-capture [-i <dest_ip>] [-p <dest_port>] [-b <bpm_selection>] [-h] [-d]\n");
    printf("where: \n \t-h \tprint this help message\n");
    printf("\t-d \tdump compressed payload to stdout\n");
    printf("\t-l \tlist available address books\n");
    printf("\t-i \tIP address of a remote host that receives the compressed BPM data\n");
    printf("\t-p \tport of a remote host that receives the compressed BPM data\n");
    printf("\t-b \tBPM section to select the corresponding IP addressbook\n");

    exit(EXIT_SUCCESS);
}


void myprintf(int status, char f, char *yourstring){
    char buffer[100];

    char *format;
    if(f == 'd'){
        format = "%s%d%s";
    }else if(f == 's'){
        format = "%s%s%s";
    }else{
        printf("Unspecified format!\n");
        exit(EXIT_FAILURE);
    }

    switch(status){
        case 1:
           snprintf(buffer, sizeof(buffer),format, green, yourstring, black);
           break;
        case 2:
            snprintf(buffer, sizeof(buffer),format, blue, yourstring, black);
            break;
        case 3:
            snprintf(buffer, sizeof(buffer),format, red, yourstring, black);
            break;
        default:
            snprintf(buffer, sizeof(buffer),format, black, yourstring, black);
            break;
    }
    printf("%s\n", buffer); 
}


/* Wrapper to set CPU affinity for the thread */ 
int select_affinity(int core_id) {
   int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
   if (core_id < 0 || core_id >= num_cores)
      return EINVAL;

   cpu_set_t cpuset;
   CPU_ZERO(&cpuset);
   CPU_SET(core_id, &cpuset);

   pthread_t current_thread = pthread_self();    
   return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}


/*  
 Reads char from stdin with wait. Source:
 https://faq.cprogramming.com/cgi-bin/smartfaq.cgi?answer=1042856625&id=1043284385
*/
int mygetch ( void ) {
  int ch;
  struct termios oldt, newt;
  
  tcgetattr ( STDIN_FILENO, &oldt );
  newt = oldt;
  newt.c_lflag &= ~( ICANON | ECHO );
  tcsetattr ( STDIN_FILENO, TCSANOW, &newt );
  ch = getchar();
  tcsetattr ( STDIN_FILENO, TCSANOW, &oldt );
  
  return ch;
}

/*
 Signal handler for alarm(). Upon completion, starts a new timer 
 depending on user input
*/
void alarm_handler(int s) {
   
    printf("Capturing stops.\n" );

TRY:    
    printf("Continue with next capture? [y/n] \n");

    char control = mygetch();

    if(control == 'y') {
        printf("Schedule a new alarm...\n");
        /* Schedule a new alarm */
        ualarm(TIME_LIMIT_USEC, 0);    //for every second
        signal(SIGALRM, alarm_handler);
    }

    if (control == 'n') {
        printf("Stopping.\n");
    }     

    if (control != 'y' && control != 'n') {
        printf ("Pretty sure I said y or n.. Try again.\n");
        goto TRY;
    }
}

/* 
 DEBUG TOOL : content dump
*/
void hexdumpunformatted(void *ptr, int buflen) {
    // hexdumpunformatted(sample.payload, 64);
    unsigned char *buf = (unsigned char*)ptr;
    int i;
    for (i=0; i<buflen; i++) {
        printf("%02x ", buf[i]);
        printf(" ");
    }
}

/* 
 Colourful print to grab attention
*/

void humanise(int boolean) {
    char *human_word;
    if (boolean == 1) {
        human_word = "ON";
        myprintf(ST_attention, 's', human_word);
    }

    if (boolean == 0) {
        human_word = "OFF";
        myprintf(ST_ignore, 's', human_word);
    }
}

/*
 Calculate elapsed time in usec
*/
double get_elapsed_time_usec(struct timespec *tic, struct timespec *toc){
    long nsecs = ((long)(toc->tv_sec * 1.0e9) + (long)toc->tv_nsec) - ((long)(tic->tv_sec * 1.0e9) + (long)tic->tv_nsec);
    return (double) nsecs/1000;     
}

int check_sudo(){
	uid_t euid = geteuid();
    return euid;
}

#endif
