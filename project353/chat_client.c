/* Skeleton code for the client side code. 
 *
 * Compile as follows: gcc -o chat_client chat_client.c -std=c99 -Wall -lrt
 *
 * Author: Naga Kandsamy
 * Date created: January 28, 2020
 * Date modified:
 *
 * Student/team name: FIXME
 * Date created: FIXME 
 *
*/

#define _POSIX_C_SOURCE 2 // For getopt()

#include <mqueue.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "msg_structure.h"

void 
print_main_menu (void)
{
    printf ("\n'B'roadcast message\n");
    printf ("'P'rivate message\n");
    printf ("'E'xit\n");
    return;
}

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
    char user_name[USER_NAME_LEN];

    if (argc != 2) {
        printf ("Usage: %s user-name\n", argv[0]);
        exit (EXIT_FAILURE);
    }

    strcpy (user_name, argv[1]); /* Get the client user name */

    /* FIXME: Connect to server */
    /* Create a FIFO using the template to be used for receiving 
     * the response from the server. This is done before sending the 
     * request to the server to make sure that the FIFO exists by the 
     * time the server attempts to open it and send the response message. 
     */
    umask (0);
    snprintf (client_fifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, user_name/*(long) getpid ()*/);
    printf ("CLIENT: creating FIFO %s \n", client_fifo);
    if (mkfifo (client_fifo, S_IRUSR | S_IWUSR | S_IWGRP) == -1 && errno != EEXIST) {
        perror ("mkfifo");
        exit (EXIT_FAILURE);
    }

    /*Init msg queuse with user_name and pid*/
    int server_fd, client_fd;
    struct client_msg req; //request
    struct server_msg resp; //response
    req.client_pid = getpid();
    strcpy (req.user_name, user_name);
    
    // control flag here
    server_fd = open (SERVER_FIFO, O_WRONLY);
    if (server_fd == -1) {
        printf ("Cannot open server FIFO %s\n", SERVER_FIFO);
        req.control = 0;
        exit (EXIT_FAILURE);
    }
    else
    {
        req.control = 1;
    }
    

    /* Operational menu for client */
    char option, dummy;
    while (1) {
        print_main_menu ();
        option = getchar ();

        switch (option) {
            case 'B':
               /* FIXME: Send message to server to be broadcast */ 
                req.broadcast = 1;
                /*Get input msg*/
                dummy = getchar(); //discard the last /n from buffer
                printf("CLIENT %s: " , req.user_name);
                int m =0;
                while (req.msg[m] = getchar(), req.msg[m]!='\n'){
                    m++;
                }
                if (write (server_fd, &req, sizeof (struct client_msg)) != sizeof (struct client_msg)) {
                    printf ("Cannot write to server");
                    exit (EXIT_FAILURE);
                }
                /* Open our FIFO and read the response from the server. */
                client_fd = open (client_fifo, O_RDONLY);
                if (client_fd == -1) {
                    printf ("Cannot open FIFO %s for reading \n", client_fifo);
                    exit (EXIT_FAILURE);
                }
    
                if (read (client_fd, &resp, sizeof (struct server_msg)) != sizeof (struct server_msg)) {
                    printf ("Cannot read response from server \n");
                    exit (EXIT_FAILURE);
                }
					 
					 printf ("Number of clients connected to server: %d \n", resp.number_client);    
                printf ("Response received from server echo to sender %s : %s \n", resp.sender_name, resp.msg);    
                exit (EXIT_SUCCESS);
                break;

            case 'P':
                /* FIXME: Get name of private user and send the private 
                 * message to server to be sent to private user */
					 req.broadcast = 0;
                dummy = getchar(); //discard the last /n from buffer
                printf("Enter private username: ");
                int a =0;
                while (req.priv_user_name[a] = getchar(), req.priv_user_name[a]!='\n'){
                    a++;
                }

                printf("Enter msg: \t");
                int n =0;
                while (req.msg[n] = getchar(), req.msg[n]!='\n'){
                    n++;
                }

                if (write (server_fd, &req, sizeof (struct client_msg)) != sizeof (struct client_msg)) {
                    printf ("Cannot write to server");
                    exit (EXIT_FAILURE);
                }

                /* Open our FIFO and read the response from the server.*/
                client_fd = open (client_fifo, O_RDONLY);
                if (client_fd == -1) {
                    printf ("Cannot open FIFO %s for reading \n", client_fifo);
                    exit (EXIT_FAILURE);
                }
    
                if (read (client_fd, &resp, sizeof (struct server_msg)) != sizeof (struct server_msg)) {
                    printf ("Cannot read response from server \n");
                    exit (EXIT_FAILURE);
                }

					 printf ("Number of clients connected to server: %d \n", resp.number_client);    
                printf ("Response received from server echo to sender %s : %s \n", resp.sender_name, resp.msg);
                exit (EXIT_SUCCESS);
                break;

            case 'E':
                printf ("Chat client exiting\n");
                /* FIXME: Send message to server that we are exiting */
                /* Establish an exit handler so that when the client program exits, the FIFO is removed 
                * from the file system. 
                */
                if (atexit (remove_fifo) != 0) {
                    perror ("atexit");
                    exit (EXIT_FAILURE);
                }
                exit (EXIT_SUCCESS);

            default:
                printf ("Unknown option\n");
                break;
                
        }
        /* Read dummy character to consume the \n left behind in STDIN */
        dummy = getchar ();
    }
    
}
