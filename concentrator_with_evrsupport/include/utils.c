/* Misc utility functions and test routines */
#include "test_utils.h"

/*
 Print help function
*/
void printhelp(void){
    printf("usage: ./irq-capture [-i <dest_ip>] [-p <dest_port>] [-b <bpm_selection>] [-d] [-h] [-l]\n");
    printf("where: \n \t-h \tprint this help message\n");
    printf("\t-d \tdump compressed payload to stdout\n");
    printf("\t-l \tlist available address books\n");
    printf("\t-i \tIP address of remote host that receives the compressed BPM data\n");
    printf("\t-p \tport of remote host that receives the compressed BPM data\n");
    printf("\t-b \tBPM section to select the corresponding IP addressbook\n");

    exit(EXIT_SUCCESS);
}

/* 
 Colourful printf formatting for warning messages and configuration parameters
*/
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

/* 
 Wrapper to set CPU affinity for the thread 
*/ 
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
 Convert boolean to ON/OFF 
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


int check_sudo(void){
	uid_t euid = geteuid();
    return euid;
}