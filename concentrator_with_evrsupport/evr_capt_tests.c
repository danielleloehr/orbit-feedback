/*
 fa-capture under test
   VERSION 00.05.0  -- Richards version 

   Main EVR test version
   - Comment everything dependent on Jukka's API 
   - Comment all irrelevant paramters too
   
   Open issues and TODOs (old not controlled)
   - signal, EINTR
   - array overflows
*/

#include "test_utils.h"		// keep this here GNU_SOURCE
#include "bpm.h"
#include "sockets.h"
 
// #include "erapi.h" 
// #include "egapi.h"

#include <zmq.h>
#include<sys/wait.h>

/******************************/
/* OPERATION CONTROL          */
#define LATENCY_PERF 	    0
#define REGISTER 		    1
#define DUMP_PAYLOAD    	1
#define CPU_CORE            1
#define EVR_IRQ             1
// #define ALARM_USEC       	6666.66  //6666.66 // 1 second)
// #define EVR_EVM_PRESENT     1
// #define CAPTURE             1
/* DEBUG_ON can be toogled in test_utils */
/******************************/

#define MAX_BUFF_SIZE 20000     // for one second more than enough
static struct packetRecord queue[NO_SPARKS][MAX_BUFF_SIZE];         /* CAREFUL */

/* Main server information                          
   Forward compressed packets to this address       */
/* TODO: replace by ZeroMQ */
static char LOCAL_ADDR[16] = "192.168.2.200"; 
static int LOCAL_PORT = 2049;                           // different than 2048

static volatile int send_data;
int fd;

// /******************************/
// /* EVR */
// struct MrfErRegs *pEr;
// int              fdEr;
// /* Control variable for the main while loop */
// /******************************/

// /******************************/
// /* EVG */
// struct MrfEgRegs *pEg;
// int              fdEg;
// /******************************/

// struct Message{
// 		char spark_id[5];
//         int payload[16];
// };

/* Additional test variables */
static int GLOBAL_PACKET_COUNTER;

/* Universal tick-tock structs for timing needs     */
/* CAREFUL: 
    tic is set by the "STOP" thread.    
    toc is set after the inner while loop is broken */
struct timespec tic, toc;       // not used right now

/* Artifial pacemaker to set the rate at which 
    compressed packets should be sent */
// void concentration_pacemaker(int s){
//     #if EVR_EVM_PRESENT 
// 	    EvgSendSWEvent(pEg, 1);
//     #endif
    
// 	send_data = 1;
// 	print_debug_info("DEBUG: Alarm!.\n");
// 	print_debug_info("DEBUG: Counter in alarm handler : %d.\n", GLOBAL_PACKET_COUNTER);
// 	GLOBAL_PACKET_COUNTER = 0;
// 	clock_gettime(CLOCK_MONOTONIC, &tic);
// 	print_debug_info("Alarm timestamp %.5f\n",(double)tic.tv_sec + 1.0e-9 * tic.tv_nsec);
	
//     /* Repeatedly set the alarm for the next wave */
// 	ualarm(ALARM_USEC, 0);		// use 1000000 -1 
// 	signal(SIGALRM, concentration_pacemaker);
// }

//-------------------------------------------------------------------------------------------------
// TODO: please remove this once the test passes 
#ifdef EVR_API_IRQ
/* This will stop the wave when interrupt thread handles an IRQ */
void signal_ready_to_send(int signum){
	print_debug_info("DEBUG: Concentrate and send signal in!.\n");
    /* Clock toc here, */
	//clock_gettime(CLOCK_MONOTONIC, &toc);   
    print_debug_info("DEBUG: Counter in signal handler : %d.\n", GLOBAL_PACKET_COUNTER);
    send_data = 1;
}

void evr_irq_handler(int param){
    int flags;
    flags = EvrGetIrqFlags(pEr);
    print_debug_info("DEBUG: Counter in interrupt handler : %d.\n", GLOBAL_PACKET_COUNTER);

    if (flags & (1 << C_EVR_IRQFLAG_PULSE)){
        clock_gettime(CLOCK_MONOTONIC, &tic); 
        print_debug_info("%.5f\n",(double)tic.tv_sec + 1.0e-9 * tic.tv_nsec);
        //raise(SIGUSR1);
        print_debug_info("Pulse IRQ\n");
        EvrClearIrqFlags(pEr, (1 << C_EVR_IRQFLAG_PULSE));  	
    }
    GLOBAL_PACKET_COUNTER = 0;
 	EvrIrqHandled(fdEr);

}


void *irqsetup(){
    select_affinity(CPU_CORE);
    EvrEnable(pEr, 1);
    if (!EvrGetEnable(pEr))
        {
        printf("Could not enable EVR!\n");
        //return -1;  // Won't return an error code !!!
        }
    
    EvrSetTargetDelay(pEr, 0x01000000);
    EvrDCEnable(pEr, 1);
    
    EvrGetViolation(pEr, 1);

    /* Build configuration for EVR map RAMS */

    EvrSetLedEvent(pEr, 0, 0x02, 1);
    /* Map event 0x01 to trigger pulse generator 0 */
    EvrSetPulseMap(pEr, 0, 0x02, 0, -1, -1);
    EvrSetPulseParams(pEr, 0, 1, 100, 100);
    EvrSetPulseProperties(pEr, 0, 0, 0, 0, 1, 1);
    EvrSetUnivOutMap(pEr, 0, 0x3f00);
    EvrSetPulseIrqMap(pEr, 0x3f00);

    EvrSetExtEvent(pEr, 0, 0x01, 1, 0);

    EvrMapRamEnable(pEr, 0, 1);

    EvrOutputEnable(pEr, 1);

    EvrIrqAssignHandler(pEr, fdEr, &evr_irq_handler);
    EvrIrqEnable(pEr, EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_PULSE);

    //while(1);  // "wait for IRQs in the background"
    wait(NULL);     // be nice to CPU
    /* Never reach */
    pthread_exit(NULL);
}
#endif
//-------------------------------------------------------------------------------------------------
/* Blocking way to do it */
void uio_read(){
    select_affinity(CPU_CORE);
    int irq_count;
    while(1){
        if(read(fd, &irq_count, 4) == 4){
           print_debug_info("DEBUG: IRQ received. Packet count : %d\n", GLOBAL_PACKET_COUNTER);
           send_data = 1;
        }
    }
}


// depends on queue, I am not passing it... 
void send_spark_data(struct bookKeeper *book_keeper, int trans_sock, struct sockaddr_in transmit_server, void *requester, struct Message msg){
    /********************************************/
    /* Payload Compression                      */
    /********************************************/
    long long payload_sums[PAYLOAD_FIELDS];                              /*TODO*/
    int compact_payload[PAYLOAD_FIELDS];

    for(int i=0; i < NO_SPARKS; i++){	// not all sparks  /* CAREFUL*/ 
        memset(payload_sums, 0 , sizeof(payload_sums));
      
        //int buffer_start = book_keeper.count_per_libera[i] - book_keeper.buffer_index[i];
        //printf("buffer start %d normalised %d\n", buffer_start, MAX_BUFF_SIZE-buffer_start);
				
        // Assume buffer didn't "overflow" and always start from the beginning.
        // there is a mechanism for overflow but nothing for data integrity
        for(int curr_ind = 0; curr_ind < book_keeper->count_per_libera[i]; curr_ind++){ 
            for(int payload_ind = 0; payload_ind< PAYLOAD_FIELDS; payload_ind++){
                payload_sums[payload_ind] += queue[i][curr_ind].liberaData[payload_ind];		//queue[i][buffer_start] 
                //buffer_start++;
            }                           
        }
		//print_debug_info("DEBUG: Payload sums vA for Spark 0 %d\n", payload_sums[0]);			

        // Format it the way it was before
        for(int ind = 0; ind< PAYLOAD_FIELDS; ind++){
            if(payload_sums[ind] == 0){                                 // CAREFUL
                compact_payload[ind] = (int) payload_sums[ind];         // CAREFUL timestamps are divided !!!!
            }else{
                compact_payload[ind] = (int) (payload_sums[ind]/book_keeper->count_per_libera[i]-1);
            }
        }

        #if DUMP_PAYLOAD
            print_payload(compact_payload);
        #endif

        //sendto(trans_sock, compact_payload, sizeof(compact_payload), 0, (struct sockaddr *)&transmit_server, sizeof(transmit_server));

        memcpy(msg.payload, compact_payload, sizeof(compact_payload));
        snprintf(msg.spark_id, sizeof(msg.spark_id), "%s%d", "s", i);
	    // this should display the "last xxx" of the IP address according to the book keeper!!
        zmq_send(requester, &msg, sizeof(struct Message), 0);  //btw this works I am just trying to figure out why alarm stops working

        // Clean up
        book_keeper->count_per_libera[i] = 0;
        book_keeper->buffer_index[i] = 0; // just do yourself a favor here. Start writing from the beginning
        // Always overwrite the buffer. 
    }
}


void display_current_config(void) {
    printf("---------------------------------------\n");
    printf("Current Capture Configuration\n");
    #if IRQ_CNTRL_TEST 
        printf("Interrupt Control mode: "); 
        fancy_print(IRQ_CNTRL_TEST);
        printf("%s\n", "\033[91mPlease make sure all other options are deactivated!\033[0m"); 
    #endif

    printf("Latency Performance Recording: "); 
    fancy_print(LATENCY_PERF);

    printf("Compressed Payload Dump: ");
    fancy_print(DUMP_PAYLOAD);  
 
    printf("Packet Registery: ");
    fancy_print(REGISTER);

    printf("\n");

    // printf("Packet Collection Parameters\n");
    // printf("Estimated Packet Traffic per Box: %d pps\n", PACKET_MAX);
    // printf("Capture Duration: %d\n", DURATION);
    printf("Number of Connected Sparks: %d\n", NO_SPARKS);
    printf("\n");

    printf("Server Configuration\n");
    printf("Server IP: %s\n", IP_ADDR);
    printf("Server Port: %d\n", PORT);

    printf("CPU core selection for multithreading: CPU %d\n", CPU_CORE);

    printf("---------------------------------------\n");
    
    /* no warnings so far */
    printf("Capture will commence...\n");

    sleep(1);   // Give people some time to digest this
}


int main(){    

    display_current_config();   

    /********************************************/
    /* Packet Collection Parameters             */
    /********************************************/
    //int packet_limit = PACKET_MAX * DURATION * NO_SPARKS;
    static struct packetRecord packet;   

    /********************************************/
    /* Socket for Reception to collect original */
    /*  data                                    */
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

    /********************************************/
    /* Client parameters for packet reception   */
    /********************************************/
    int client_addr_size;
    struct sockaddr_in client;
    client_addr_size = sizeof(client);
    int buf[PAYLOAD_FIELDS];

    /********************************************/                  
    /* Socket to collect concentrated packets   */
    /********************************************/
    int sock_concentrated;
    struct sockaddr_in transmit_server;

    if ((sock_concentrated = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("socket()");
        return -1;
    }

    /* Set up the server name */
    transmit_server.sin_family      = AF_INET;               /* Internet Domain    */
    transmit_server.sin_port        = htons(LOCAL_PORT);     /* Server Port        */
    transmit_server.sin_addr.s_addr = inet_addr(LOCAL_ADDR); /* Server's Address   */

    /********************************************/
    /* ZeroMQ PUSH/PULL messaging line */
    /********************************************/
    struct Message msg;

    void *context = zmq_ctx_new ();
    void *requester = zmq_socket (context, ZMQ_PUB);

	/* Single Spark experiment */
	/* Connect to one listening socket */
	//char libera_send_address[27];
	//int sparkport = 9999;   // in case you would like to send to different ports       
	//snprintf(libera_send_address, sizeof(libera_send_address), "%s%s%s%d", "tcp://", LOCAL_ADDR,":", sparkport);	

    //zmq_connect (requester, libera_send_address);
    zmq_bind(requester, "tcp://*:9999");

    /********************************************/
    /* Address Book                             */ 
    /********************************************/
    struct bookKeeper book_keeper;
    init_bookkeeper(&book_keeper);
    
    /********************************************/
    /* Open EVR device                          */ 
    /********************************************/

    fd = open("/dev/uio0", O_RDWR);

    if (fd < 0) {
        perror("uio open:");
        return errno;
    }

    // #if EVR_EVM_PRESENT 
    //     fdEr = EvrOpen(&pEr, "/dev/era3");
    //     if (fdEr == -1){
    //         return errno;
    //     } 

    //     fdEg = EvgOpen(&pEg, "/dev/ega3");
    //     if (fdEg == -1){
    //         return errno;
    //     }
    // #endif

    /********************************************/
    /* Thread Control                           */
    /********************************************/	
    pthread_t irq_thread_id;

    /* MAIN */
    #if LATENCY_PERF
        /* Print this to make our lives bit easier when we later parse with pandas */
        printf("sec, nanosec\n");
    #endif

    /* Register the signal thread uses to communicate with main loop */
	//signal(SIGUSR1, signal_ready_to_send);                      // reintroduce signal later. much cleaner
    print_debug_info("DEBUG: Reporting interrupt requests from EVR...\n");
    /* Start thread for interrupt handling */   
    pthread_create(&irq_thread_id, NULL, uio_read, NULL);
    
    GLOBAL_PACKET_COUNTER = 0;
    // ualarm(ALARM_USEC, 0);
	// signal(SIGALRM, concentration_pacemaker);

    while(1){
        if(send_data){
            send_spark_data(&book_keeper,sock_concentrated, transmit_server, requester, msg);
            send_data = 0;
            GLOBAL_PACKET_COUNTER = 0;
        }  

        if(recvfrom(sock_collection, buf, sizeof(buf), 0, (struct sockaddr *) &client, (socklen_t *) &client_addr_size) >= 0){
            /* Record the packet (only valid for extended structures) */
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
                    if(packet.id == book_keeper.box_id[i]){             // Get the box ID                   
                        book_keeper.count_per_libera[i]++;              // Put the packet in the corresponding queue

                        // Check the buffer limits          <-- Check this again
                        if (book_keeper.count_per_libera[i] < MAX_BUFF_SIZE){
                            queue[i][book_keeper.buffer_index[i]] = packet ;
                            book_keeper.buffer_index[i]++;
                        }else {
                            book_keeper.buffer_index[i] = 0;                         // go to beginning
                            queue[i][book_keeper.buffer_index[i]] = packet ;         // then register the packet
                        }
                    }
                }

            #endif   

         
            
        }else{
            perror("recvfrom()");
            if(errno == EINTR){
                //send_spark_data(&book_keeper,sock_concentrated, transmit_server);
                continue;
            }else {
                // all other errors
                return -1;
            }		
        }
    } 
    zmq_close (requester);
    zmq_ctx_destroy (context);

    /* Never reach */
    /* Deallocate the socket*/
    close(sock_collection);
    close(sock_concentrated);

    return 0;
}