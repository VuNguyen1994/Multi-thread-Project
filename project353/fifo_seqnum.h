/* Header file for fifo_seqnum_server.c and fifo_seqnum_client.c 
 *
 * Source: M. Kerrisk, Linux Programming Interface
 *
 * Author: Naga Kandasamy
 * Date created: July 10, 2018
 *
 */
#ifndef _FIFO_SEQNUM_H_
#define _FIFO_SEQNUM_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/* Well-known name for server FIFO */
#define SERVER_FIFO "/tmp/seqnum_sv"

/* Template for building the client FIFO */
#define CLIENT_FIFO_TEMPLATE "/tmp/seqnum_client.%ld"
/* Space required to hold the client FIFO pathname; 20 bytes to hold the client pid */
#define CLIENT_FIFO_NAME_LEN (sizeof (CLIENT_FIFO_TEMPLATE) + 20)

/* Recall that the data in pipes and FIFOs is a byte stream: boundaries between multiple 
 * messages are not preserved. So, when multiple messages are delivered to the server by the clients, 
 * the reader (server) and the writer (client) must agree on some convention for separating the 
 * messages. We use fixed-length messages and have the server always read messages of this fixed length to solve the 
 * above problem. Though simple to program, the solution is brittle in the sense that if one of the clients 
 * accidentally or deliberately sends a message that is not the right length, all subsequent messages will be out of
 * step. The server cannot recover from this situation easily. 
 *
 * Format for the request message from client ----> server: 
 * */
struct request {
    pid_t pid;          /* PID of the client */
    int seq_len;        /* Length of requested sequence */
    char msg[256];
};

/* Format for the response message from server ----> client */
struct response {
    int seq_num;
    char msg[256];
};

#endif  /* _FIFO_SEQNUM_H_ */