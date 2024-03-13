/*
 fa-capture under test
   VERSION 00.05.1  -- Richards version with mrfioc2 support

TODO
-rename spark_bookkeeper 

   Open issues and TODOs (old not controlled)
   - signal, EINTR
   - array overflows
*/

#include "test_utils.h"		// keep this here GNU_SOURCE
#include "bpm.h"
#include "sockets.h"
 
#include <zmq.h>
#include<sys/wait.h>

/******************************/
/* OPERATION CONTROL          */
#define LATENCY_PERF 	    0
#define REGISTER 		    1
#define DUMP_PAYLOAD    	1
#define CPU_CORE            1

#define EVR_IRQ             1
#define SOFT_IRQ            0
#define ALARM_USEC       	6666.66

#define TESTMODE            0
/* DEBUG_ON can be toogled in test_utils */
/******************************/

#define MAX_BUFF_SIZE 20000     
static struct packetRecord queue[NO_SPARKS][MAX_BUFF_SIZE];         /* CAREFUL */

static volatile int send_data;
int evr_fd;

/* Additional test variables */
static int GLOBAL_PACKET_COUNTER;

/* Universal tick-tock structs for timing needs     */
/* CAREFUL: 
    tic is set by the "STOP" thread.    
    toc is set after the inner while loop is broken */
struct timespec tic, toc;       // not used right now


#if SOFT_IRQ
    /* Artifial pacemaker to set the rate at which 
        compressed packets should be sent */
    void concentration_pacemaker(int s){
        #if EVR_EVM_PRESENT 
            EvgSendSWEvent(pEg, 1);
        #endif
        
        send_data = 1;
        print_debug_info("DEBUG: Alarm!.\n");
        print_debug_info("DEBUG: Counter in alarm handler : %d.\n", GLOBAL_PACKET_COUNTER);
        GLOBAL_PACKET_COUNTER = 0;
        clock_gettime(CLOCK_MONOTONIC, &tic);
        print_debug_info("Alarm timestamp %.5f\n",(double)tic.tv_sec + 1.0e-9 * tic.tv_nsec);
        
        /* Repeatedly set the alarm for the next wave */
        ualarm(ALARM_USEC, 0);		// use 1000000 -1 
        signal(SIGALRM, concentration_pacemaker);
    }
#endif

/* Blocking read that doesn't harm anyone yet */
void uio_read(){
    select_affinity(CPU_CORE);
    int irq_count;
    while(1){
        if(read(evr_fd, &irq_count, 4) == 4){
           print_debug_info("DEBUG: IRQ received. Packet count : %d\n", GLOBAL_PACKET_COUNTER);
           send_data = 1;
        }
    }
}

/* Queue must be Global */
void compress_and_send(struct bookKeeper *spark_book_keeper, void *publisher, struct Message msg){
    /********************************************/
    /* Payload Compression                      */
    /********************************************/
    long long payload_sums[PAYLOAD_FIELDS];                         
    int compact_payload[PAYLOAD_FIELDS];

    for(int i=0; i < NO_SPARKS; i++){
        memset(payload_sums, 0 , sizeof(payload_sums));
       
        int buffer_start = spark_book_keeper->count_per_libera[i] - spark_book_keeper->buffer_index[i];
        printf("buffer start %d normalised %d\n", buffer_start, MAX_BUFF_SIZE-buffer_start);
		
        // if buffer and count are not the same, there was an overflow. 
        // then take the latest packets and compress those 
        for(int curr_ind = 0; curr_ind < spark_book_keeper->count_per_libera[i]; curr_ind++){ 
            for(int payload_ind = 0; payload_ind < PAYLOAD_FIELDS; payload_ind++){
                // Don't sum indices 8(LTM_h), 9(LTM_h) and 15(status), just use the fields of the latest packet
                if (payload_ind == 8 || payload_ind == 9 || payload_ind == 15){
                    payload_sums[payload_ind] = queue[i][curr_ind].liberaData[payload_ind];      
                }else{
                    payload_sums[payload_ind] += queue[i][curr_ind].liberaData[payload_ind];		//queue[i][buffer_start] 
                    //buffer_start++;
                }
            }                           
        }
		//print_debug_info("DEBUG: Payload sums vA for Spark 0 %d\n", payload_sums[0]);			

        // Format it the way it was before
        for(int ind = 0; ind< PAYLOAD_FIELDS; ind++){
            // Don't divide values = 0, and indices 8(LTM_h), 9(LTM_h) and 15(status)
            if(payload_sums[ind] == 0 || ind == 8 || ind == 9 || ind == 15){ 
                compact_payload[ind] = (int) payload_sums[ind]; 
            }else{
                compact_payload[ind] = (int) (payload_sums[ind]/spark_book_keeper->count_per_libera[i]);                
            }
        }

        #if DUMP_PAYLOAD
            print_payload(compact_payload);
        #endif

        memcpy(msg.payload, compact_payload, sizeof(compact_payload));
        snprintf(msg.spark_id, sizeof(msg.spark_id), "%s%s", "s", get_ipaddr_printable(spark_book_keeper->box_id[i], 1));  //i+1
        print_debug_info("DEBUG: %s\n", msg.spark_id);
        zmq_send(publisher, &msg, sizeof(struct Message), 0); 

        // Clean up
        spark_book_keeper->count_per_libera[i] = 0;
        spark_book_keeper->buffer_index[i] = 0; // just do yourself a favor here. Start writing from the beginning
        // Always overwrite the buffer. 
    }
    
    sleep(10);
}


void display_current_config(void) {
    printf("---------------------------------------\n");
    printf("Current Capture Configuration\n");
    
    printf("Latency Performance Recording: "); 
    fancy_print(LATENCY_PERF);

    printf("Compressed Payload Dump: ");
    fancy_print(DUMP_PAYLOAD);  
 
    printf("Packet Registery: ");
    fancy_print(REGISTER);

    #if TESTMODE
        printf("Packet Collection Parameters\n");
        printf("Estimated Packet Traffic per Box: %d pps\n", PACKET_MAX);
        printf("Capture Duration: %d\n", DURATION);
    #endif

    printf("\n");

    printf("Number of Connected Sparks: \033[92m%d\033[0m\n", NO_SPARKS);
    printf("\n");

    printf("Server Configuration\n");
    printf("Server IP: %s\n", IP_ADDR);
    printf("Server Port: %d\n", PORT);

    if(EVR_IRQ && SOFT_IRQ){
        printf("%s\n", "\033[91mPlease choose one interrupt control option!\033[0m"); 
        exit(EXIT_FAILURE);
    }

    printf("Interrupt Control Mode: ");
    if(EVR_IRQ) printf("\033[92mEVR_IRQ\033[0m");
    if(SOFT_IRQ) printf("\033[92mSOFT_IRQ\033[0m");
    printf("\n");
    
    printf("CPU core selection for multithreading: CPU %d\n", CPU_CORE);

    printf("---------------------------------------\n");
    
    /* no warnings so far */
    printf("Capture will commence...\n");

    sleep(1);   // Give people some time to digest this
}


int main(){    

    display_current_config(); 

    /********************************************/
    /* Packet Collection                        */
    /********************************************/
    static struct packetRecord packet;   
    struct bookKeeper spark_book_keeper;
    init_bookkeeper(&spark_book_keeper, example_addressbook);
    print_addressbook(&spark_book_keeper);

    /* TEST MODE */
    #if TESTMODE
        int packet_limit = PACKET_MAX * DURATION * NO_SPARKS;
    #endif
    
    pthread_t irq_thread_id;
    GLOBAL_PACKET_COUNTER = 0;
    
    /********************************************/
    /* Open EVR device                          */ 
    /********************************************/
    #if(EVR_IRQ)
        evr_fd = open("/dev/uio0", O_RDWR);

        if (evr_fd < 0) {
            perror("uio open:");
            return errno;
        }
    #endif

    /********************************************/
    /* Socket for Reception to collect from     */
    /*  Sparks and Client parameters            */
    /*                                          */
    /********************************************/
    int sock_collection; 
    struct sockaddr_in receive_server;
    sock_collection = prepare_socket(receive_server);    
    if (sock_collection == -1){
        perror("Socket binding failed. Exiting now..\n");
        exit(EXIT_FAILURE);
    }
    /**/
	long extend_buffer = 16777216;
	setsockopt(sock_collection, SOL_SOCKET, SO_RCVBUF,&extend_buffer, sizeof(long)); 

    int client_addr_size;
    struct sockaddr_in client;
    client_addr_size = sizeof(client);
    int buf[PAYLOAD_FIELDS];

    /********************************************/
    /* ZeroMQ PUB/SUB                           */
    /********************************************/
    struct Message msg;
    void *context = zmq_ctx_new ();
    void *publisher = zmq_socket (context, ZMQ_PUB);
    zmq_bind(publisher, "tcp://*:9999");

    /*******************************************************************************/
    /* MAIN */
    #if LATENCY_PERF
        /* Print this to make our lives bit easier when we later parse with pandas */
        printf("sec, nanosec\n");
    #endif
   
    /* Start thread for interrupt handling */ 
    print_debug_info("DEBUG: Reporting interrupt requests from EVR...\n");
    pthread_create(&irq_thread_id, NULL, (void *) uio_read, NULL);
    
    #if(SOFT_IRQ)
        ualarm(ALARM_USEC, 0);
	    signal(SIGALRM, concentration_pacemaker);
    #endif

    while(1){
        if(send_data == 2){
            compress_and_send(&spark_book_keeper, publisher, msg);
            send_data = 0;
            GLOBAL_PACKET_COUNTER = 0;
        }  

        if(recvfrom(sock_collection, buf, sizeof(buf), 0, (struct sockaddr *) &client, (socklen_t *) &client_addr_size) >= 0){
            /* Record the packet */
            clock_gettime(CLOCK_MONOTONIC, &packet.arrival); 
            GLOBAL_PACKET_COUNTER++;

            #if LATENCY_PERF
                printf("%ld, %ld\n", packet.arrival.tv_sec, packet.arrival.tv_nsec); 
                /* Human readable format */
                // printf("%.5f\n",(double)packet.arrival.tv_sec + 1.0e-9 * packet.arrival.tv_nsec);  
            #endif

           
            #if REGISTER
                memcpy(packet.liberaData, buf, 16*sizeof(int));  
                packet.src_port = ntohs(client.sin_port);
                packet.id = client.sin_addr.s_addr; 

                // Semi-cyclic registeration
                for(int i=0; i<NO_SPARKS; i++){
                    if(packet.id == spark_book_keeper.box_id[i]){             // Get the box ID                   
                        spark_book_keeper.count_per_libera[i]++;              // Put the packet in the corresponding queue

                        // Check the buffer limits 
                        // maximum buffer size is deducted by 1. (0 indexing for buffer, 1 indexing for count!)                                 
                        if (spark_book_keeper.buffer_index[i] < MAX_BUFF_SIZE-1){
                            queue[i][spark_book_keeper.buffer_index[i]] = packet ;
                            spark_book_keeper.buffer_index[i]++;

                            print_debug_info("DEBUG: current packet count for this Spark is %d\n", spark_book_keeper.count_per_libera[i]);
                            print_debug_info("DEBUG: current buffer index for this Spark is %d\n", spark_book_keeper.buffer_index[i]);

                        }else {
                            spark_book_keeper.buffer_index[i] = 0;                         // go to beginning
                            queue[i][spark_book_keeper.buffer_index[i]] = packet ;         // then register the packet
                        }
                    }
                    if (spark_book_keeper.count_per_libera[i] == 1000) send_data = 2;
                }
                

            #endif   
            
        }else{
            perror("recvfrom()");
            if(errno == EINTR){
                // Check how often we end up here with mrfioc2. Priority : Not urgent
                continue;
            }else {
                // all other errors
                return -1;
            }		
        }
    } 

    /* Never reach */
    /* Deallocate the socket*/
    close(sock_collection);
    zmq_close (publisher);
    zmq_ctx_destroy (context);

    return 0;
}