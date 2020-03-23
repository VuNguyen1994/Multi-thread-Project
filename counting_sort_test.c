/* Program to perform counting sort
 * 
 * Author: Naga Kandasamy
 * Date created: February 24, 2020
 *  * 
 *   * Compile as follows: gcc -o counting_sort counting_sort.c -std=c99 -Wall -O3 -lpthread -lm
 *  Team Member: Toan Huynh, Dinh Nguyen
 *    */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <float.h>

/* Do not change the range value. */
#define MIN_VALUE 0 
#define MAX_VALUE 1023

/* Comment out if you don't need debug info */
//#define DEBUG
//#define DEBUG_MORE_VERBOSE

int compute_gold (int *, int *, int, int);
void *compute_silver(void *);
int rand_int (int, int);
void print_array (int *, int);
void print_min_and_max_in_array (int *, int);
void compute_using_pthreads (int *, int *, int **, int, int, int);
int check_if_sorted (int *, int);
int compare_results (int *, int *, int);
void print_histogram (int *, int, int);
void print_histogram_thr (int *, int, int, int);




typedef struct args_for_thread
{
	int	tid;
	int	*input_array;
	int	**local_array;
	int	num_elements;	
	int	start;
	int	range;
	int	num_threads;	

} ARGS_FOR_THREAD;

typedef struct barrier_struct {
	sem_t counter_sem;
	sem_t barrier_sem;
	int counter;
} BARRIER;

BARRIER barrier;
int *global_bin_array;
void barrier_sync (BARRIER *, int, int);



int 
main (int argc, char **argv)
{
    if (argc != 3) {
        printf ("Usage: %s num-elements num-threads\n", argv[0]);
        exit (EXIT_FAILURE);
    }

    int num_elements = atoi (argv[1]);
    int num_threads = atoi (argv[2]);

    int range = MAX_VALUE - MIN_VALUE;
    int *input_array, *sorted_array_reference, *sorted_array_g, **local_array;
    int i;

    barrier.counter = 0;
    sem_init (&barrier.counter_sem, 0, 1); /* Initialize the semaphore protecting the counter to 1 */
    sem_init (&barrier.barrier_sem, 0, 0); /* Initialize the semaphore protecting the barrier to 0 */
	
    /* Populate the input array with random integers between [0, RANGE]. */
    printf ("Generating input array with %d elements in the range 0 to %d\n", num_elements, range);
    input_array = (int *) malloc (num_elements * sizeof (int));
    if (input_array == NULL) {
        printf ("Cannot malloc memory for the input array. \n");
        exit (EXIT_FAILURE);
    }
    srand (time (NULL));
    for ( int i = 0; i < num_elements; i++)
        input_array[i] = rand_int (MIN_VALUE, MAX_VALUE);

#ifdef DEBUG
    print_array (input_array, num_elements);
    print_min_and_max_in_array (input_array, num_elements);
#endif

    /* Sort the elements in the input array using the reference implementation. 
 *      * The result is placed in sorted_array_reference. */
    printf ("\nSorting array using serial version\n");
    int status;
    sorted_array_reference = (int *) malloc (num_elements * sizeof (int));
    if (sorted_array_reference == NULL) {
        perror ("Malloc"); 
        exit (EXIT_FAILURE);
    }
    memset (sorted_array_reference, 0, num_elements);
    struct timeval start, stop;
    gettimeofday(&start, NULL);
    status = compute_gold (input_array, sorted_array_reference, num_elements, range);
    gettimeofday(&stop, NULL);
    if (status == 0) {
        exit (EXIT_FAILURE);
    }

    status = check_if_sorted (sorted_array_reference, num_elements);
    if (status == 0) {
        printf ("Error sorting the input array using the reference code\n");
        exit (EXIT_FAILURE);
    }

    printf ("Counting sort was successful using reference version\n\n");
    printf ("Execution time of serial version : %fs\n", (float)(stop.tv_sec - start.tv_sec + (stop.tv_usec - start.tv_usec)/(float)1000000));

#ifdef DEBUG
    print_array (sorted_array_reference, num_elements);
#endif

    /* FIXME: Write function to sort the elements in the array in parallel fashion. 
 *      * The result should be placed in sorted_array_mt. */

    printf ("\nSorting in pthreads\n");
    sorted_array_g = (int *) malloc (num_elements * sizeof (int));
    if (sorted_array_g == NULL) {
        perror ("Malloc");
        exit (EXIT_FAILURE);
    }
    memset (sorted_array_g, 0, num_elements);
    local_array = (int **) malloc (num_threads * sizeof (int *));
    if (local_array == NULL) {
        perror ("Malloc");
        exit (EXIT_FAILURE);
    }
    for(i = 0; i < num_threads; i++){
	local_array[i] = (int *) malloc ((range + 1) * sizeof (int));
    	if (local_array[i] == NULL) {
       		perror ("Malloc");
        	exit (EXIT_FAILURE);
    	}
    	memset (local_array[i], 0, (range + 1));
    }
    global_bin_array = (int *) malloc ((range + 1) * sizeof (int));
    gettimeofday(&start, NULL);
    compute_using_pthreads (input_array, sorted_array_g, local_array, num_elements, range, num_threads);
    gettimeofday(&stop, NULL);
	
    /* Check the two results for correctness. */
    printf ("\nComparing reference vs sorting results\n");
    status = compare_results (sorted_array_reference, sorted_array_g, num_elements);
    if (status == 1)
        printf ("PASS!!!!!!!!\n");
    else
        printf ("FAIL!\n");
    printf ("Execution time of the comparation : %fs\n", (float)(stop.tv_sec - start.tv_sec + (stop.tv_usec - start.tv_usec)/(float)1000000));

    exit (EXIT_SUCCESS);
}

/* Reference implementation of counting sort. */
int 
compute_gold (int *input_array, int *sorted_array, int num_elements, int range)
{
    /* Compute histogram. Generate bin for each element within 
 *      * the range. 
 *           * */
    int i, j;
    int num_bins = range + 1;
    int *bin = (int *) malloc (num_bins * sizeof (int));    
    if (bin == NULL) {
        perror ("Malloc");
        return 0;
    }

    memset(bin, 0, num_bins); /* Initialize histogram bins to zero */ 
    for (i = 0; i < num_elements; i++)
        bin[input_array[i]]++;

#ifdef DEBUG_MORE_VERBOSE
    print_histogram (bin, num_bins, num_elements);
#endif

    /* Generate the sorted array. */
    int idx = 0;
    for (i = 0; i < num_bins; i++) {
        for (j = 0; j < bin[i]; j++) {
            sorted_array[idx++] = i;
        }
    }

    return 1;
}
/* FIXME */
/* p_threads implementation of counting sort. */
/* compute_silver (int *input_array, int *sorted_array, int num_elements, int start, int range, int num_threads) */
void * 
compute_silver (void *args)
{
    /* Compute histogram. Generate bin for each element within 
     * the range. */
	ARGS_FOR_THREAD *args_for_me = (ARGS_FOR_THREAD *) args; /* Typecast argument passed to function to appropriate type */
	printf("Thread %d reporting for duty\n", args_for_me->tid);
    	int i, j;
	int num_bins = args_for_me->range + 1;
#ifdef DEBUG_MORE_VERBOSE
	printf("waiting for all threads to spawn\n");
	barrier_sync(&barrier, args_for_me->tid, args_for_me->num_threads);
	printf("Thread %d is starting to stride\n", args_for_me->tid);
#endif

    	for (i = args_for_me->start; i < args_for_me->num_elements; i = i + args_for_me->num_threads)
    		args_for_me->local_array[args_for_me->tid][args_for_me->input_array[i]]++;

#ifdef DEBUG_MORE_VERBOSE
    	print_histogram_thr (args_for_me->local_array[args_for_me->tid], num_bins, args_for_me->num_elements, args_for_me->tid);
#endif

	printf("Thread %d is at the barrier. \n", args_for_me->tid);
	barrier_sync(&barrier, args_for_me->tid, args_for_me->num_threads);
	printf("The flood gates hath opened for thread %d. \n", args_for_me->tid);

	
    	/* Stride through local histograms to generate global histogram */
    	for (i = args_for_me->start; i < num_bins; i=i+args_for_me->num_threads) {
    	    for (j = 0; j < args_for_me->num_threads; j++) {
    	        global_bin_array[i] += args_for_me->local_array[j][i];
    	    }
    	}
    	pthread_exit(NULL);
}

/* Fix Me: Write multi-threaded implementation of counting sort. */
void 
compute_using_pthreads (int *input_array, int *sorted_array_g, int **local_array, int num_elements, int range, int num_threads)
{
	pthread_t	*thread_id;
	pthread_attr_t	attributes;
	pthread_attr_init(&attributes);
	thread_id = (pthread_t *) malloc (sizeof(pthread_t) * num_threads);
	ARGS_FOR_THREAD *args_for_thread = (ARGS_FOR_THREAD *) malloc (sizeof (ARGS_FOR_THREAD) * num_threads);
	int i, j;
	int num_bins = range + 1;
	//printf("Spawning Threads to perform counting sum\n");
	for(i = 0; i < num_threads; i++){
		args_for_thread[i].tid = i;
		args_for_thread[i].input_array = input_array;
		args_for_thread[i].local_array = local_array;
		args_for_thread[i].num_elements = num_elements;
		args_for_thread[i].start = i;
		args_for_thread[i].range = range;
		args_for_thread[i].num_threads = num_threads;
		pthread_create(&thread_id[i], &attributes, compute_silver, (void *) &args_for_thread[i]);
	
	}

	for(i = 0; i < num_threads; i++){
		pthread_join(thread_id[i], NULL);
	}

#ifdef DEBUG_MORE_VERBOSE
	print_histogram (global_bin_array, num_bins, num_elements);
#endif

	/* Generate the sorted array. */
    	int idx = 0;
    	for (i = 0; i < num_bins; i++) {
        	for (j = 0; j < global_bin_array[i]; j++) {
            		sorted_array_g[idx++] = i;
        	}
    	}

	return;
}

/* Check if the array is sorted. */
int
check_if_sorted (int *array, int num_elements)
{
    int status = 1;
    for (int i = 1; i < num_elements; i++) {
        if (array[i - 1] > array[i]) {
            status = 0;
            break;
        }
    }

    return status;
}

/* Check if the arrays elements are identical. */ 
int 
compare_results (int *array_1, int *array_2, int num_elements)
{
    int status = 1;
    for (int i = 0; i < num_elements; i++) {
        if (array_1[i] != array_2[i]) {
            status = 0;
            break;
        }
    }

    return status;
}


/* Returns a random integer between [min, max]. */ 
int
rand_int (int min, int max)
{
    float r = rand ()/(float) RAND_MAX;
    return (int) floorf (min + (max - min) * r);
}

/* Helper function to print the given array. */
void
print_array (int *this_array, int num_elements)
{
    printf ("Array: ");
    for (int i = 0; i < num_elements; i++)
        printf ("%d ", this_array[i]);
    printf ("\n");
    return;
}

/* Helper function to return the min and max values in the given array. */
void 
print_min_and_max_in_array (int *this_array, int num_elements)
{
    int i;

    int current_min = INT_MAX;
    for (i = 0; i < num_elements; i++)
        if (this_array[i] < current_min)
            current_min = this_array[i];

    int current_max = INT_MIN;
    for (i = 0; i < num_elements; i++)
        if (this_array[i] > current_max)
            current_max = this_array[i];

    printf ("Minimum value in the array = %d\n", current_min);
    printf ("Maximum value in the array = %d\n", current_max);
    return;
}

/* Helper function to print the contents of the histogram. */
void 
print_histogram (int *bin, int num_bins, int num_elements)
{
    int num_histogram_entries = 0;
    int i;

    for (i = 0; i < num_bins; i++) {
        printf ("Bin %d: %d\n", i, bin[i]);
        num_histogram_entries += bin[i];
    }

    printf ("Number of elements in the input array = %d \n", num_elements);
    printf ("Number of histogram elements = %d \n", num_histogram_entries);

    return;
}

/* Helper function to print the contents of the histogram for pthread. */
void 
print_histogram_thr (int *bin, int num_bins, int num_elements, int tid)
{
    int num_histogram_entries = 0;
    int i;

    for (i = 0; i < num_bins; i++) {
        printf ("Thread %d - Bin %d: %d\n", tid, i, bin[i]);
        num_histogram_entries += bin[i];
    }

    printf ("Thread %d - Number of elements in the input array = %d \n", tid, num_elements);
    printf ("Thread %d - Number of histogram elements = %d \n", tid, num_histogram_entries);

    return;
}


void
barrier_sync (BARRIER *barrier, int thread_number, int num_threads)
{
    sem_wait (&(barrier->counter_sem)); /* Obtain the lock on the counter */

    /* Check if all threads before us, that is NUM_THREADS-1 threads have reached this point */
    if (barrier->counter == (num_threads - 1)) {
        barrier->counter = 0; /* Reset the counter */
					 
        sem_post (&(barrier->counter_sem)); 
					 
        /* Signal the blocked threads that it is now safe to cross the barrier */			 
        printf("Thread number %d is signalling other threads to proceed. \n", thread_number); 			 
        for (int i = 0; i < (num_threads - 1); i++)
            sem_post (&(barrier->barrier_sem));
    } 
    else {
        barrier->counter++; // Increment the counter
        sem_post (&(barrier->counter_sem)); // Release the lock on the counter
        sem_wait (&(barrier->barrier_sem)); // Block on the barrier semaphore and wait for someone to signal us when it is safe to cross
    }
}