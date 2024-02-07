#ifndef NET_HELPERS_H
#define NET_HELPERS_H

/* Network */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>      // perror()

/************************************
 * TYPEDEFS
 ************************************/

struct Message{
		int spark_id;
        int payload[16];
        int payload_all[128];
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