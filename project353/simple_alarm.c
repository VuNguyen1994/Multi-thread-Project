/* This code illustrates a simple signal handler that catches the alarm signal on a periodic basis 
 *
 * Author: Naga Kandasamy
 * Date created: 6/27/2018
 *
 * */

#define _POSIX_C_SOURCE 1

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <setjmp.h>
#include <stdlib.h>

#define TRUE 1
#define FALSE 0
#define CTRLC 2020

static void alarm_handler (int); /* Signal handler to catch the ALRM signal */
static void ctrl_c_handler (int); /* Signal handler to catch CTRL+C signal */

static sigjmp_buf env;

int 
main (int argc, char **argv)
{
    int alarm_interval = 2; /* Set our alarm to ring every two seconds */
  
    /* Set up signal handlers to catch SIGALRM and SIGINT */
    signal (SIGALRM, alarm_handler);
    signal (SIGINT, ctrl_c_handler); 

    int ret;
    ret = sigsetjmp (env, TRUE);
    switch (ret) {
        case 0:
            /* Returned from explicit sigsetjmp call */
            break;

        case CTRLC:
            printf ("\nSIGINT received\n");
            printf ( "Cleaning up and exiting safely...\n");
            exit (EXIT_SUCCESS);
    }

    while (TRUE) {
        alarm (alarm_interval); /* Set our alarm to ring at the specified interval */ 
        pause (); /* Suspend our process here until a signal is caught */
    }
}

/* The user-defined signal handler for SIGALRM */
static void 
alarm_handler (int sig)
{
    printf ("Alarm caught. \n");
    signal (SIGALRM, alarm_handler); /* Restablish handler for next occurrence */
    return;
}

/* The user-defined signal handler for SIGINT */
static void 
ctrl_c_handler (int sig)
{
    siglongjmp (env, CTRLC);
}
