#include <unistd.h>
#include <string.h> // memset etc..
#include <pthread.h> 
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

header_t *head = NULL;
header_t *tail = NULL;

pthread_mutex_t global_malloc_lock;

header_t *get_free_block(size_t size) 
{
    header_t *curr = head;

    while (curr) {
        if (curr->s.is_free && curr->s.size >= size) 
            return curr;
        curr = curr->s.next;
    }
    
    return NULL;
}

void *dalloc(size_t size)
{
	size_t total_size;
	void *block;
	header_t *header;

	if (size == 0)
		return NULL;

	// Align the user's memory request and add header margin
	// The header is already aligned thanks to the union, only aligning the size
	size_t aligned_size = ALIGN(size);
	total_size = sizeof(header_t) + aligned_size;

	header = get_free_block(aligned_size);

	if (header) {
		// Found available space in free list
		header->s.is_free = 0;
		return (void*)(header + 1);
	}

	// If no available space in free list, request it from OS using sbrk() sys-call
	block = sbrk(total_size);
	if (block == (void *) -1)
		return NULL;

	// Create header with new block
	header = (header_t *)block;
	header->s.size = aligned_size;	// How much space will be freed up when it is freed in the future?
	header->s.is_free = 0;
	header->s.next = NULL;	// Not linked to a list at the moment.

	// Add new block into free (linked) list
	if (!head)
		head = header;	// Add as first element

	if (tail)
		tail->s.next = header;	// Link to the end of the old

	tail = header; 	// Update tail
	
	// return the address after header
	return (void*)(header + 1);
}

void dfree(void *ptr)
{
	if (!ptr)
		return;

	// 1. Go back to header using pointer arithmetic
	// We gave the user (header + 1), now we are returning 1.
	header_t *header = (header_t *)ptr - 1;
	header->s.is_free = 1;
}
