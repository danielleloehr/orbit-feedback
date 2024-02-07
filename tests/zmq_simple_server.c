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

struct Message{
    char spark_id[5];
    int payload[16];
    //int payload_all[128];       //payload for 8 sparks
};

/* argc[1] -- Spark filter in format s(xxx) where xxx is the 
    identifier in IP address*/
    
int main (int argc, char *argv []){   
    // Socket to communicate with Richards
    /* Note on messaging pattern:
        request and reply doesn't work with our model 
        of communication.
        see https://zeromq.org/socket-api/          */
    void *context = zmq_ctx_new ();
    void *responder = zmq_socket (context, ZMQ_SUB); 
    
    
   // int rd = zmq_bind (responder, "tcp://enx00e04c680048:5555");
    /* some error checking */
   // assert (rd == 0);
  //  perror("zmq_bind to 5555");
    
    int rc = zmq_bind (responder, "tcp://*:9999");  
    const char *filter = (argc > 1)? argv [1]: "s201";
    zmq_setsockopt (responder, ZMQ_SUBSCRIBE, filter, strlen(filter));
    assert (rc == 0);
    perror("zmq_bind to 9999");

    while (1) {
        /* Create an empty 0MQ message */
        struct Message *msg;
        zmq_msg_t zmsg;
        zmq_msg_init (&zmsg);
        //zmq_msg_init_size(&zmsg, sizeof(struct Message));

        /* Receive message and transfer it to our message struct*/
        zmq_msg_recv(&zmsg, responder, 0);
        msg = (struct Message *)zmq_msg_data(&zmsg);

        //const char *user_id = zmq_msg_gets (&zmsg, "Socket-Type");

        printf ("\nSpark %s sent this: \n", msg->spark_id);
        /* Show me the message now */
        for(int i=0; i<16; i++){
            if (i % 16 == 0){
                printf("\n");
            }
            printf("%d\t", msg->payload[i]);
        }
        /* Release message */
        zmq_msg_close (&zmsg);

        /* Rinse and repeat for now */
    }

    /* Never reach */
    return 0;
}