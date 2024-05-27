/*
 fa-capture under test
   VERSION 00.05.1  -- Richards version with mrfioc2 support
                 ____________________                   __________________
                |                    |                 |                  |
 Spark 1..8 --->|  Concentrator CPU  |---compressed--->|   Remote Server  |
                |      (Richard)     |      data       |  (dest IP/port)  | 
                |____________________|                 |__________________|


*/

#include "test_utils.h"		// keep this here GNU_SOURCE
#include "bpm.h"
#include "sockets.h"

#include <ctype.h>
#include <getopt.h>

/******************************/
/* OPERATION CONTROL          */
#define LATENCY_PERF 	    0
#define REGISTER 		    1
#define EVR_IRQ             1
#define SOFT_IRQ            0
#define CPU_CORE            1
#define ALARM_USEC       	6666.66

#define TESTMODE            0
/* DEBUG_ON can be toogled in test_utils */
/******************************/

/* Please adjust these parameters carefully */
/* queue size per Spark  */
#define MAX_BUFF_SIZE 1000000       
/* an estimated value for averaging in the case of an overflow */
#define EST_PACKET_COUNT 10000      

static struct packetRecord queue[NO_SPARKS][MAX_BUFF_SIZE];       

static volatile int send_data;
int evr_fd;

/******************************/
/* Commandline arguments      */
static char *dest_ip;
static int dest_port;
static char *select_bpms;
static int debug_payload;
/******************************/

static char *DEFAULT_ADDR = "192.168.21.34"; 
static int DEFAULT_PORT = 2049;
static char *DEFAULT_BPM_SELECTION = "mtca1c1s1s14g";
static int DEFAULT_DEBUG = 0;

/* Additional test variables */
static int GLOBAL_PACKET_COUNTER;
static int GLOBAL_SEND_COUNTER;

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

/* Careful: queue must be global */
void compress_and_send(struct bookKeeper *spark_bookkeeper, int trans_sock, struct sockaddr_in transmit_server){
    clock_gettime(CLOCK_MONOTONIC, &toc);
    // more than 1 second, reset the counter 
    // if((toc.tv_sec - tic.tv_sec) >= 1){
    //     GLOBAL_SEND_COUNTER = 0;
    // }            

    /********************************************/
    /* Original Payload :                                
      vA | vB | vC | vD | Sum | Q | X | Y | LMT_l | LMT_h | res.| res.| res.| res.| res.| status
        0|   1|   2|   3|    4|  5|  6|  7|      8|      9|   10|   11|   12|   13|   14|     15|  */
    /********************************************/
    long long payload_sums[PAYLOAD_FIELDS];                         
    int compact_payload[PAYLOAD_FIELDS];
    
    int arrayXY_all[NO_SPARKS*2 + 2];

    for(int i=0; i < NO_SPARKS; i++){
        memset(payload_sums, 0 , sizeof(payload_sums));

        // Overwrite the count value if there was an overflow to average over only the latest packets in the full queue
        int buffer_overflow = spark_bookkeeper->count_per_libera[i] - MAX_BUFF_SIZE;    
        if(buffer_overflow >= 0 ){
            printf("WARNING: Buffer overflow (or edge condition) is reported.\n");
            if(!(buffer_overflow % MAX_BUFF_SIZE)){
                print_debug_info("DEBUG: Edge Cond. Averaging over MAX_BUFF_SIZE (=%d)\n", MAX_BUFF_SIZE);
                spark_bookkeeper->count_per_libera[i] = MAX_BUFF_SIZE;
            }else{
                print_debug_info("DEBUG: Overflow. Averaging over fixed packet count (=%d)\n", EST_PACKET_COUNT);
                spark_bookkeeper->count_per_libera[i] = EST_PACKET_COUNT;
            }
        }
		
        for(int curr_ind = 0; curr_ind < spark_bookkeeper->count_per_libera[i]; curr_ind++){ 
            for(int payload_ind = 0; payload_ind < PAYLOAD_FIELDS; payload_ind++){
                // Don't sum indices 8(LTM_l), 9(LTM_h) and 15(status), just use the fields of the latest packet
                if (payload_ind == 8 || payload_ind == 9 || payload_ind == 15){
                    payload_sums[payload_ind] = queue[i][curr_ind].liberaData[payload_ind];      
                }else{
                    payload_sums[payload_ind] += queue[i][curr_ind].liberaData[payload_ind];
                }
            }                           
        }		
        
        // Format it the way it was before
        for(int ind = 0; ind< PAYLOAD_FIELDS; ind++){
            // Don't divide values = 0, and indices 8(LTM_l), 9(LTM_h) and 15(status)
            if(payload_sums[ind] == 0 || ind == 8 || ind == 9 || ind == 15){ 
                compact_payload[ind] = (int) payload_sums[ind]; 
            }else{
                compact_payload[ind] = (int) (payload_sums[ind]/spark_bookkeeper->count_per_libera[i]);                
            }
        }

        arrayXY_all[i] = compact_payload[6];
        arrayXY_all[i+7] = compact_payload[7];
        arrayXY_all[14] =  compact_payload[8];                 // LTM_l
        arrayXY_all[15] =  GLOBAL_SEND_COUNTER;                // packet count param
        /* If you want to send individual compact payload, uncomment this */
        // sendto(trans_sock, compact_payload, sizeof(compact_payload), 0, (struct sockaddr *)&transmit_server, sizeof(transmit_server));

        if(debug_payload){
            print_payload(compact_payload);
        }

        // Clean up
        spark_bookkeeper->count_per_libera[i] = 0;
        // Always overwrite the buffer. 
        spark_bookkeeper->buffer_index[i] = 0;         
    }
    
    /* Send all compressed X[0,1,2,3,4,5,6] and Y[7,8,9,10,11,12,13] values */
    sendto(trans_sock, arrayXY_all, sizeof(arrayXY_all), 0, (struct sockaddr *)&transmit_server, sizeof(transmit_server));

    GLOBAL_SEND_COUNTER++;  
    // tic 
    clock_gettime(CLOCK_MONOTONIC, &tic); 
}

/* Print out curernt settings and relevant parameters */
void display_current_config(void) {
    printf("---------------------------------------\n");
    printf("Current Capture Configuration\n");
    
    printf("Latency Performance Recording: "); 
    humanise(LATENCY_PERF);
    printf("Packet Registery: ");
    humanise(REGISTER);

    if(EVR_IRQ && SOFT_IRQ){
        myprintf(ST_warning, 's', "Please choose one interrupt control option!");
        exit(EXIT_FAILURE);
    }

    printf("Interrupt Control Mode: ");
    if(EVR_IRQ) myprintf(ST_attention, 's', "EVR_IRQ");
    if(SOFT_IRQ) myprintf(ST_attention, 's', "EVR_IRQ");

    #if TESTMODE
        printf("Packet Collection Parameters\n");
        printf("Estimated Packet Traffic per Box: %d pps\n", PACKET_MAX);
        printf("Capture Duration: %d\n", DURATION);
    #endif
    printf("\n");

    printf("Destination Configuration of Sparks\n");
    printf("Local IP: ");
    myprintf(ST_attention, 's', MY_IP_ADDR);
    printf("Local Port: ");
    myprintf(ST_attention, 'd', MY_PORT);
    printf("\n");

    printf("Sending Concentrated BPM Data to:\n");
    printf("Remote IP: ");
    myprintf(ST_attention, 's', dest_ip);
    printf("Remote Port: ");
    myprintf(ST_attention, 'd', dest_port);
    printf("\n");

    printf("Number of Connected Sparks: ");
    myprintf(ST_attention, 'd', NO_SPARKS);
    printf("Current BPM Section Selection: ");
    myprintf(ST_attention, 's', select_bpms);
    printf("Dumping concentrated packet payload: ");
    humanise(debug_payload);
    printf("\n");

    printf("CPU core selection for IRQ handling: CPU %d\n", CPU_CORE);

    printf("---------------------------------------\n");
    
    /* no warnings so far */
    printf("Capture will commence...\n");

    sleep(1);   // Give people some time to digest this
}


void parse_command_arguments(int argc, char *argv[]){
    int command;

    while ((command = getopt (argc, argv, "i:p:b:hdl")) != -1){
        switch (command){    
            case 'h':
                printhelp();
                break; 
            case 'd':
                debug_payload = 1;
                break;
            case 'l':
                list_addressbooks();
                exit(EXIT_SUCCESS);
                break;
            case 'i':
                dest_ip = optarg;
                break;
            case 'p':
                dest_port = atoi(optarg);
                break;
            case 'b':
                select_bpms = optarg;
                break;
            case '?':
                if (optopt == 'c')
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint (optopt))
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
                exit(EXIT_FAILURE);
        default:
            abort();
        }
    }
    // printf("Commandline: dest_ip = %s, dest_port = %d, bpm section selection = %s debug = %d\n",
    //         dest_ip, dest_port, select_bpms, debug_payload);
}


int main(int argc, char *argv[]){   

    dest_ip = DEFAULT_ADDR;
    dest_port = DEFAULT_PORT;
    select_bpms = DEFAULT_BPM_SELECTION; 
    debug_payload = DEFAULT_DEBUG;

    parse_command_arguments(argc, argv);  
    display_current_config(); 
    
    /********************************************/
    /* Packet Collection                        */
    /********************************************/
    static struct packetRecord packet;   
    struct bookKeeper spark_bookkeeper;
    //init_bookkeeper(&spark_bookkeeper, mtca1c1s1s14g_addressbook);
    init_bookkeeper(&spark_bookkeeper, select_bpms);
    print_addressbook(&spark_bookkeeper);

    /* TEST MODE */
    #if TESTMODE
        int packet_limit = PACKET_MAX * DURATION * NO_SPARKS;
    #endif
    
    pthread_t irq_thread_id;
    GLOBAL_PACKET_COUNTER = 0;
    GLOBAL_SEND_COUNTER = 0;
    
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
    /* Socket to collect from Sparks            */
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
    /* Socket to send all concentrated packets  */
    /********************************************/
    int sock_concentrated;
    struct sockaddr_in transmit_server;

    if ((sock_concentrated = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("socket()");
        return -1;
    }

    /* Set up the server name */
    transmit_server.sin_family      = AF_INET;             
    transmit_server.sin_port        = htons(dest_port);    
    transmit_server.sin_addr.s_addr = inet_addr(dest_ip); 

    /*******************************************************************************/
    /* MAIN */
    #if LATENCY_PERF
        /* Print this to make our lives bit easier when we later parse with pandas */
        printf("sec, nanosec\n");
    #endif

    /* Start thread for interrupt handling */ 
    #if(EVR_IRQ)
        print_debug_info("DEBUG: Reporting interrupt requests from EVR...\n");
        pthread_create(&irq_thread_id, NULL, (void *) uio_read, NULL);
    #endif

    #if(SOFT_IRQ)
        ualarm(ALARM_USEC, 0);
	    signal(SIGALRM, concentration_pacemaker);
    #endif

    while(1){
        if(send_data){
            compress_and_send(&spark_bookkeeper, sock_concentrated, transmit_server);
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
                memcpy(packet.liberaData, buf, PAYLOAD_FIELDS*sizeof(int));  
                packet.src_port = ntohs(client.sin_port);
                packet.id = client.sin_addr.s_addr; 

                for(int i=0; i<NO_SPARKS; i++){
                    if(packet.id == spark_bookkeeper.box_id[i]){             // Get the box ID                   
                        spark_bookkeeper.count_per_libera[i]++;              // Put the packet in the corresponding queue
                        // Check the buffer limits                                 
                        if (spark_bookkeeper.buffer_index[i] < MAX_BUFF_SIZE-1){
                            queue[i][spark_bookkeeper.buffer_index[i]] = packet ;
                            spark_bookkeeper.buffer_index[i]++;
                        }else {
                            spark_bookkeeper.buffer_index[i] = 0;                         // go to beginning
                            queue[i][spark_bookkeeper.buffer_index[i]] = packet ;         // then register the packet
                        }
                    }
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
    close(sock_concentrated);

    return 0;
}