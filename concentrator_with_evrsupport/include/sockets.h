#ifndef SOCKETS_H
#define SOCKETS_H

/* Network */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

/************************************
 * TYPEDEFS
 ************************************/
struct Message{
    char spark_id[5];
    int payload[16];
};

/************************************
 * EXPORTED VARIABLES
 ************************************/
/* Server's Internet Address  */
static char IP_ADDR[16] = "1.1.2.1";    
/* Libera default port      */
static int PORT = 2048;

/************************************
 * FUNCTION PROTOTYPES
 ************************************/
int prepare_socket(struct sockaddr_in server);

#endif