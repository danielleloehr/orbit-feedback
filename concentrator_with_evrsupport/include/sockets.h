#ifndef SOCKETS_H
#define SOCKETS_H

/* Network */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

/************************************
 * EXPORTED VARIABLES
 ************************************/
/* Server's Internet Address 
    (=libera gbe.dest.ip)           */
static char MY_IP_ADDR[16] = "1.1.2.1";    
/* Libera default port      
    (=libera gbe.dest.port)         */
static int MY_PORT = 2048;

/************************************
 * FUNCTION PROTOTYPES
 ************************************/
int prepare_socket(struct sockaddr_in server);

#endif