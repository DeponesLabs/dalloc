#include <unistd.h>
#include <string.h> // memset etc..
#include <pthread.h> 
#include "dalloc.h"

// union trick for 16-byte alignment.

typedef union header {
	struct {
		size_t size; 		// Allocation Size 
		unsigned is_free;
		union header *pNext;
	} data;
	unsigned __int128 alignment_enforcer;
} header_t;

#define ALIGNMENT 16
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

header_t *pHead = NULL;
header_t *pTail = NULL;

pthread_mutex_t global_malloc_lock;

static header_t *get_free_block(size_t size) 
{
    header_t *pCurr = pHead;

    while (pCurr) {
        if (pCurr->data.is_free && pCurr->data.size >= size) 
            return pCurr;
            
        pCurr = pCurr->data.pNext;
    }
    
    return NULL;
}

// Walks the list from the beginning to the end
// If it finds two adjacent 'freed' blocks, it merges them
static void coalesce()
{
	header_t *pCurr = pHead;

	while (pCurr && pCurr->data.pNext) {
		// If the current one and the next one are free
		if (pCurr->data.is_free && pCurr->data.pNext->data.is_free) {
			// New size = Current one's size + next one's header + next one's size
			pCurr->data.size += sizeof(header_t) + pCurr->data.pNext->data.size;
			// Remove the next one from the list
			pCurr->data.pNext = pCurr->data.pNext->data.pNext;
		}
		else
		 	// Walk to the next one if there is no merge
			pCurr = pCurr->data.pNext;
	}
}

static void split_block(header_t *pBlock, size_t size)
{
	// block->data.size: The currently available large size (e.g., 1024)
	// size: The size requested by the user (e.g., 32)

	// Is there enough space to split?
	if (pBlock->data.size >= size + sizeof(header_t) + ALIGNMENT) {
		// Create a new pointer for the extra portion
		// Address calculation: Existing Header + Header Size + Data Size to be used
		header_t *pNewBlock = (header_t *)((void *)pBlock + sizeof(header_t) + size);

		// Set new block's data
		// New size = old total size - (Used + Header)
		pNewBlock->data.size = pBlock->data.size - size - sizeof(header_t);
		pNewBlock->data.is_free = 1;	// Free
		pNewBlock->data.pNext = pBlock->data.pNext; 

		// Set data of splitted block before sending it to the user
		pBlock->data.size = size;
		pBlock->data.is_free = 0;	// Allocating by dalloc
		pBlock->data.pNext = pNewBlock;	// pNewBlock is next to the block requested (and splitted) by the user.

		// If tail was pBlock before user's memory  request and splitting, set tail as pNewBlock
		if (pTail == pBlock)
			pTail = pNewBlock;	
	}
}

void *dalloc(size_t size)
{
	size_t total_size;
	void *pBlock;
	header_t *pHeader;

	if (size == 0)
		return NULL;

	// Align the user's memory request and add pHeader margin
	// The pHeader is already aligned thanks to the union, only aligning the size
	size_t aligned_size = ALIGN(size);
	total_size = sizeof(header_t) + aligned_size;

	// Search in the free list for recycled space
	pHeader = get_free_block(aligned_size);

	// If there is no available space in free list as large as the user requested
	if (!pHeader) {		
        coalesce(); // Merge free adjacent spaces
        pHeader = get_free_block(aligned_size); // Request again
	}

	// Found available space in free list
	if (pHeader) {
		// Split it if it is much bigger than the requested size
		split_block(pHeader, aligned_size);
		pHeader->data.is_free = 0;	// set it as not-free
		
		return (void*)(pHeader + 1);	// Return this block to the user who requested it
	}

	// If there is still no available space in free list, request it from OS using sbrk() syscall
	if ((pBlock = sbrk(total_size)) == (void *) -1)
		return NULL;

	// Create pHeader with new pBlock
	pHeader = (header_t *)pBlock;
	pHeader->data.size = aligned_size;	// How much space will be freed up when it is freed in the future?
	pHeader->data.is_free = 0;
	pHeader->data.pNext = NULL;

	// Add new pBlock into free (linked) list
	if (!pHead)
		pHead = pHeader;	// Add as first element

	if (pTail)
		pTail->data.pNext = pHeader;	// Link to the end of the old

	pTail = pHeader; 	// Update pTail
	// return the address after pHeader
	return (void*)(pHeader + 1);
}

void dfree(void *pBlock)
{
	if (!pBlock)
		return;

	// Go back to pHeader using pointer arithmetic
	// We gave the user (pHeader + 1), now we are returning 1.
	header_t *pHeader = (header_t *)pBlock - 1;
	pHeader->data.is_free = 1;
}
