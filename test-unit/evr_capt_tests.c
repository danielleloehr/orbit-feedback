/*
 fa-capture under test
   VERSION 00.02.0 

   EVR testing
*/

#include "test_utils.h"
#include "bpm.h"
#include "sockets.h"

#include "erapi.h"      // fix this issue with makefile

/******************************/
/* OPERATION CONTROL          */
#define LATENCY_PERF 	    0
#define REGISTER 		    0
#define ATTENDANCE_ALARM    0
#define ATTENDANCE_MANUAL   0
#define DUMP_PAYLOAD    	1
#define CPU_CORE            1
/* DEBUG_ON can be toogled in test_utils */
/******************************/

#define MAX_BUFF_SIZE 200000

/* EVR */
struct MrfErRegs *pEr;
int              fdEr;
static int fd;

/* Control variable for the main while loop */
static int send_data;  // should be 0. set it to -1 for now to check the rest of the code without libera

static int packet_counter;

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
    // Raise signal here
        EvrClearIrqFlags(pEr, (1 << C_EVR_IRQFLAG_PULSE));
        raise(SIGUSR1);	
    }
  
 	EvrIrqHandled(fdEr);
    // then try to raise it here
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
    static struct packetRecord queue[NO_SPARKS][FRAME_COMPLETE]; 

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
    struct timespec start, end;     
	int all = 0;
    clock_gettime(CLOCK_MONOTONIC, &start);

    packet_counter = 0;
	send_data = 0;

    /* Register the signal thread uses to communicate with main loop */
	signal(SIGUSR1, thread_handler);
    print_debug_info("DEBUG: Reporting interrupt requests from EVR...\n");
    /* Start thread for interrupt handling */   
    pthread_create(&irq_thread_id, NULL, irqsetup, NULL);
 
    while(1){
        if(recvfrom(socket_desc, buf, sizeof(buf), 0, (struct sockaddr *) &client, &client_addr_size) >= 0){

            /* Record the packet (only valid for extended structures) */
            clock_gettime(CLOCK_MONOTONIC, &packet.arrival); 
            memcpy(packet.liberaData, buf, 16*sizeof(int));  
            packet.src_port = ntohs(client.sin_port);
            packet.id = client.sin_addr.s_addr;

            packet_counter++;

            if(send_data){
                //concentrate();
                //sendto(local_desc, compact_payload, sizeof(compact_payload), 0, (struct sockaddr *)&local_server, sizeof(local_server));
                //just print the last one
                #if DUMP_PAYLOAD
                    print_payload(packet.liberaData);
                #endif
				send_data=0;
                printf("packet_counter %d\n", packet_counter);
            }

            #if LATENCY_PERF
                printf("%ld, %ld\n", packet.arrival.tv_sec, packet.arrival.tv_nsec); 
                /* Human readable format */
                // printf("%.5f\n",(double)packet.arrival.tv_sec + 1.0e-9 * packet.arrival.tv_nsec);  
            #endif
			
            #if REGISTER
                /* Record the packet */   
                memcpy(packet.liberaData, buf, 16*sizeof(int));  
                packet.src_port = ntohs(client.sin_port);
                packet.id = client.sin_addr.s_addr;  //inet_toa(client.sin_addr)

                /* Register packets, sort by sender and check if any queue exceeds 30 */
                #if ATTENDANCE_ALARM
					signal(SIGALRM, alarm_handler);
                    ualarm(TIME_LIMIT_USEC, 0);

                    for(int i=0; i<NO_SPARKS; i++){
                        if(packet.id == book_keeper.box_id[i]){                       
                            queue[i][book_keeper.count_per_libera[i]] = packet ;        
                            book_keeper.count_per_libera[i] += 1;                           
                        }
                    }
					print_addressbook(&book_keeper);
                #endif

                #if ATTENDANCE_MANUAL
                    /* Manual attendance check stops when exactly 1 packet is 
                       received from each Spark */
                    for(int i=0; i<NO_SPARKS; i++){
                        if(packet.id == book_keeper.box_id[i]){                       
                            queue[i][book_keeper.count_per_libera[i]] = packet;
                            /* Only care if this Spark did not send before */
							if(book_keeper.count_per_libera[i] == 0){       
                            	book_keeper.count_per_libera[i] = 1; 
								all++;
							}		                          
                    	} 
					}   
                    print_debug_info("DEBUG: all in %d\n", all);              //remove these lines 	
                    print_addressbook(&book_keeper);
                    print_debug_info("----------------------------------\n");

                    if(all == NO_SPARKS) {
                        clock_gettime(CLOCK_MONOTONIC, &end);
                        
                        printf("Time elapsed %f usec\n", get_elapsed_time_usec(&start, &end));
                        pause();
                    } 
                #endif

                /* Normal Operation cont.'d */
                /* THIS HAS TO BE COMPLETELY CHANGED*/
                #if (!ATTENDANCE_ALARM && !ATTENDANCE_MANUAL)	
		            for(int i=0; i<NO_SPARKS; i++){

		                if(packet.id == book_keeper.box_id[i]){                       
		                    queue[i][book_keeper.count_per_libera[i]] = packet ;            // REGISTER 0 .. 29 ( 30 PACKETS )
		                    book_keeper.count_per_libera[i] += 1;                           // INC RIGHT AFTER 1.. 30
		                }

                        // this condition needs to change
                        if(book_keeper.count_per_libera[i] == (FRAME_COMPLETE)){            // TRUE WHEN LAST COUNT = 30 
                            /* Start summing */
                            memset(payload_sums, 0 , sizeof(payload_sums));
		                    for(int j = 0; j < 30; j++){ 
                                for(int ind = 0; ind< PAYLOAD_FIELDS; ind++){
                                    payload_sums[ind] += queue[i][j].liberaData[ind];
                                }                           
		                    }

                            /* Keep the original format (64 byte int array) */
                            for(int ind = 0; ind< PAYLOAD_FIELDS; ind++){
                                compact_payload[ind] = (int) payload_sums[ind]/30;
                            }

							// #if DUMP_PAYLOAD
		                    //     print_payload(i, vA, vB, vC, vD, SUM, Q, X, Y);
							// #endif

		                    /* Clean up */
		                    book_keeper.count_per_libera[i] = 0;
                            // Payload arrays are never cleaned up
		                }
					}
				#endif
                  
            #endif

        }else{
            perror("recvfrom()");
			if(errno == EINTR){
				printf("error was EINTR\n");
			}				
            //return -1;
        }
    } 

    /* Deallocate the socket*/
    close(socket_desc);

    return 0;
}
