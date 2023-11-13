/*
 VERSION 00.01.1
 DO NOT TOUCH THIS VERSION
*/

#include "test_utils.h"
#include "bpm_utils.h"

/******************************/
/* OPERATION CONTROL          */
#define LATENCY_PERF 	    0
#define REGISTER 		    1
#define ATTENDANCE_ALARM    0
#define ATTENDANCE_MANUAL   1
#define DEBUG_PAYLOAD    	0
/******************************/

void init_bookkeeper(struct bookKeeper *book_keeper);
int prepare_socket(struct sockaddr_in server);
void print_payload(int n, long int vA, long int vB, long int vC, long int vD, long int SUM, 
                    long int Q, long int X, long int Y);
void print_addressbook(struct bookKeeper *book_keeper);


void display_current_config(void) {
    char line[16];
    
    printf("---------------------------------------\n");
    printf("Current Capture Configuration\n");
    printf("Latency Performance Recording: "); 
    fancy_print(LATENCY_PERF);

    printf("Compressed Payload Dump: ");
    fancy_print(DEBUG_PAYLOAD);  
 
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


int main (){    
    
    display_current_config();       
    /********************************************/
    /* New Payload                              */
    /********************************************/
    long int vA = 0;
	long int vB = 0;
	long int vC = 0;
	long int vD = 0;
    long int SUM = 0;
    long int Q = 0;    
    long int X, Y = 0;
    int LTM_l, LTM_h;
    int res1, res2, res3, res4, res5;
    int status;

    /********************************************/
    /* Packet Collection Parameters             */
    /********************************************/
    int packet_limit = PACKET_MAX * DURATION * NO_SPARKS;
    struct packetRecord packet;   
    static struct packetRecord queue[NO_SPARKS][FRAME_COMPLETE]; 

    /********************************************/
    /* Socket for Listening                     */
    /********************************************/
    int socket_desc; 
    struct sockaddr_in server;
    socket_desc = prepare_socket(server);
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
    struct bookKeeper book_keeper;
    init_bookkeeper(&book_keeper);

    /**/

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
    
    
    while(packet_limit > 0){
        if(recvfrom(socket_desc, buf, sizeof(buf), 0, (struct sockaddr *) &client, &client_addr_size) >= 0){

            /* Record the packet (only valid for extended structures) */
            clock_gettime(CLOCK_MONOTONIC, &packet.arrival);      
           
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
					signal(SIGALRM, signal_handler);
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
                    printf("DEBUG: all in %d\n", all);              //remove these lines 	
                    print_addressbook(&book_keeper);
                    printf("----------------------------------\n");

                    if(all == NO_SPARKS) {
                        clock_gettime(CLOCK_MONOTONIC, &end);
                        
                        printf("Time elapsed %f usec\n", get_elapsed_time_usec(&start, &end));
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
					
		                    for(int j = 0; j < 30; j++){
		                        vA  += queue[i][j].liberaData[0];
		                        vB  += queue[i][j].liberaData[1];
		                        vC  += queue[i][j].liberaData[2];
		                        vD  += queue[i][j].liberaData[3];
		                        SUM += queue[i][j].liberaData[4];
		                        Q   += queue[i][j].liberaData[5];
		                        X   += queue[i][j].liberaData[6];
		                        Y   += queue[i][j].liberaData[7];
		                    }

							#if DEBUG_PAYLOAD
		                        print_payload(i, vA, vB, vC, vD, SUM, Q, X, Y);
							#endif

		                    /* Clean up */
		                    book_keeper.count_per_libera[i] = 0;
		                    vA = 0;
		                    vB = 0;
		                    vC = 0;
		                    vD = 0;
		                    SUM = 0;
		                    Q = 0;
		                    X = 0;
		                    Y = 0;
		                }
					}
				#endif
                  
            #endif
            packet_limit--;

        }else{
            perror("recvfrom()");
            return -1;
        }
    } 

    /* Deallocate the socket*/
    close(socket_desc);

    return 0;
}

