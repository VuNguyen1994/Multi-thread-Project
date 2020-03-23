/* This code illustrates the correct way to implement a simple signal handler that catches the Control+C signal.
 * The signal handler uses no non-reentrant functions. Rather, it justs sets a global flag that is read by the 
  * main function.
   *
	 * Author: Naga Kandasamy
	  * Date created: 6/26/2018
	   *
		 * */

#define _POSIX_C_SOURCE 1

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>

#define FALSE 0
#define TRUE !FALSE

#define CTRLC  1337
static void custom_signal_handler (int); 

static sigjmp_buf env;

int 
main (int argc, char **argv)
{
	/* Sets up our signal handler to catch SIGINT */
	signal (SIGINT, custom_signal_handler);

	int ret;
	ret=sigsetjmp(env, TRUE);
	switch(ret) {
		case 0:
		/* Return from explicit sigsetjmp call. */
			break;
		case CTRLC:
			printf ("\nReturned from longjmp!\n");
			printf ( "Cleaning up and exiting safely...\n");
			exit (EXIT_SUCCESS);
	}
	/* Main processing loop */
	for (int i = 0; i < 10; i++) {
		printf ("Working. \n");
		sleep (2); /* Simulate some processing in the loop */
	}
	exit (EXIT_SUCCESS);
}
/* The user-defined signal handler */
static void 
custom_signal_handler (int signalNumber)
{
	    siglongjmp (env, CTRLC);

}
