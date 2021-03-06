/* Code for the Jacobi equation solver. 
 * Author: Naga Kandasamy
 * Date created: April 19, 2019
 * Date modified: February 21, 2020
 *
 * Compile as follows:
 * gcc -o solver solver.c solver_gold.c -O3 -Wall -std=c99 -lm -lpthread
 *
 * If you wish to see debug info, add the -D DEBUG option when compiling the code.
 */

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <math.h>
#include "grid.h" 

typedef struct barrier_struct {
    sem_t counter_sem; /* Protects access to the counter */
    sem_t barrier_sem; /*Signals that barrier is safe to cross */
    int counter; /* The counter value */
} BARRIER;

/* Structure used to pass arguments to the worker threads */
typedef struct args_for_thread_t {
    int thread_idx;
} ARGS_FOR_THREAD; 

/* Create the barrier data structure */
BARRIER barrier; 

/* Function prototypes */
void *my_thread (void *);
void barrier_sync (BARRIER *);

extern int compute_gold (grid_t *);
int compute_using_pthreads_jacobi (grid_t *, int);
void compute_grid_differences(grid_t *, grid_t *);
grid_t *create_grid (int, float, float);
grid_t *copy_grid (grid_t *);
void print_grid (grid_t *);
void print_stats (grid_t *);
double grid_mse (grid_t *, grid_t *);

/*Shared variables*/
int num_threads;
int total_iter = 0;
float eps = 1e-2; /* Convergence criteria. */
int num_elements = 0; 
int done = 0;
double diff = 0.0;
grid_t *grid_2;
grid_t *grid_temp;

int 
main (int argc, char **argv)
{	
	if (argc < 5) {
        printf ("Usage: %s grid-dimension num-threads min-temp max-temp\n", argv[0]);
        printf ("grid-dimension: The dimension of the grid\n");
        printf ("num-threads: Number of threads\n"); 
        printf ("min-temp, max-temp: Heat applied to the north side of the plate is uniformly distributed between min-temp and max-temp\n");
        exit (EXIT_FAILURE);
    }
    
    /* Parse command-line arguments. */
    int dim = atoi (argv[1]);
    num_threads = atoi (argv[2]);
    float min_temp = atof (argv[3]);
    float max_temp = atof (argv[4]);
    
    /* Generate the grids and populate them with initial conditions. */
 	grid_t *grid_1 = create_grid (dim, min_temp, max_temp);
    /* Grid 2 should have the same initial conditions as Grid 1. */
    grid_2 = copy_grid (grid_1); 
    grid_temp = copy_grid (grid_2);

	/* Compute the reference solution using the single-threaded version. */
	printf ("\nUsing the single threaded version to solve the grid\n");
	int num_iter = compute_gold (grid_1);
	printf ("Convergence achieved after %d iterations\n", num_iter);
    /* Print key statistics for the converged values. */
	printf ("Printing statistics for the interior grid points\n");
    print_stats (grid_1);
#ifdef DEBUG
    print_grid (grid_1);
#endif
	
	/* Use pthreads to solve the equation using the jacobi method. */
	printf ("\nUsing pthreads to solve the grid using the jacobi method\n");
	num_iter = compute_using_pthreads_jacobi (grid_2, num_threads);
	printf ("Convergence achieved after %d iterations\n", num_iter);			
    printf ("Printing statistics for the interior grid points\n");
	print_stats (grid_2);
#ifdef DEBUG
    print_grid (grid_2);
#endif
    
    /* Compute grid differences. */
    double mse = grid_mse (grid_1, grid_2);
    printf ("MSE between the two grids: %f\n", mse);

	/* Free up the grid data structures. */
	free ((void *) grid_1->element);	
	free ((void *) grid_1); 
	free ((void *) grid_2->element);	
	free ((void *) grid_2);

	exit (EXIT_SUCCESS);
}

/* FIXME: Edit this function to use the jacobi method of solving the equation. The final result should be placed in the grid data structure. */
int 
compute_using_pthreads_jacobi (grid_t *grid, int num_threads)
{		
    pthread_t *thread_id = (pthread_t *) malloc (sizeof (pthread_t) * num_threads);
    ARGS_FOR_THREAD *thread_parameter;

    int i;
    /* Initialize the barrier data structure */
    barrier.counter = 0;
    sem_init (&barrier.counter_sem, 0, 1); /* Initialize the semaphore protecting the counter to 1 */
    sem_init (&barrier.barrier_sem, 0, 0); /* Initialize the semaphore protecting the barrier to 0 */
    
     
    /* Create the threads */
    for (i = 0; i < num_threads; i++){
        thread_parameter = (ARGS_FOR_THREAD *) malloc (sizeof (ARGS_FOR_THREAD)); /* Memory for structure to pack the arguments */
        thread_parameter->thread_idx = i+1;
        if ((pthread_create (&thread_id[i], NULL, my_thread, (void *)thread_parameter)) != 0) {
            perror ("pthread_create");
            exit (EXIT_FAILURE);
        }
    }

    /* Wait to reap the threads that we have created */
    for (i = 0; i < num_threads; i++){
        pthread_join (thread_id[i], NULL);
    }

    free ((void *) thread_id);
    //pthread_exit (NULL);
    return total_iter;
}

/* The function executed by the threads. */
void *
my_thread (void *thread_parameter)
{
    ARGS_FOR_THREAD *parameter = (ARGS_FOR_THREAD *) thread_parameter; /* Typecast argument passed to function to appropriate type */
	float old, new; 
    
    while (!done){
        int element_count = 0;
        double diff_temp = 0.0;
        int idx = parameter->thread_idx + grid_2->dim; //Start at row 1, column 1
        while (idx < (grid_2->dim) * (grid_2->dim-1))
        {
            //rear of matrix - last row, first column, last column
            if ( (((idx - (idx % grid_2->dim)) / grid_2->dim)  == (grid_2->dim -1)) ){
                grid_temp->element[idx] = 0.1;
                idx = idx + num_threads;
            }
            if ((idx % grid_2->dim) == 0){
                //grid_temp->element[idx] = 0.0;
                idx = idx + 2;
            }
            else if ((idx % grid_2->dim) == (grid_2->dim -1)){
                //grid_temp->element[idx] = 0.0;
                idx = idx + 2;
            }
            else //not rear case
            {
                old = grid_2->element[idx];
                //Update temporary matrix based on previous iteration from grid_2
                new = 0.25 * (grid_2->element[idx-1] + grid_2->element[idx+1] + grid_2->element[idx + grid_2->dim] + grid_2->element[idx - grid_2->dim]);
                grid_temp->element[idx] = new;

                diff_temp = diff_temp + fabs(new - old); /* Calculate the difference in values. */
                element_count++;

                if ((idx % grid_2->dim) + num_threads < (grid_2->dim -1)){ //idx stay within the row for the next increment
                   idx = idx + num_threads;
                }
                else{ 
                    idx = idx + num_threads + 2;
                }
            }
        }
        printf("Thread %d max_idx: %d element_count %d reached barrier!\n", parameter->thread_idx, idx, element_count);
        num_elements+=element_count;
        diff += diff_temp;
        barrier_sync (&barrier); /* Wait here for all threads to catch up before starting the next iteration */
    }
    pthread_exit (NULL);
}

/* The function that implements the barrier synchronization. */
void 
barrier_sync (BARRIER *barrier)
{
    sem_wait (&(barrier->counter_sem)); /* Obtain the lock on the counter */

    /* Check if all threads before us, that is NUM_THREADS-1 threads have reached this point */
    if (barrier->counter == (num_threads - 1)) {
        /* Check for convergence after each iteration */
        diff = diff/num_elements;
        printf ("Iteration %d. DIFF: %f. Num_Element: %d\n", total_iter, diff, num_elements);
        total_iter++;
        if (diff < eps){
            done = 1;
        }
        /*Reset diff each iteration*/
        diff = 0.0;
        num_elements = 0;
        /*Parse temporary grid to original grid_2 to start new iteration*/
        grid_2 = copy_grid(grid_temp);
        barrier->counter = 0; /* Reset the counter */    
        sem_post (&(barrier->counter_sem)); /* Signal the blocked threads that it is now safe to cross the barrier */			 
        
        for (int i = 0; i < (num_threads - 1); i++)
            sem_post (&(barrier->barrier_sem));
    } 
    else {
        barrier->counter++; // Increment the counter
        sem_post (&(barrier->counter_sem)); // Release the lock on the counter
        sem_wait (&(barrier->barrier_sem)); // Block on the barrier semaphore and wait for someone to signal us when it is safe to cross
    }
}


/* Create a grid with the specified initial conditions. */
grid_t * 
create_grid (int dim, float min, float max)
{
    grid_t *grid = (grid_t *) malloc (sizeof (grid_t));
    if (grid == NULL)
        return NULL;

    grid->dim = dim;
	printf("Creating a grid of dimension %d x %d\n", grid->dim, grid->dim);
	grid->element = (float *) malloc (sizeof (float) * grid->dim * grid->dim);
    if (grid->element == NULL)
        return NULL;

    int i, j;
	for (i = 0; i < grid->dim; i++) {
		for (j = 0; j < grid->dim; j++) {
            grid->element[i * grid->dim + j] = 0.0; 			
		}
    }

    /* Initialize the north side, that is row 0, with temperature values. */ 
    srand ((unsigned) time (NULL));
	float val;		
    for (j = 1; j < (grid->dim - 1); j++) {
        val =  min + (max - min) * rand ()/(float)RAND_MAX;
        grid->element[j] = val; 	
    }

    return grid;
}

/* Creates a new grid and copies over the contents of an existing grid into it. */
grid_t *
copy_grid (grid_t *grid) 
{
    grid_t *new_grid = (grid_t *) malloc (sizeof (grid_t));
    if (new_grid == NULL)
        return NULL;

    new_grid->dim = grid->dim;
	new_grid->element = (float *) malloc (sizeof (float) * new_grid->dim * new_grid->dim);
    if (new_grid->element == NULL)
        return NULL;

    int i, j;
	for (i = 0; i < new_grid->dim; i++) {
		for (j = 0; j < new_grid->dim; j++) {
            new_grid->element[i * new_grid->dim + j] = grid->element[i * new_grid->dim + j] ; 			
		}
    }

    return new_grid;
}

/* This function prints the grid on the screen. */
void 
print_grid (grid_t *grid)
{
    int i, j;
    for (i = 0; i < grid->dim; i++) {
        for (j = 0; j < grid->dim; j++) {
            printf ("%f\t", grid->element[i * grid->dim + j]);
        }
        printf ("\n");
    }
    printf ("\n");
}


/* Print out statistics for the converged values of the interior grid points, including min, max, and average. */
void 
print_stats (grid_t *grid)
{
    float min = INFINITY;
    float max = 0.0;
    double sum = 0.0;
    int num_elem = 0;
    int i, j;

    for (i = 1; i < (grid->dim - 1); i++) {
        for (j = 1; j < (grid->dim - 1); j++) {
            sum += grid->element[i * grid->dim + j];

            if (grid->element[i * grid->dim + j] > max) 
                max = grid->element[i * grid->dim + j];

             if(grid->element[i * grid->dim + j] < min) 
                min = grid->element[i * grid->dim + j];
             
             num_elem++;
        }
    }
                    
    printf("AVG: %f\n", sum/num_elem);
	printf("MIN: %f\n", min);
	printf("MAX: %f\n", max);
	printf("\n");
}

/* Calculate the mean squared error between elements of two grids. */
double
grid_mse (grid_t *grid_1, grid_t *grid_2)
{
    double mse = 0.0;
    int num_elem = grid_1->dim * grid_1->dim;
    int i;

    for (i = 0; i < num_elem; i++) 
        mse += (grid_1->element[i] - grid_2->element[i]) * (grid_1->element[i] - grid_2->element[i]);
                   
    return mse/num_elem; 
}



		

