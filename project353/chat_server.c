/* Skeleton code for the server side code. 
 * 
 * Compile as follows: gcc -o chat_server chat_server.c -std=c99 -Wall -lrt
 *
 * Author: Naga Kandasamy
 * Date created: January 28, 2020
 *
 * Student/team name: FIXME
 * Date created: FIXME  
 *
 */

#include <signal.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "msg_structure.h"

/* FIXME: Establish signal handler to handle CTRL+C signal and 
 * shut down gracefully. 
 */

int 
main (int argc, char **argv) 
{
    /* Create a well-known FIFO and open it for reading. The server 
     * must be run before any of its clients so that the server FIFO exists by the 
     * time a client attempts to open it. The server's open() blocks until the first client 
     * opens the other end of the server FIFO for writing. 
     * */
    umask (0);
    if (mkfifo (SERVER_FIFO, S_IRUSR | S_IWUSR | S_IWGRP) == -1 \
        && errno != EEXIST) {
        perror ("mkfifo");
        exit (EXIT_FAILURE);
    }
   int server_fd = open (SERVER_FIFO, O_RDONLY);
   if (server_fd == -1) {
       perror ("open");
       exit (EXIT_FAILURE);
   }

   /* Open the server's FIFO once more, this time for writing. This will not block 
    * since the FIFO has already been opened for reading. This operation is done 
    * so that the server will not see EOF if all clients close the write end of the 
    * FIFO.  
    */
   int dummy_fd = open (SERVER_FIFO, O_WRONLY);
   if (dummy_fd == -1) {
       perror ("open");
       exit (EXIT_FAILURE);
   }

   /* Ignore the SIGPIPE signal. This way if the server attempts to write to a client 
    * FIFO that doesn't have a reader, rather than the kernel sending it the SIGPIPE 
    * signal (which will by default terminate the server), it receives an EPIPE error 
    * from the write () call. 
    */
   if (signal (SIGPIPE, SIG_IGN) == SIG_ERR) {
       perror ("signal");
       exit (EXIT_FAILURE);
   }

   /* Loop that reads and responds to each incoming client request. To send the 
    * response, the server constructs the name of the client FIFO and then opens the 
    * FIFO for writing. If the server encounters an error in opening the client FIFO, 
    * it abandons that client's request and moves on to the next one.
    */
   int client_fd;
   char client_fifo[CLIENT_FIFO_NAME_LEN];
    struct client_msg req; //request
    struct server_msg resp; //response
	 resp.number_client = 0;
  // int seq_num = 0; /* This is the service that we provide as a server */

   while (1) {
       if (read (server_fd, &req, sizeof (struct client_msg)) != sizeof (struct client_msg)) {
           fprintf (stderr, "SERVER: Error reading request; discarding \n");
           continue;
       }
	
       /* Construct the name of the client FIFO previously created by the client and 
        * open it for writing. */
       snprintf (client_fifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, req.user_name/*(long)req.client_pid*/);
       printf ("SERVER: Opening client FIFO %s \n", client_fifo);

       client_fd = open (client_fifo, O_WRONLY);
       if (client_fd == -1) {    /* Open failed on the client FIFO. Give up and move on. */
           printf ("SERVER: Error opening client fifo %s \n", client_fifo);
           continue;
       }

       /* Send the response to the client and close FIFO */
        strcpy (resp.msg, req.msg);
		  strcpy (resp.sender_name, req.user_name);
		  resp.number_client += req.control;
       
       if (write (client_fd, &resp, sizeof (struct server_msg)) != sizeof (struct server_msg))
           fprintf (stderr, "Error writing to client FIFO %s \n", client_fifo);
       if (close (client_fd) == -1)
           fprintf (stderr, "Error closing client FIFO %s \n", client_fifo);

       /* Update the sequence number */
   }
    exit (EXIT_SUCCESS);
}
