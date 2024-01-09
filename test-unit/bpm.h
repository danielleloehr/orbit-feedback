/**/
#ifndef BPM
#define BPM

/* LIBERA SPARK parameters */
#define FRAME_COMPLETE 30
#define PAYLOAD_FIELDS 16
#define NO_SPARKS 32

/* Number of packets to accept until socket is closed
    for 1GbE: 10264 pps, 10GbE: 10260 pps*/
#define PACKET_MAX 10260  
#define DURATION 60   /* for 1 minute */

/************************************
 * TYPEDEFS
 ************************************/
/* A packet structure to keep track of receive queues */ 
struct packetRecord{    
    int src_port;   
    int liberaData[PAYLOAD_FIELDS];
    int id; /* IP address of the sender in decimal format */
    struct timespec arrival;
};

/* A book keeper to store our friends' addresses and packet counts */     
struct bookKeeper{
    int count_per_libera[NO_SPARKS];    /* packet counter for each friend */
    int box_id[NO_SPARKS];              /* friends' IP addresses */
    int buffer_index[NO_SPARKS];
};

/************************************
 * EXPORTED VARIABLES
 ************************************/

/* Hardcoded address book*/
static char ADDRESS_BOOK[32][16] = { "1.1.1.202", 	//32
                                     "1.1.1.201", 
                                     "1.1.1.200",	
                                     "1.1.1.199",
                                     "1.1.1.198",
                                     "1.1.1.197",
                                     "1.1.1.196",
                                     "1.1.1.195",
                                     "1.1.1.194",
                                     "1.1.1.193",
                                     "1.1.1.192",
                                     "1.1.1.191",
                                     "1.1.1.190",	//20
                                     "1.1.1.189",
                                     "1.1.1.188",
                                     "1.1.1.187",
                                     "1.1.1.186",
                                     "1.1.1.185",
                                     "1.1.1.184",
                                     "1.1.1.183",
                                     "1.1.1.182",
                                     "1.1.1.181",
                                     "1.1.1.180",	//10
                                     "1.1.1.179",	//09
                                     "1.1.1.178",
                                     "1.1.1.177",
                                     "1.1.1.176",
                                     "1.1.1.175",	//05
                                     "1.1.1.174",
                                     "1.1.1.173",
                                     "1.1.1.172",
                                     "1.1.1.171"
                                     };


/************************************
 * FUNCTION PROTOTYPES
 ************************************/
/* Initiliases the book keeper entity */
void init_bookkeeper(struct bookKeeper *book_keeper) {
    /* Copy from hard-coded address book */
    for(int i=0; i<NO_SPARKS; i++){
        book_keeper->count_per_libera[i] = 0;   
        book_keeper->buffer_index[i] = 0; 
        book_keeper->box_id[i] = inet_addr(ADDRESS_BOOK[i]);
    } 
}

void print_addressbook(struct bookKeeper *book_keeper) {
    printf("Address book: \n");
    for(int i = 0; i< NO_SPARKS; i++){
        printf("%d : Box IP: %u\t No. of Packets: %d\n" , 
            i, (book_keeper->box_id[i]), (book_keeper->count_per_libera[i]));
    }
}

void print_payload(int payload[PAYLOAD_FIELDS]){
    for(int i = 0; i < PAYLOAD_FIELDS; i++){
        printf("%d\t", payload[i]); 
    }
    printf("\n");
}


/* Not used */
void print_payload_test(int n, long int vA, long int vB, long int vC, long int vD, long int SUM, 
                   long int Q, long int X, long int Y) {
    printf("SPARK %d \t:", (32-n));
    printf("vA %ld\t vB %ld\t vC %ld\t vD %ld\t SUM %ld\t Q %ld\t X %ld\t  Y %ld\n", vA/30, vB/30, vC/30, vD/30, SUM/30, Q/30, X/30, Y/30);					
}

#endif