#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>	// for usleep
#include "dalloc.h"

#define THREAD_COUNT 10
#define ITERATION_COUNT 100

void *worker_routine(void *arg)
{
	int id = *(int *)arg;

	printf("Thread %d started.\n", id);

	for (int i = 0; i < ITERATION_COUNT; ++i) {
		// 1. Request random size space
		size_t size = (rand() % 64) + 16;	// Between 16 and 80
		int *pData = (int *)dalloc(size);

		if (pData == NULL) {
			fprintf(stderr, "Error: Thread #%d cannot find free space", id);
			pthread_exit(NULL);
		}
		
		// 2. Write data. 
		// If there is any conflict, data will be corrupted.
		*pData = id * 1000 + i;	// Unique data

		// To increase 'race condition'
		usleep(10);

		// 3. Recursive Mutex Test (Dreallock)
		// Make the block very large so that it becomes a "Relocation". 
		// dalloc is called internally during relocation. 
		// If the Recursive Mutex is not working, a deadlock will occur here, and the program will freeze.
		int *pNewData = (int *)drealloc(pData, 2048);

		if (pNewData == NULL) {
			fprintf(stderr, "Error: Realloc didn't work at thread #%d!\n", id);
			// If drealloc fails, the old pointer is still valid, free it.
			dfree(pData);
			pthread_exit(NULL);
		}

		// 4. Read: Check the data (Is it still what we wrote?)
		if (*pNewData != id * 1000 + i) {
			fprintf(stderr, "Fatal Error: Thread #%d data is corrupted! (Race Condition)", id);
			exit(1);
		}

		// 5. Free it.
		dfree(pNewData);
	}

	printf("Thread #%d completed.\n", id);

	return NULL;
}

int main()
{
	printf("*** dalloc Multi-Thread Stress Test ***\n");
    printf("Thread Count: %d, Loop: %d\n", THREAD_COUNT, ITERATION_COUNT);

    pthread_t threads[THREAD_COUNT];
    int thread_ids[THREAD_COUNT];

    for (int i = 0; i < THREAD_COUNT; ++i) {
    	thread_ids[i] = i + 1;
    	if (pthread_create(&threads[i], NULL, worker_routine, &thread_ids[i]) != 0) {
    		perror("Cannot create thread");
    		return 1;
    	}
    }

    for (int i = 0; i < THREAD_COUNT; i++)
		pthread_join(threads[i], NULL);

	printf("\nTest is successful! All threads completed without error.\n");
	printf("If there was a Deadlock, the program would freeze.\n");
	printf("If there was a Race Condition, we would get a 'Segmentation Fault' or the data would be corrupted.\n");

	return 0;
}
