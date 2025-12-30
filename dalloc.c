#include <unistd.h>
#include <string.h> // memset etc..
#include "dalloc.h"

// union trick for 16-byte alignment.

typedef union header {
	struct {
		size_t size; 		// Allocation Size 
		unsigned is_free;
		union header *next;
	} s;
	unsigned __int128 alignment_enforcer;
} header_t;

#define ALIGNMENT 16
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

void *dalloc(size_t size)
{
	size_t total_size;
	void *block;
	header_t *header;

	if (size == 0)
		return NULL;

	// Align the user's request and add header margin
	// Note: The header is already aligned thanks to the union, we are only aligning the size.
	size_t aligned_size = ALIGN(size);
	total_size = sizeof(header_t) + aligned_size;

	// Request space from the operating system.
	if ((block = sbrk(total_size)) == (void *) -1)
		return NULL;	// No memory

	// Insert the header
	header = (header_t *)block;
	header->s.size = aligned_size;	// How much space will be freed up when you make it free in the future?
	header->s.is_free = 0;
	header->s.next = NULL;	// Not linked to a list atm.

	// return the address after header
	return (void*)(header + 1);
	
	

	
}
