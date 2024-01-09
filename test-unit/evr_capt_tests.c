/*
 fa-capture under test
   VERSION 00.02.0 

   EVR testing
*/

#include "test_utils.h"
#include "bpm.h"
#include "sockets.h"

#include "erapi.h" 

/******************************/
/* OPERATION CONTROL          */
#define LATENCY_PERF 	    0

/* OBSOLETE                   */
#define REGISTER 		    0
#define ATTENDANCE_ALARM    0
#define ATTENDANCE_MANUAL   0
/******************************/

#define DUMP_PAYLOAD    	1
#define CPU_CORE            1
/* DEBUG_ON can be toogled in test_utils */
/******************************/

#define MAX_BUFF_SIZE 20000

/* EVR */
struct MrfErRegs *pEr;
int              fdEr;
static int fd;

/* Control variable for the main while loop */
static int send_data; 

/* Universal tick-tock structs for timing needs     */
/* CAREFUL: 
    tic is set by the "STOP" thread.    
    toc is set after the inner while loop is broken */
struct timespec tic, toc;       // not used right now


void init_bookkeeper(struct bookKeeper *book_keeper);
int prepare_socket(struct sockaddr_in server, int listen);
void print_payload(int compact_payload[PAYLOAD_FIELDS]);
void print_addressbook(struct bookKeeper *book_keeper);
int select_affinity(int core_id); 


/* This will stop the wave when interrupt thread handles an IRQ */
void thread_handler(int signum){
	print_debug_info("DEBUG: Concentrate and send signal in!.\n");
    /* Clock toc here, */
	//clock_gettime(CLOCK_MONOTONIC, &toc);   
    send_data = 1;
}



void evr_irq_handler(int param){
  int flags, i;
  struct FIFOEvent fe;

  flags = EvrGetIrqFlags(pEr);

  if (flags & (1 << C_EVR_IRQFLAG_PULSE)){
      printf("Pulse IRQ\n");
        EvrClearIrqFlags(pEr, (1 << C_EVR_IRQFLAG_PULSE));
        	
    }
 	EvrIrqHandled(fdEr);
   //raise(SIGUSR1);
}


void *irqsetup(){
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

    while(1);  // "wait for IRQs in the background"

    /* Never reach */
    pthread_exit(NULL);
}


void display_current_config(void) {
    char line[16];
    
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

    printf("Attendance Check with Alarm: ");
    fancy_print(ATTENDANCE_ALARM);
    #if ATTENDANCE_ALARM
        printf("Time interval for attendance check: %d usec\n", TIME_LIMIT_USEC);
    #endif    
    
    printf("Attendance Check (Manual): ");
    fancy_print(ATTENDANCE_MANUAL);

    printf("\n");

    printf("Packet Collection Parameters\n");
    printf("Estimated Packet Traffic per Box: %d pps\n", PACKET_MAX);
    printf("Capture Duration: %d\n", DURATION);
    printf("Number of Connected Sparks: %d\n", NO_SPARKS);
    printf("\n");

    printf("Server Configuration\n");
    printf("Server IP: %s\n", IP_ADDR);
    printf("Server Port: %d\n", PORT);

    printf("CPU core selection for multithreading: CPU %d\n", CPU_CORE);

    printf("---------------------------------------\n");
    
    #if ATTENDANCE_ALARM && ATTENDANCE_MANUAL
        printf("%s\n", "\033[91mWarning: Please select exactly ONE attendance modus!\033[0m");
        printf("Exiting now..\n");
        exit(EXIT_FAILURE);
    #endif

    #if (ATTENDANCE_ALARM || ATTENDANCE_MANUAL) && !REGISTER
        printf("%s\n", "\033[91mWarning: REGISTER must be set for attendance check!\033[0m");
        printf("Exiting now..\n");
        exit(EXIT_FAILURE); 
    #endif

    /* no warnings so far */
    printf("Capture will commence...\n");

    sleep(1);   // Give people some time to digest this
}


int main(){    

    display_current_config();  
	
    /********************************************/
    /* Payload Compression                      */
    /********************************************/
    long int payload_sums[PAYLOAD_FIELDS];          // this could be bypassed in the future
    int compact_payload[PAYLOAD_FIELDS]; 

    /********************************************/
    /* Packet Collection Parameters             */
    /********************************************/
    int packet_limit = PACKET_MAX * DURATION * NO_SPARKS;
    static struct packetRecord packet;   
    //static struct packetRecord queue[NO_SPARKS][FRAME_COMPLETE];

    /* CAREFUL */
    static struct packetRecord queue[NO_SPARKS][MAX_BUFF_SIZE]; 

    /********************************************/
    /* Socket for Reception                     */
    /********************************************/
    int socket_desc; 
    struct sockaddr_in server;
    socket_desc = prepare_socket(server, 1);    
    if (socket_desc == -1){
        perror("Socket binding failed. Exiting now..\n");
        exit(EXIT_FAILURE);
    }

    /********************************************/
    /* Client parameters for packet reception   */
    /********************************************/
    int client_addr_size;
    struct sockaddr_in client;
    client_addr_size = sizeof(client);
    int buf[PAYLOAD_FIELDS];

    /********************************************/          /*TODO*/
    /* Socket for Transmission                       
       Test phase naming convention             */
    /********************************************/
    int local_desc; 
    struct sockaddr_in local_server;
    local_desc = prepare_socket(local_server, 0); 
    if (local_desc == -1){
        perror("Socket binding failed. Packets are not forwarded.\n");
    }

    /********************************************/
    /* Address Book                             */ 
    /********************************************/
    struct bookKeeper book_keeper;
    init_bookkeeper(&book_keeper);
    
    /********************************************/
    /* Open EVR device                          */ 
    /********************************************/
    fdEr = EvrOpen(&pEr, "/dev/era3");
  
    if (fdEr == -1){
      return errno;
    } 

    /********************************************/
    /* Thread Control                           */
    /********************************************/	
    pthread_t irq_thread_id;

    /* MAIN */
    #if LATENCY_PERF
        /* Print this to make our lives bit easier when we later parse with pandas */
        printf("sec, nanosec\n");
    #endif

    /*******************************************/
    /* Some preparation for ATTENDANCE MANUAL  */
    /* Please do NOT move (time-critical)      */
    /*******************************************/
    // struct timespec start, end;     
	// int all = 0;
    // clock_gettime(CLOCK_MONOTONIC, &start);


    /* Register the signal thread uses to communicate with main loop */
	//signal(SIGUSR1, thread_handler);                      // reintroduce signal later. much cleaner
    print_debug_info("DEBUG: Reporting interrupt requests from EVR...\n");
    /* Start thread for interrupt handling */   
    pthread_create(&irq_thread_id, NULL, irqsetup, NULL);
 
    while(1){
        if(recvfrom(socket_desc, buf, sizeof(buf), 0, (struct sockaddr *) &client, &client_addr_size) >= 0){
            /* Record the packet (only valid for extended structures) */
            clock_gettime(CLOCK_MONOTONIC, &packet.arrival); 

            memcpy(packet.liberaData, buf, 16*sizeof(int));  
            packet.src_port = ntohs(client.sin_port);
            packet.id = client.sin_addr.s_addr;  //inet_toa(client.sin_addr)

            // Concentrate (after IRQ of course)
            // Simple way. Probably not cheap

            // New registery
            for(int i=0; i<NO_SPARKS; i++){
                // Get the box ID
                if(packet.id == book_keeper.box_id[i]){                       
                    // Put the packet in the corresponding queue, using current packet count as the index
                    book_keeper.count_per_libera[i]++;

                    // Check the buffer first
                    if (book_keeper.buffer_index[i] < MAX_BUFF_SIZE){
                        queue[i][book_keeper.buffer_index[i]] = packet ;
                        book_keeper.buffer_index[i]++;
                    }else {
                        book_keeper.buffer_index[i] = 0;                         // go to beginning
                        queue[i][book_keeper.buffer_index[i]] = packet ;    // then register the packet
                    }
                }

            }

            // IRQ signal then here:

            if(send_data){
                for(int i=0; i <NO_SPARKS; i++){
                    memset(payload_sums, 0 , sizeof(payload_sums));
                    // index the right packets and clean the queue.
                    // TODO you have to find a way to test this with regular arrays please
                    int buffer_start = book_keeper.count_per_libera[i] - book_keeper.buffer_index[i];
                
                    for(int curr_ind = 0; curr_ind < book_keeper.count_per_libera[i]; curr_ind++){ 
                        for(int payload_ind = 0; payload_ind< PAYLOAD_FIELDS; payload_ind++){
                            payload_sums[payload_ind] += queue[i][buffer_start].liberaData[payload_ind];
                            buffer_start++;
                        }                           
                    }

                    // most lkely unnecessary 
                    for(int ind = 0; ind< PAYLOAD_FIELDS; ind++){
                        compact_payload[ind] = (int) payload_sums[ind]/30;
                    }
                    #if DUMP_PAYLOAD
                        print_payload(compact_payload);
                    #endif

                    // Clean up
                    book_keeper.count_per_libera[i] = 0;
                    book_keeper.buffer_index[i] = 0; // just do yourself a favor here. Start writing from the beginning
                }
                send_data = 0;
            }
            
     
	

            #if LATENCY_PERF
                printf("%ld, %ld\n", packet.arrival.tv_sec, packet.arrival.tv_nsec); 
                /* Human readable format */
                // printf("%.5f\n",(double)packet.arrival.tv_sec + 1.0e-9 * packet.arrival.tv_nsec);  
            #endif
			
 

        }else{
            perror("recvfrom()");
		 if(errno == EINTR){
printf("%d\n", book_keeper.count_per_libera[0]);
			for(int i=0; i <1; i++){	// not all sparks
                    memset(payload_sums, 0 , sizeof(payload_sums));
                    // index the right packets and clean the queue.
                    // TODO you have to find a way to test this with regular arrays please
                    
					//int buffer_start = book_keeper.count_per_libera[i] - book_keeper.buffer_index[i];

					//printf("buffer start %d normalised %d\n", buffer_start, MAX_BUFF_SIZE-buffer_start);
				
                	// Assume buffer didn't "overflow" and always start from the beginning.
					// there is a mechanism for overflow but nothing for data integrity
                    for(int curr_ind = 0; curr_ind < book_keeper.count_per_libera[i]; curr_ind++){ 
                        for(int payload_ind = 0; payload_ind< PAYLOAD_FIELDS; payload_ind++){
                            payload_sums[payload_ind] += queue[i][curr_ind].liberaData[payload_ind];		//queue[i][buffer_start] 
                            //buffer_start++;
                        }                           
                    }
					
printf("payload sums 0 %d\n",  payload_sums[0]);

                    // most lkely unnecessary 
                    for(int ind = 0; ind< PAYLOAD_FIELDS; ind++){
						if(payload_sums[ind] == 0){
							compact_payload[ind] = (int) payload_sums[ind];
						}else{
                        	compact_payload[ind] = (int) payload_sums[ind]/book_keeper.count_per_libera[i]-1;
                    }
}
                    #if DUMP_PAYLOAD
                        print_payload(compact_payload);
                    #endif

                    // Clean up
                    book_keeper.count_per_libera[i] = 0;
                    book_keeper.buffer_index[i] = 0; // just do yourself a favor here. Start writing from the beginning
// always overwrite the buffer. 
                }
	
}		
             else {
                 // all other errors
               return -1;
             }		
        }
    } 

    /* Deallocate the socket*/
    close(socket_desc);

    return 0;
}
