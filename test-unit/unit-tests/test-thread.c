/* Test unit for interrupt registering via thread */
#define _GNU_SOURCE     /* Requirement for scheduler*/
#include <sched.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h> 

#include "test_utils.h"
#include "bpm_utils.h"

#define CPU_CORE 1 

static int32_t irq_count;
static int fd;
static int loop_control;

/* Universal tick-tock structs for timing needs */
struct timespec tic, toc;


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

/* Register IRQ */
void *thread_register_irq(void *flags){
	select_affinity(CPU_CORE); 
    
    int notify = (uintptr_t) flags;

    int stop = 0;
    while(!stop) {
        if(read(fd, &irq_count, 4) == 4){
            printf("IRQ: Interrupt counter: %d\n", irq_count);
            clock_gettime(CLOCK_MONOTONIC, &tic);
            stop = 1;
        }
        if(notify) {
            /* If notify is passed, thread will wait for "STOP */
            loop_control = loop_control - stop;
        }
    }
    pthread_exit(NULL);	
}


/* Dummy function, just count a bit, keep CPU busy */
void do_something(){
    int count = 10000;
	while(count > 0){
		count--;
	}
	printf("...\t");
}


int main(){    

    /*******************************************/
    /* Open device file                        */
    /*******************************************/
    fd = open("/dev/uio0", O_RDWR);                             

    if (fd < 0) {
        perror("uio open:");
        return errno;
    }

	pthread_t start_cond, stop_cond;

    // just to make the compiler bit happy today. Don't make this global!
    int *notify_on = (int *) 1;

    do{ 
        printf("DEBUG: Waiting for start signal...\n");
        /* Start thread for "START" signal */
        pthread_create(&start_cond, NULL, thread_register_irq, NULL); 
        /* Wait for blocking read to complete */
        (void) pthread_join(start_cond, NULL);    

        loop_control = 1;    
        /* Start thread for "STOP" signal */
        pthread_create(&stop_cond, NULL, thread_register_irq, (void *)notify_on); 
        
        while(loop_control){
            /* Important stuff will happen here */
            do_something();
            
        }

        printf("\n");
        printf("*******************************************\n");
        clock_gettime(CLOCK_MONOTONIC, &toc);
        double elapsed = get_elapsed_time_usec(&tic, &toc);
        printf("Time elapsed \n thread detected an IRQ --- while loop stop via cont:\n %f usec\n", elapsed);
        printf("*******************************************\n");

        pthread_detach(stop_cond);      // cleanup, probably not necessary

    }while(1);  


    return 0;
}
