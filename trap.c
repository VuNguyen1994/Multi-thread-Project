/*  Purpose: Calculate definite integral using trapezoidal rule.
 *
 * Input:   a, b, n, num_threads
 * Output:  Estimate of integral from a to b of f(x)
 *          using n trapezoids, with num_threads.
 *
 * Compile: gcc -o trap trap.c -O3 -std=c99 -Wall -lpthread -lm
 * Usage:   ./trap
 *
 * Note:    The function f(x) is hardwired.
 *
 * Name: Dinh Nguyen & Toan Huynh
 * ECEC 353 Project 2 Part 1
 */
#define _REENTRANT /* Make sure the library functions are MT (muti-thread) safe */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

double compute_using_pthreads (float, float, int, float, int);
double compute_gold (float, float, int, float);
/* Function prototype for the thread routines */
void *compute_each (void *);

/* Structure used to pass arguments to the worker threads */
typedef struct args_for_thread_t {
    float a_thread;                /* Lower limit of each thread */
    float b_thread;               /* Upper limit of each thread */
    float n_thread;             /* Num of trapezoids for each thread */
    float h_thread;             /* base of each trapezoid */
} ARGS_FOR_THREAD; 

double thread_integral = 0.0; /*shared integral for threads*/

int 
main (int argc, char **argv) 
{
    if (argc < 5) {
        printf ("Usage: %s lower-limit upper-limit num-trapezoids num-threads\n", argv[0]);
        printf ("lower-limit: The lower limit for the integral\n");
        printf ("upper-limit: The upper limit for the integral\n");
        printf ("num-trapezoids: Number of trapeziods used to approximate the area under the curve\n");
        printf ("num-threads: Number of threads to use in the calculation\n");
        exit (EXIT_FAILURE);
    }

    float a = atof (argv[1]); /* Lower limit */
	float b = atof (argv[2]); /* Upper limit */
	float n = atof (argv[3]); /* Number of trapezoids */

	float h = (b - a)/(float) n; /* Base of each trapezoid */  
	printf ("The base of the trapezoid is %f\n", h);

    struct timeval start, stop;
    gettimeofday(&start, NULL);
	double reference = compute_gold (a, b, n, h);
    printf ("Reference solution computed using single-threaded version = %f\n", reference);
   gettimeofday(&stop, NULL);
    printf ("Computing time for single-threaded version: %fs\n", (float)(stop.tv_sec - start.tv_sec + (stop.tv_usec - start.tv_usec)/(float)1000000));

	/* Write this function to complete the trapezoidal rule using pthreads. */
    int num_threads = atoi (argv[4]); /* Number of threads */
    gettimeofday(&start, NULL);
	double pthread_result = compute_using_pthreads (a, b, n, h, num_threads);
	printf ("Solution computed using %d threads = %f\n", num_threads, pthread_result);
    gettimeofday(&stop, NULL);
    printf ("Computing time for multi-threaded version: %fs\n", (float)(stop.tv_sec - start.tv_sec + (stop.tv_usec - start.tv_usec)/(float)1000000));

    exit (EXIT_SUCCESS);
} 

/*------------------------------------------------------------------
 * Function:    f
 * Purpose:     Defines the integrand
 * Input args:  x
 * Output: sqrt((1 + x^2)/(1 + x^4))

 */
float 
f (float x) 
{
    return sqrt ((1 + x*x)/(1 + x*x*x*x));
}

/*------------------------------------------------------------------
 * Function:    compute_gold
 * Purpose:     Estimate integral from a to b of f using trap rule and
 *              n trapezoids using a single-threaded version
 * Input args:  a, b, n, h
 * Return val:  Estimate of the integral 
 */
double 
compute_gold (float a, float b, int n, float h) 
{
   double integral;
   int k;

   integral = (f(a) + f(b))/2.0;

   for (k = 1; k <= n-1; k++) 
     integral += f(a+k*h);
   
   integral = integral*h;

   return integral;
}  

/* FIXME: Complete this function to perform the trapezoidal rule using pthreads. */
double 
compute_using_pthreads (float a, float b, int n, float h, int num_threads)
{
	
    /* Allocate memory to store the IDs of the worker threads */
    pthread_t *worker_thread = (pthread_t *) malloc (num_threads * sizeof (pthread_t));
    ARGS_FOR_THREAD *thread_parameter;
	
    int i;
    //printf ("Main thread is creating %d worker threads \n", num_threads);
	
    /* Create worker threads and ask them to execute my_func that takes a structure as an argument */
    for (int i = 0; i < num_threads; i++) {
        thread_parameter = (ARGS_FOR_THREAD *) malloc (sizeof (ARGS_FOR_THREAD)); /* Memory for structure to pack the arguments */
        thread_parameter->a_thread = a + i * ((b-a)/num_threads); /* Fill the structure with some dummy arguments */
        thread_parameter->b_thread = b - (num_threads-i-1)*((b-a)/num_threads); 
        thread_parameter->n_thread = n/num_threads;
        thread_parameter->h_thread = (thread_parameter->b_thread - thread_parameter->a_thread)/thread_parameter->n_thread; 
		
        if ((pthread_create (&worker_thread[i], NULL, compute_each, (void *)thread_parameter)) != 0) {
            perror ("pthread_create");
            exit (EXIT_FAILURE);
        }
    }
		  
    /* Wait for all the worker threads to finish */
    for (i = 0; i < num_threads; i++)
        pthread_join (worker_thread[i], NULL);
		
    free ((void *) worker_thread);

    return thread_integral;
}

/* Function that will be executed by all the worker threads */
void *
compute_each (void *thread_parameter)
{
    ARGS_FOR_THREAD *parameter = (ARGS_FOR_THREAD *) thread_parameter; /* Typecast argument passed to function to appropriate type */
	double integral;
    int k;

    integral = (f(parameter->a_thread) + f(parameter->b_thread))/2.0;

    for (k = 1; k <= parameter->n_thread - 1; k++) 
        integral += f(parameter->a_thread + k * parameter->h_thread);
   
    integral = integral*parameter->h_thread;
    thread_integral += integral;
    free ((void *) parameter); /* Free up the structure */
    pthread_exit (NULL);
}

