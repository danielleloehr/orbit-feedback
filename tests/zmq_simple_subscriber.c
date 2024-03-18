/* 
    Test server (to be replaced by a central node (Charles)).
     ______________________                           ______________________
    |                      |                         |                      |
    |  Concentrator CPU    |-----compressed data---->|   0MQ Test Server    |
    |      (Richard)       |       Spark 1..8        | (deputy for Charles) | 
    |______________________|                         |______________________|

    Main(and only) Job: have some connections to listen to Richards.
*/

#include <zmq.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <time.h>

/* Message struct consistency */
#include "sockets.h"

/* argc[1] -- Spark filter in format s(xxx) where xxx is the 
    identifier in IP address
    e.g. ./zmq-server s1 to get Spark1 
    when omitted all data is printed */

int main (int argc, char *argv []){   
    /* Socket to communicate with Richards
         see https://zeromq.org/socket-api/ */    
    void *context = zmq_ctx_new ();
    void *subscriber = zmq_socket (context, ZMQ_SUB); 
        
    int rc = zmq_connect (subscriber, "tcp://10.42.0.44:9999");      //Richards address
    const char *filter = (argc > 1)? argv [1]: "";
    zmq_setsockopt (subscriber, ZMQ_SUBSCRIBE, filter, strlen(filter));
    assert (rc == 0);
    perror("zmq_bind to 9999");

    // void *subscriber_no2 = zmq_socket (context, ZMQ_SUB); 
    // zmq_connect (subscriber_no2, "tcp://192.168.2.198:9998");
    // filter = "";
    // zmq_setsockopt (subscriber_no2, ZMQ_SUBSCRIBE, filter, strlen(filter));

     zmq_pollitem_t items [] = {
         { subscriber,   0, ZMQ_POLLIN, 0 }
         //{ subscriber_no2, 0, ZMQ_POLLIN, 0}
     };
    
    struct timespec tic;

    while (1) {
        /* Create an empty 0MQ message */
        struct Message *msg;
        zmq_msg_t zmsg;
        zmq_msg_init (&zmsg);
        //zmq_msg_init_size(&zmsg, sizeof(struct Message));

         zmq_poll(items, 2, -1);
         if (items[0].revents & ZMQ_POLLIN){
            /* Receive message and transfer it to our message struct*/
            zmq_msg_recv(&zmsg, subscriber, 0);
            /* Timestamp */
            clock_gettime(CLOCK_MONOTONIC, &tic);
            printf("Alarm timestamp %.5f\n",(double)tic.tv_sec + 1.0e-9 * tic.tv_nsec);

            msg = (struct Message *)zmq_msg_data(&zmsg);

            printf ("\nSpark %s sent this: \n", msg->spark_id);
            /* Show me the message now */
            for(int i=0; i<16; i++){
                // if (i % 16 == 0){
                //     printf("\n");
                // }
                printf("%d\t", msg->payload[i]);
            }
        
         }
        //  if (items[0].revents & ZMQ_POLLIN){
        //     /* Receive message and transfer it to our message struct*/
        //     zmq_msg_recv(&zmsg, subscriber_no2, 0);
        //     msg = (struct Message *)zmq_msg_data(&zmsg);

        //     printf("This is the other subscriber.\n");
        //     printf ("\nSpark %s sent this: \n", msg->spark_id);
        //     /* Show me the message now */
        //     for(int i=0; i<16; i++){
        //         // if (i % 16 == 0){
        //         //     printf("\n");
        //         // }
        //         printf("%d\t", msg->payload[i]);
        //     }
        
        //  }

        /* Release message */        
        zmq_msg_close (&zmsg);
        /* Rinse and repeat for now */
    }

    /* Never reach */
    return 0;
}