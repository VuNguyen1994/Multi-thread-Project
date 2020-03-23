/* This code illustrates the use of simple signal handlers with the intention of 
 * catching some selected signals. 
  *
   * Signals are one of the mechanisms used by the kernel to communicate with the processes in user space. 
	 * Signals can also be used as a rudimentary form of inter-process communication.
	  *
	   * Author: Naga Kandasamy
		 * Date created: December 22, 2008
		  * Date modified: January 14, 2020
		   *
			 * */
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

static void custom_signal_handler (int);

int 
main (int argc, char **argv)
{
	signal (SIGFPE, custom_signal_handler);
	signal (SIGINT, custom_signal_handler);
	signal (SIGQUIT, custom_signal_handler);
	signal (SIGUSR1, custom_signal_handler); 
	    signal (SIGUSR2, custom_signal_handler);
	for (;;)
		        pause ();
}

static void 
custom_signal_handler (int signalNumber)
{
	switch (signalNumber){
		case SIGINT:
			signal (SIGINT, custom_signal_handler);
			printf ("Ouch...Caught the Control+C signal. \n");
			             break;
		case SIGQUIT:
			printf ("Caught the Quit signal. \n");
			exit (EXIT_SUCCESS);
		default:
			break;
	}
}

