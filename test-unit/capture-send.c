/*
 VERSION 00.01.1
 fa-capture +
*/

#include "test_utils.h"
#include "bpm_utils.h"

/******************************/
/* OPERATION CONTROL          */
#define LATENCY_PERF 	    0
#define REGISTER 		    1
#define ATTENDANCE_ALARM    0
#define ATTENDANCE_MANUAL   0
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
    /* Payload Compression                      */
    /********************************************/
    long int payload_sums[PAYLOAD_FIELDS];          // this could be bypassed in the future
    int compact_payload[PAYLOAD_FIELDS]; 


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

    // send test
    int s;
    unsigned short local_port;
    struct sockaddr_in local_server;

    local_port = htons(2049);

    /* Create a datagram socket in the internet domain and use the
    * default protocol (UDP).
    */
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("socket()");
        return -1;
    }

    /* Set up the server name */
    local_server.sin_family      = AF_INET;            /* Internet Domain    */
    local_server.sin_port        = local_port;               /* Server Port        */
    local_server.sin_addr.s_addr = inet_addr("192.168.1.200"); /* Server's Address   */
    
    
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

                            sendto(s, compact_payload, sizeof(compact_payload), 0, (struct sockaddr *)&local_server, sizeof(local_server));

							#if DEBUG_PAYLOAD
		                        print_payload(i, vA, vB, vC, vD, SUM, Q, X, Y);
							#endif
            
                            /* Clean up */
		                    book_keeper.count_per_libera[i] = 0;
        
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

