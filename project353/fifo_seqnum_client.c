/* The client side code for requesting sequence numbers from the server. 
 *
 * Compile as follows:
 * gcc -o fifo_seqnum_client fifo_seqnum_client.c -std=c99 -Wall
 *
 * Author: Naga Kandasamy
 * Date created: July 11, 2018
 * Date modified: January 20, 2020
 *
 * Source: M. Kerrisk, Linux Programming Interface
 *
 */

#include "fifo_seqnum.h"

static char client_fifo[CLIENT_FIFO_NAME_LEN];

/* Exit handler for the program. */
static void 
remove_fifo (void)
{
    unlink (client_fifo);
}

int 
main (int argc, char **argv)
{
    /* Read command line argument to the program. */
    if (argc > 1 && strcmp (argv[1], "--help") == 0) {
        printf ("Usage: %s seq-len \n", argv[0]);
        exit (EXIT_SUCCESS);
    }

    /* Create a FIFO using the template to be used for receiving 
     * the response from the server. This is done before sending the 
     * request to the server to make sure that the FIFO exists by the 
     * time the server attempts to open it and send the response message. 
     */
    umask (0);
    snprintf (client_fifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, (long) getpid ());
    printf ("CLIENT: creating FIFO %s \n", client_fifo);
    if (mkfifo (client_fifo, S_IRUSR | S_IWUSR | S_IWGRP) == -1 && errno != EEXIST) {
        perror ("mkfifo");
        exit (EXIT_FAILURE);
    }

    /* Establish an exit handler so that when the client program exits, the FIFO is removed 
     * from the file system. 
     */
    if (atexit (remove_fifo) != 0) {
        perror ("atexit");
        exit (EXIT_FAILURE);
    }

    /* Construct a message for the server containing the client's process ID and a 
     * number taken from the command line argument specifying the length of the 
     * sequence that the client wishes the server to assign to it. If no command line 
     * argument is provided, the default value of one is used. 
     */
    int server_fd, client_fd;
    struct request req;
    struct response resp;

    req.pid = getpid ();
    if (argc > 1)
        req.seq_len = atoi (argv[1]);
    else
        req.seq_len = 1; 

    server_fd = open (SERVER_FIFO, O_WRONLY);
    if (server_fd == -1) {
        printf ("Cannot open server FIFO %s\n", SERVER_FIFO);
        exit (EXIT_FAILURE);
    }

    /*Get input msg*/
    printf("CLIENT: ");
    int m =0;
    while (req.msg[m] = getchar(), req.msg[m]!='\n'){
        m++;
    }
    /* Send the message to the server */
    if (write (server_fd, &req, sizeof (struct request)) != sizeof (struct request)) {
        printf ("Cannot write to server");
        exit (EXIT_FAILURE);
    }

    /* Open our FIFO and read the response from the server. */
    client_fd = open (client_fifo, O_RDONLY);
    if (client_fd == -1) {
        printf ("Cannot open FIFO %s for reading \n", client_fifo);
        exit (EXIT_FAILURE);
    }
    
    if (read (client_fd, &resp, sizeof (struct response)) != sizeof (struct response)) {
        printf ("Cannot read response from server \n");
        exit (EXIT_FAILURE);
    }

    printf ("SERVER: %s\n", resp.msg);
    exit (EXIT_SUCCESS);
}
