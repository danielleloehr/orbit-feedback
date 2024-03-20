#include <stdio.h>
#include <errno.h>      // perror()

#include "sockets.h"


int prepare_socket(struct sockaddr_in server) {
    int socket_desc;

    /* Create a datagram socket*/
    if ((socket_desc = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("socket()");
        return -1;
    }
    
    /* Bind server to this socket.*/
    server.sin_family      = AF_INET;       /* Server is in Internet Domain */
    server.sin_port        = htons(MY_PORT);         
    server.sin_addr.s_addr = inet_addr(MY_IP_ADDR); 

    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0){
        perror("bind()");
        return -1;
    }
    
    return socket_desc;
}