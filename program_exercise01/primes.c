/* primes.c print out the last five primes found. 
  * Name: Dinh Nguyen
    * ECEC 353
	   * Winter 2020
		  * Date created: June 28, 2018
		    * Date updated: January 16, 2020 
			   *
				  * Build your code as follows: gcc -o primes primes.c -std=c99 -Wall
				    *
					   * */
#define _POSIX_C_SOURCE 1

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <signal.h>
#include <setjmp.h>

#define FALSE 0
#define TRUE !FALSE

#define CTRLC  1337

unsigned long int num_found; /* Number of prime numbers found */
long int buf[5]; /*Bounded buffer max 5 long integers */

static void custom_signal_handler (int);

static sigjmp_buf env; /*declare jmp_buf*/

int 
is_prime (unsigned int num)
{
unsigned long int i;
	if (num < 2) {
		return FALSE;
	}
	else if (num == 2) {
		return TRUE;
	}
	else if (num % 2 == 0) {
		return FALSE;
	}
	else {
		for (i = 3; (i*i) <= num; i += 2) {
			if (num % i == 0) {
				return FALSE;
			}
		}
		return TRUE;
	}
}
/* Complete the function to display the last five prime numbers found by the 
 * program, either upon normal program termination or upon being terminated 
  * via a SINGINT or SIGQUIT signal. 
   */

void 
report ()
{
	printf ("The last five primes found are: \n");
	for (int i=0; i<5; i++){
		printf("%ld\t\t", buf[i]);
	}
	printf("\n");
}

int 
main (int argc, char** argv)
{
    /* Sets up our signal handler to catch SIGINT and SIGQUIT */
	signal (SIGINT, custom_signal_handler);
	signal (SIGQUIT, custom_signal_handler);
	
	/*set jmp point here*/
	int ret;
	ret=sigsetjmp (env, TRUE);
	switch (ret){
		case 0:
			break;
		case CTRLC: /*if a CTRLC caught*/
			report(); /*print the last five primes found*/
			exit(EXIT_SUCCESS);
	}

	/*Count the number of primes*/
	unsigned long int num;
   num_found = 0;
	printf ("Beginning search for primes between 1 and %lu. \n", LONG_MAX);
	for (num = 1; num < LONG_MAX; num++) {
		if (is_prime (num)) {
			if (num_found<4){ /*use num_found as buf index, max num_found = 4*/
				num_found++;
			}
			else
			{
				num_found=0;
			}
			buf[num_found]=num; /*load primes to buf*/
			printf ("%lu \n", num);
		}
	}
	report(); /*print the last five primes and terminate normally*/
	exit(EXIT_SUCCESS);
}

/*define signal handler*/
static void 
custom_signal_handler(int signalNumber)
{
	   switch (signalNumber){
			case SIGINT: //catch SIGINT signal
				siglongjmp (env, CTRLC);
			case SIGQUIT: //catch SIGQUIT signal
				siglongjmp (env, CTRLC);
		}
}

