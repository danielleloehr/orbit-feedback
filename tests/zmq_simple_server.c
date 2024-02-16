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

/* Message struct consistency */
#include "sockets.h"

/* argc[1] -- Spark filter in format s(xxx) where xxx is the 
    identifier in IP address*/

int main (int argc, char *argv []){   
    /* Socket to communicate with Richards
         see https://zeromq.org/socket-api/ */    
    void *context = zmq_ctx_new ();
    void *subscriber = zmq_socket (context, ZMQ_SUB); 
    
    /* Bind SUB
         see https://stackoverflow.com/questions/29752338/zeromq-which-socket-should-bind-on-pubsub-pattern */
    // int rd = zmq_bind (another_sub, "tcp://*:5555");
    // /* some error checking */
    // assert (rd == 0);
    // perror("zmq_bind to 5555");
    
    int rc = zmq_bind (subscriber, "tcp://*:9999");
    const char *filter = (argc > 1)? argv [1]: "s202";
    zmq_setsockopt (subscriber, ZMQ_SUBSCRIBE, filter, strlen(filter));
    assert (rc == 0);
    perror("zmq_bind to 9999");

    zmq_pollitem_t items [] = {
        { subscriber,   0, ZMQ_POLLIN, 0 }
    };

    while (1) {
        /* Create an empty 0MQ message */
        struct Message *msg;
        zmq_msg_t zmsg;
        zmq_msg_init (&zmsg);
        //zmq_msg_init_size(&zmsg, sizeof(struct Message));

        zmq_poll(items, 1, -1);
        if (items[0].revents & ZMQ_POLLIN){
            /* Receive message and transfer it to our message struct*/
            zmq_msg_recv(&zmsg, subscriber, 0);
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
        /* Release message */        
        zmq_msg_close (&zmsg);
        /* Rinse and repeat for now */
    }

    /* Never reach */
    return 0;
}