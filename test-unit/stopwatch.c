/**/
#include <errno.h>
#include <string.h>

#include "test_utils.h"
#include "bpm_utils.h"

/* OPERATION CONTROL */
#define LATENCY_PERF 	    0
#define REGISTER 		    1
#define ATTENDANCE          1   /* REGISTER has to be set for attendance check */
#define DEBUG_AVERAGING 	0



void test_stopwatch(void) {
    signal(SIGALRM, signal_handler);
    ualarm(TIME_LIMIT_USEC, 0);

    /* Test code */
    int n = 0;
    while (1) {
        ++n;
        printf("packet number %d \n ", n);
    }
}

void display_current_config(void) {
    char line[16];
    
    printf("---------------------------------------\n");
    printf("Current Capture Configuration\n");
    printf("Latency Performance Recording: "); 
    fancy_print(LATENCY_PERF);
 
    printf("Packet Registery: ");
    fancy_print(REGISTER);

    printf("Attendance Check: ");
    fancy_print(ATTENDANCE);
    #if ATTENDANCE
        printf("Time interval for attendance check %d usec\n", TIME_LIMIT_USEC);
    #endif

    printf("Dump compressed payload: ");
    fancy_print(DEBUG_AVERAGING);   
    printf("\n");

    printf("Packet Collection Parameters\n");
    printf("Estimated Packet Traffic per Box: %d pps\n", PACKET_MAX);
    printf("Capture Duration: %d\n", DURATION);
    printf("Number of Connected Sparks: %d\n", NO_SPARKS);

    printf("---------------------------------------\n");
    
    #if ATTENDANCE && !REGISTER
        printf("%s\n", "\033[91mWarning: REGISTER must be set for attendance check!\033[0m");
        exit(EXIT_FAILURE);
    #endif
    
    sleep(1);   // Give people some time to digest this
}

int main(void) {

    display_current_config();

   // printf("%s", "\033[92mEnter Number : \033[34m");
    
    //printf("%s", "\033[0m");


    return 0;
}