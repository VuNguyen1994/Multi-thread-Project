/* Code for the Jacobi equation solver. 
 * Name: Dinh Nguyen & Toan Huynh
 * ECEC 353 - Project 2 Part 2
 * 
 * Compile as follows:
 * gcc -o solver solver.c solver_gold.c -O3 -Wall -std=c99 -lm -lpthread
 *
 * If you wish to see debug info, add the -D DEBUG option when compiling the code.
 */

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <sys/mman.h>
#include <time.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <math.h>
#include "grid.h" 

typedef struct barrier_struct {
    sem_t barrier_sem_dln45; /*Signals that barrier is safe to cross */
    sem_t counter_sem_dln45;
    int counter; /* The counter of threads */
} BARRIER;
typedef struct barrier2_struct {
    sem_t update_sem_dln45; /*Signals the barrier is safe to start update grid */
    sem_t update_counter_sem_dln45;
    int update_counter; 
} BARRIER2;

/* Create the barrier data structure */
BARRIER barrier; //barrier at the end of each iteration
BARRIER2 barrier2; //barrier at the end of calculation section of each iteration. Begin updating buffer grid

/* Function prototypes */
void *my_thread (void*);
void barrier_sync (BARRIER *);
void update_sync (BARRIER2 *);

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
float eps = 1e-6; /* Convergence criteria. */
int num_elements2 = 0; 
int done2 = 0;
double diff2 = 0.0;

grid_t *grid_2;
grid_t *grid_temp; //grid buffer

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

    time_t start, end;
    double time_used1, time_used2;
    
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
//    start = clock();
	time(&start);
	int num_iter = compute_gold (grid_1);
  //  end = clock();
  time(&end);
   // time_used1 = ((double) (end - start)) / CLOCKS_PER_SEC;
	time_used1 = (double)(end -start);
    printf("Begin Jacobi iteration ******************\n");
//    start = clock();
	time(&start);
	int num_iter2 = compute_using_pthreads_jacobi (grid_2, num_threads);
	time(&end);
  //  end = clock();
   // time_used2 = ((double) (end - start)) / CLOCKS_PER_SEC;
	time_used2 = (double)(end - start);

#ifdef DEBUG
    print_grid (grid_1);
#endif
    printf ("\nUsing the single threaded version to solve the grid\n");
	printf ("Convergence achieved after %d iterations\n", num_iter);
    /* Print key statistics for the converged values. */
	printf ("Printing statistics for the interior grid points\n");
    print_stats (grid_1);
    
	/* Use pthreads to solve the equation using the jacobi method. */
	printf ("\nUsing %d pthreads to solve the grid %d x %d using the jacobi method\n", num_threads, dim, dim);
	printf ("Convergence achieved after %d iterations\n", num_iter2);			
    printf ("Printing statistics for the interior grid points\n");
	print_stats (grid_2);
#ifdef DEBUG
    print_grid (grid_2);
#endif
    
    /* Compute grid differences. */
    double mse = grid_mse (grid_1, grid_2);
    printf ("MSE between the two grids: %f\n", mse);
    printf ("Time used for single thread: %f seconds.\n",time_used1);
    printf ("Time used for multi thread: %f seconds.\n",time_used2);

	/* Free up the grid data structures. */
	free ((void *) grid_1->element);	
	free ((void *) grid_1); 
	free ((void *) grid_2->element);	
	free ((void *) grid_2);
    free ((void *) grid_temp->element);	
	free ((void *) grid_temp);

	exit (EXIT_SUCCESS);
}

/* FIXME: Edit this function to use the jacobi method of solving the equation. The final result should be placed in the grid data structure. */
int 
compute_using_pthreads_jacobi (grid_t *grid, int num_threads)
{		
    pthread_t *thread_id = (pthread_t *) malloc (sizeof (pthread_t) * num_threads);
    int i;
    /* Initialize the barrier data structure */
    barrier.counter = 0;
    barrier2.update_counter = 0;
    sem_init (&barrier.barrier_sem_dln45, 0, 0); /* Initialize the semaphore protecting the barrier to 0 */
    sem_init (&barrier.counter_sem_dln45, 0, 1);
    sem_init (&barrier2.update_sem_dln45, 0, 0);
    sem_init (&barrier2.update_counter_sem_dln45, 0, 1);
    /* Create the threads */
    for (i = 0; i < num_threads; i++){
        if (pthread_create (&thread_id[i], NULL, my_thread, (void*) (long) i) != 0) {
            perror ("pthread_create");
            exit (EXIT_FAILURE);
        }
    }

    /* Wait to reap the threads that we have created */
    for (i = 0; i < num_threads; i++){
        pthread_join (thread_id[i], NULL);
    }
    /* Clean up semaphores and threads*/
    sem_unlink("barrier_sem_dln45");
    sem_unlink("counter_sem_dln45");
    sem_unlink("update_sem_dln45");
    sem_unlink("update_counter_sem_dln45");
    free ((void *) thread_id);
    return total_iter;
}

/* The function executed by the threads. */
void *
my_thread (void* thread_parameter)
{
    int thread_idx = (int) (long) thread_parameter;
    thread_idx++;
    float old, new;
    while (!done2){
		float diff_temp = 0.0;
		int no_el = 0;
        for (int i = thread_idx; i < (grid_2->dim - 1); i+= num_threads) {    
            for ( int j = 1; j < (grid_2->dim - 1); j++) {
                old = grid_2->element[i * grid_2->dim + j]; /* Store old value of grid point. */
                /* Apply the update rule. */	
                new = 0.25 * (grid_2->element[(i - 1) * grid_2->dim + j] +\
                              grid_2->element[(i + 1) * grid_2->dim + j] +\
                              grid_2->element[i * grid_2->dim + (j + 1)] +\
                              grid_2->element[i * grid_2->dim + (j - 1)]);

                grid_temp->element[i * grid_2->dim + j] = new; /* Update temporary grid with the new grid-point value. */
                diff_temp += fabs(new - old); /* Calculate the difference in values. */
                no_el++;
            }
        }
		  diff2 += diff_temp;
		  num_elements2 += no_el;
        /*Updating section of each iteration*/
        update_sync(&barrier2); //Wait here before start updating section
        for (int i = thread_idx; i < (grid_2->dim - 1); i+=num_threads) {    
            for ( int j = 1; j < (grid_2->dim - 1); j++) {
                grid_2->element[i * grid_2->dim + j] = grid_temp->element[i * grid_2->dim + j];
            }
        }
        /* Wait here for all threads at the end of each iteration */
        barrier_sync (&barrier); 
    }

    pthread_exit (NULL);
}

/* The function that implements the barrier synchronization. */
void 
update_sync (BARRIER2 *barrier2)
{
    sem_wait (&(barrier2->update_counter_sem_dln45));
    if (barrier2->update_counter == (num_threads - 1)) 
    {
        barrier2->update_counter = 0;
        sem_post (&(barrier2->update_counter_sem_dln45));
        for (int i = 0; i < (num_threads-1); i++)
            sem_post (&(barrier2->update_sem_dln45)); 
    }
    else
    {
        barrier2->update_counter++;
        sem_post (&(barrier2->update_counter_sem_dln45));
        sem_wait (&(barrier2->update_sem_dln45));
    }
}

void 
barrier_sync (BARRIER *barrier)
{
    sem_wait (&(barrier->counter_sem_dln45));
    /* Check if all threads before us, that is NUM_THREADS-1 threads have reached this point */
    if (barrier->counter == (num_threads - 1)) 
    {
        /*Check convergence*/
        diff2 = diff2/num_elements2;
        printf ("Iteration %d. DIFF: %f.\n", total_iter, diff2);
        total_iter++;
        if (diff2 < eps){
            done2 = 1;
        }

        /*Reset after each iteration*/
        diff2 = 0.0;
        num_elements2 = 0;
        barrier->counter = 0; /* Reset the counter */
        /* Signal the blocked threads that it is now safe to cross the barrier */			 
        sem_post (&(barrier->counter_sem_dln45));
        for (int i = 0; i < (num_threads-1); i++)
            sem_post (&(barrier->barrier_sem_dln45));   
    } 
    else {
        barrier->counter++; // Increment the counter
        sem_post (&(barrier->counter_sem_dln45));
        sem_wait (&(barrier->barrier_sem_dln45)); // Block on the barrier semaphore and wait for someone to signal us when it is safe to cross
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



		

