#ifndef SOCKETS_H
#define SOCKETS_H

#include <stdio.h>
#include <errno.h>      // perror()

/* Network */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

/************************************
 * EXPORTED VARIABLES
 ************************************/
/* Server's Internet Address  */
static char IP_ADDR[16] = "1.1.2.1";    
/* Libera default port      */
static int PORT = 2048;

/* Main server information                          
   Forward compressed packets to this address       */
static char LOCAL_ADDR[16] = "192.168.1.200"; 
static int LOCAL_PORT = 2049;


int prepare_socket(struct sockaddr_in server, int listen) {
    int socket_desc;

    /* Create a datagram socket*/
    if ((socket_desc = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("socket()");
        return -1;
    }

    /* Set up Listening if forward is not set to 1*/
    if(listen){
        /* Bind server to this socket.*/
        server.sin_family      = AF_INET;       /* Server is in Internet Domain */
        server.sin_port        = htons(PORT);         
        server.sin_addr.s_addr = inet_addr(IP_ADDR); 

        if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0){
            perror("bind()");
            return -1;
        }
    }else {
        

    }

    return socket_desc;
}

#endif