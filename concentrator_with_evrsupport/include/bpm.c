#include "bpm.h"

/* Initiliases the book keeper entity */
//void init_bookkeeper(struct bookKeeper *book_keeper, char *address_book[]) {
void init_bookkeeper(struct bookKeeper *book_keeper, char *sel) {
    /* Copy from hard-coded address book */    
    for(int i=0; i<NO_SPARKS; i++){
        book_keeper->count_per_libera[i] = 0;   
        book_keeper->buffer_index[i] = 0; 
        //book_keeper->box_id[i] = inet_addr(ADDRESS_BOOK[i]);

        if(strcmp("mtca1c1s1s14g", sel) == 0){
            book_keeper->box_id[i] = inet_addr(mtca1c1s1s14g_addressbook[i]);
        }else if(strcmp("injector", sel) == 0){
            book_keeper->box_id[i] = inet_addr(injector_test[i]);
        }else{
            printf("No address book is selected\n");
            exit(1);
        }       
    } 
}

/* Please keep up-to-date. */
void list_addressbooks(void){
    printf("Available sections: \n \tmtca1c1s1s14g (7 SPARKS), injector (32 SPARKS)\n");
}

char *get_ipaddr_printable(int ip, int lastbyte){
    static char ipaddress[16];
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;   
   
    if(lastbyte){
        snprintf(ipaddress, sizeof(ipaddress), "%d", bytes[3]);
    }else{
        snprintf(ipaddress, sizeof(ipaddress), "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);
    }    
    return ipaddress;    
}



void print_addressbook(struct bookKeeper *book_keeper) {
    printf("Address book: \n");  
    for(int i = 0; i< NO_SPARKS; i++){ 
        printf("%d : Box IP: %s\t No. of Packets: %d\n" , 
            i, get_ipaddr_printable(book_keeper->box_id[i], 0), (book_keeper->count_per_libera[i]));
    }
    printf("\n");
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
