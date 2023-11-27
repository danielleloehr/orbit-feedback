#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>      // perror()
#include <time.h>       // clock_gettime()
#include <unistd.h>     // close()


static char IP_ADDR[16] = "192.168.1.200";//"192.168.2.22";       /* Server's Internet Address  */
static int PORT = 2049;                         /* Libera default port      */
static int PACKET_MAX = 1000;                   /* Number of packets to accept until socket is closed*/


void hexdumpunformatted(void *ptr, int buflen) {
  unsigned char *buf = (unsigned char*)ptr;
  int i, j;
  for (i=0; i<buflen; i++) {
    printf("%02x ", buf[i]);  //nospace
    printf(" ");
  }
}

void hexdumpunformatted(void *ptr, int buflen);

int main (){
    int socket_desc, namelen, client_addr_size;
    struct sockaddr_in server, client;
    char buf[64];
    struct timespec time_arrival = {0,0};
    int packet_limit = PACKET_MAX;

    /* Create a datagram socket*/
    if ((socket_desc = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("socket()");
         return -1;
    }

    /* Bind server to this socket.*/
    server.sin_family      = AF_INET;  /* Server is in Internet Domain */
    server.sin_port        = htons(PORT);         
    server.sin_addr.s_addr = inet_addr(IP_ADDR); 


    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0){
        perror("bind()");
        return -1;
    }

    /* Find out what port was really assigned and print it */
    // namelen = sizeof(server);
    // if (getsockname(socket_desc, (struct sockaddr *) &server, &namelen) < 0){
    //     perror("getsockname()");
    //      return -1;
    // }

   // printf("Port assigned is %d\n", ntohs(server.sin_port));
    
    client_addr_size = sizeof(client);

    int liberaData[16];

    printf("sec, nanosec\n");
    while(packet_limit > 0){
        if(recvfrom(socket_desc, buf, sizeof(buf), 0, (struct sockaddr *) &client,
        &client_addr_size) >= 0){
            clock_gettime(CLOCK_MONOTONIC, &time_arrival);
            // printf("%.5f\n",(double)time_arrival.tv_sec + 1.0e-9 * time_arrival.tv_nsec);
            // printf("%ld, %ld\n", time_arrival.tv_sec, time_arrival.tv_nsec);
            //packet_limit--;
        }else{
            perror("recvfrom()");
            return -1;
        }    
    

    
    memcpy(liberaData, buf, 16*sizeof(int));
    for(int i= 0; i<16; i++){
        printf("%d\t", liberaData[i]);
        
    }
    printf("\n");
    //hexdumpunformatted(buf, 64);            //this actually works 
    
    }   

    //     printf("Received message %s from domain %s port %d internet\
    // address %s\n", buf, 
    // (client.sin_family == AF_INET?"AF_INET":"UNKNOWN"),
    // ntohs(client.sin_port), 
    // inet_ntoa(client.sin_addr));

    /* Deallocate the socket*/
    close(socket_desc);

    return 0;
}
