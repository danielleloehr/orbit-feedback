/*
 fa-capture under test
   VERSION 00.02.1 
   Advanced functionality. Replaces uut-capture.c
   Disclaimer: I don't remember if this was completely tested (18/12/23)
*/

#include "test_utils.h"
#include "bpm.h"
#include "sockets.h"

/******************************/
/* OPERATION CONTROL          */
#define LATENCY_PERF 	    0
#define REGISTER 		    1
#define ATTENDANCE_ALARM    0
#define ATTENDANCE_MANUAL   0
#define DUMP_PAYLOAD    	1
#define IRQ_CNTRL_TEST      0      
#define CPU_CORE            1
/******************************/


/* Interrupt Control                                */
static int32_t irq_count;
static int fd;
static int loop_control;

/* Attendance Control                               */
static int all_in;
struct timespec start_attend, end_attend;  

/* Address book                                     */
/* Init in main(), no one else changes the contents */
struct bookKeeper book_keeper;          

/* Libera Spark queues for averager                 */
/* No concurrent access needed                      */
static struct packetRecord queue[NO_SPARKS][FRAME_COMPLETE];


/* Universal tick-tock struct for IRQ timing        */
/* CAREFUL: 
    tic is set by the "STOP" thread.    
    toc is set after the inner while loop is broken */
struct timespec tic, toc;

/* */
long int payload_sums[PAYLOAD_FIELDS];          // this could be bypassed in the future

/********************************************/
/* New Payload  - Test setup compatibility  */
/********************************************/
long int vA, vB, vC, vD, SUM, Q, X, Y;
int LTM_l, LTM_h;
int res1, res2, res3, res4, res5;
int status;

/****************************************************/

void init_bookkeeper(struct bookKeeper *book_keeper);
int prepare_socket(struct sockaddr_in server, int listen);
void print_payload(int compact_payload[PAYLOAD_FIELDS]);
void print_addressbook(struct bookKeeper *book_keeper);
int select_affinity(int core_id);  


/* This will stop the wave when "STOP" thread returns */
void thread_handler(int signum){
	print_debug_info("DEBUG: Stop right there!\n");
    /* Clock toc here, */
	clock_gettime(CLOCK_MONOTONIC, &toc);
	loop_control = 0;
}


/* Register IRQ */
void *thread_register_irq(void *flags){
	select_affinity(CPU_CORE); 
    int notify = (uintptr_t) flags;

    int stop = 0;
    while(!stop) {
        if(read(fd, &irq_count, 4) == 4){
			clock_gettime(CLOCK_MONOTONIC, &tic);
            print_debug_info("DEBUG: IRQ: Interrupt counter: %d\n", irq_count);
            stop = 1;
			if(notify) {						
		        /* If notify is passed, thread will wait for "STOP */
		        loop_control = loop_control - stop;
                /* So far the most deterministic way to break the while loop */
				raise(SIGUSR1);
        	}
        }
    }
    pthread_exit(NULL);	
}


void display_current_config(void) {
    char line[16];
    
    printf("---------------------------------------\n");
    printf("Current Capture Configuration\n");
    #if IRQ_CNTRL_TEST 
        printf("Interrupt Control mode: "); 
        fancy_print(IRQ_CNTRL_TEST);
        printf("%s\n", "\033[91mIf in TEST PHASE, please make sure all other options are deactivated!\033[0m"); 
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

    printf("Settings for Packet Forwarding\n");
    printf("Forward to: %s on Port %d\n", LOCAL_ADDR, LOCAL_PORT);      //test

    printf("---------------------------------------\n");
    
    #if ATTENDANCE_ALARM && ATTENDANCE_MANUAL
        printf("%s\n", "\033[91mWarning: Please select exactly ONE attendance mode!\033[0m");
        printf("Exiting now..\n");
        exit(EXIT_FAILURE);
    #endif

    #if (ATTENDANCE_ALARM || ATTENDANCE_MANUAL) && !REGISTER
        printf("%s\n", "\033[91mWarning: REGISTER must be set for attendance check!\033[0m");
        printf("Exiting now..\n");
        exit(EXIT_FAILURE); 
    #endif

    #if DUMP_PAYLOAD && (ATTENDANCE_ALARM || ATTENDANCE_MANUAL)
        printf("%s\n", "\033[91mWarning: Dumping payload is not allowed in attendance mode!\033[0m");
        printf("Exiting now..\n");
        exit(EXIT_FAILURE); 
    #endif

    /* Warning for sudo rights */
    if(check_sudo() != 0){
        printf("%s\n", "\033[91mProceed as non-root user.\033[0m\n");
    }else{
        printf("Proceed as sudo.\n");
    }

    /* no warnings so far */
    printf("Capture will commence...\n");

    //sleep(1);   // Give people some time to digest this
    printf("Press ENTER key to Continue\n");  
    getchar();      
}


static inline int start_capture(int socket_desc, int *buf, struct sockaddr_in client, int client_addr_size, struct packetRecord packet, 
                                int local_desc, struct sockaddr_in local_server, int compact_payload[PAYLOAD_FIELDS]){
    if(recvfrom(socket_desc, buf, sizeof(buf), 0, (struct sockaddr *) &client, &client_addr_size) >= 0){
        /* Record the packet (only valid for extended structures) */
        clock_gettime(CLOCK_MONOTONIC, &packet.arrival); 

        // Just print the timestamp so we know something is happening
        printf("%f\n",(double)packet.arrival.tv_sec + 1.0e-9 * packet.arrival.tv_nsec);
        
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
                            all_in++;
                        }		                          
                    } 
                }   
                print_debug_info("DEBUG: all in %d\n", all_in);              //remove these lines 	
                print_addressbook(&book_keeper);
                print_debug_info("----------------------------------\n");

                if(all_in == NO_SPARKS) {
                    clock_gettime(CLOCK_MONOTONIC, &end_attend);
                    
                    printf("Time elapsed %f usec\n", get_elapsed_time_usec(&start_attend, &end_attend));
                    pause();
                } 
            #endif

            /* Normal Operation cont.'d */
            #if (!ATTENDANCE_ALARM && !ATTENDANCE_MANUAL)	
                for(int i=0; i<NO_SPARKS; i++){

                    if(packet.id == book_keeper.box_id[i]){                       
                        queue[i][book_keeper.count_per_libera[i]] = packet ;            // REGISTER 0 .. 29 ( 30 PACKETS )
                        book_keeper.count_per_libera[i] += 1;                           // INC RIGHT AFTER 1.. 30
                    }

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

                        sendto(local_desc, compact_payload, sizeof(compact_payload), 0, (struct sockaddr *)&local_server, sizeof(local_server));

                        #if DUMP_PAYLOAD
                            print_payload(compact_payload);
                        #endif
    
                        /* Clean up */
		                book_keeper.count_per_libera[i] = 0;

                        // for(int j = 0; j < 30; j++){
                        //     vA  += queue[i][j].liberaData[0];
                        //     vB  += queue[i][j].liberaData[1];
                        //     vC  += queue[i][j].liberaData[2];
                        //     vD  += queue[i][j].liberaData[3];
                        //     SUM += queue[i][j].liberaData[4];
                        //     Q   += queue[i][j].liberaData[5];
                        //     X   += queue[i][j].liberaData[6];
                        //     Y   += queue[i][j].liberaData[7];
                        // }

                        // #if DUMP_PAYLOAD
                        //     print_payload(i, vA, vB, vC, vD, SUM, Q, X, Y);
                        // #endif

                        // /* Clean up */
                        // book_keeper.count_per_libera[i] = 0;
                        // vA = 0;
                        // vB = 0;
                        // vC = 0;
                        // vD = 0;
                        // SUM = 0;
                        // Q = 0;
                        // X = 0;
                        // Y = 0;
                    }
                }
            #endif
            
        #endif
        return 1;       

    }else{
        perror("recvfrom()");
        return 0;       // -1 control accept param
    }
}


int main(){    

    display_current_config();  

    /********************************************/
    /* Payload Compression                      */
    /********************************************/
    int compact_payload[PAYLOAD_FIELDS];                    /*TODO here too*/

    /********************************************/
    /* Packet Collection Parameters             */
    /********************************************/
    int packet_limit = PACKET_MAX * DURATION * NO_SPARKS;
    int accept = 0;
    struct packetRecord packet;    
    // --queue used to be here

    /********************************************/          /*TODO*/
    /* Socket for Sending                       
       Test phase naming convention             */
    /********************************************/
    int local_desc; 
    struct sockaddr_in local_server;
    local_desc = prepare_socket(local_server, 0); 
    if (local_desc == -1){
        perror("Socket binding failed. Packets are not forwarded.\n");
    }
    
    /********************************************/
    /* Socket for Listening                     */
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

    /********************************************/
    /* Address Book                             */ 
    /********************************************/
    //--bookkeeper used to be here
    init_bookkeeper(&book_keeper);

    /********************************************/
    /* Thread Control                           */
    /********************************************/
	pthread_t start_cond, stop_cond;
    int *notify_on = (int *) 1;                         // Don't make this global!
    /* Register "STOP" signal                   */
    signal(SIGUSR1, thread_handler);

    /* MAIN */
    #if LATENCY_PERF
        /* Print this to make our lives bit easier when we later parse with pandas */
        printf("sec, nanosec\n");
    #endif

    /*******************************************/
    /* Some preparation for ATTENDANCE MANUAL  */
    /* Please do NOT move (time-critical)      */
    /* Mind the additional delay               */
    /*******************************************/
    clock_gettime(CLOCK_MONOTONIC, &start_attend);

    // >>THIS IS TEST CODE. IT WILL BE REMOVED.
    #if !IRQ_CNTRL_TEST          
        while(packet_limit > 0){
            accept = start_capture(socket_desc, buf, client, client_addr_size, packet, local_desc, local_server, compact_payload);
            packet_limit = packet_limit - accept;
        }      
    #else
        /********************************************/          /* this for test phase */
        /* Open UIO device file                     */
        /********************************************/
        fd = open("/dev/uio0", O_RDWR);                             
        if (fd < 0) {
            perror("uio open: \n");
            return errno;
        }

        do{
            print_debug_info("DEBUG: Waiting for start signal...\n");
            /* Start thread for "START" signal */
            pthread_create(&start_cond, NULL, thread_register_irq, NULL); 
            /* Wait for blocking read to complete */
            (void) pthread_join(start_cond, NULL);    

            loop_control = 1;    
            /* Start thread for "STOP" signal */
            pthread_create(&stop_cond, NULL, thread_register_irq, (void *)notify_on); 
            
            while(loop_control){
                start_capture(socket_desc, buf, client, client_addr_size, packet, local_desc, local_server, compact_payload);          // no pointers --
            }

            print_debug_info("\n*******************************************\n");
            //clock_gettime(CLOCK_MONOTONIC, &toc);
            double elapsed = get_elapsed_time_usec(&tic, &toc);
            print_debug_info("DEBUG: Time elapsed \n thread detected an IRQ --- while loop stop via cont:\n %f usec \n", elapsed);
            print_debug_info("*******************************************\n");

            pthread_detach(stop_cond);      // cleanup, probably not necessary

        }while(1);
    #endif 

    /* Deallocate the socket*/
    close(socket_desc);

    return 0;
}
