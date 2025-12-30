#include <unistd.h> // sbrk sys-call
#include "dalloc.h"

/*
 * Stage 0: Naive Allocator
 * Only pushes the program break forward. 
 * No return (free) option.
 * No alignment.
 */

 void *dalloc(size_t size)
 {
 	// sbrk(0) gives the current break address.

	// sbrk(size): Increments the break line by 'size'. 
	// If successful: Returns the old break address (start of the block). 
	// If unsuccessful: Returns (void *) -1.

	void *block = sbrk(size);

	if (block == (void *) -1)
		return NULL;

	return block;
 }
