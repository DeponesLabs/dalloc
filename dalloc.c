#include <unistd.h>
#include <string.h> // memset etc..
#include <stdint.h>	// for SIZE_MAX (or <limits.h>)
#include <pthread.h> 
#include <errno.h>

#include "dalloc.h"

static pthread_mutex_t global_malloc_lock;

// ***********************************************************************
// Cross-Platform Constructor Macros
// ***********************************************************************

// GCC or Clang (Linux, macOS, MinGW)
#if defined(__GNUC__) || defined(__clang__)

	// Constructor function which will executed after the library is loaded (before main)
	// This is a gcc/clang specific feature (constructor attribute)
    __attribute__((constructor))
    void dalloc_init_lock() 
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&global_malloc_lock, &attr);
        // printf("dalloc: Lock (Linux/Mac) started.\n");
    }

// MSVC (Microsoft Visual Studio)
#elif defined(_MSC_VER)

    // Microsoft's "Constructor" logic is a bit complex. 
    // Place a function pointer in a special memory area called .CRT$XCU. 
    // When the Windows program starts, it executes the functions in this area sequentially.
    
    #pragma section(".CRT$XCU", read)
    
    static void dalloc_init_lock(void); // Fuction prototype
    
    // Set function address in that specific field.
    __declspec(allocate(".CRT$XCU")) void (*dalloc_init_lock_ptr)(void) = dalloc_init_lock;
    
    void dalloc_init_lock() 
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&global_malloc_lock, &attr);
        // printf("dalloc: Lock (Windows) started.\n");
    }

#endif

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

// Combines the specified block with the block immediately following it
// The caller must ensure the next block exists and mergable
static void merge_next(header_t *pBlock)
{
	header_t *pNext = pBlock->data.pNext;

	// New size = Current one's size + next one's header + next one's size
	pBlock->data.size += sizeof(header_t) + pNext->data.size;
	// Move the pointer to the next one to remove the next one from the list
	pBlock->data.pNext = pNext->data.pNext;
	// If tail was pNext, set tail as pBlock
	if (pTail == pNext)
		pTail = pBlock;
}

// Walks the list from the beginning to the end
// If it finds two adjacent 'freed' blocks, it merges them
static void coalesce()
{
	header_t *pCurr = pHead;

	while (pCurr && pCurr->data.pNext) {
		// If the current one and the next one are free
		if (pCurr->data.is_free && pCurr->data.pNext->data.is_free)
			// Merge them
			merge_next(pCurr);
			// Don't advance pCurr here
			// After merging, the new bigger block might be adjacent to 
            // yet another free block. Stay to check again.
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

		// If tail was pBlock before user's memory request and splitting, set tail as pNewBlock
		if (pTail == pBlock)
			pTail = pNewBlock;	
	}
}

void *dalloc(size_t size)
{
	pthread_mutex_lock(&global_malloc_lock); // Lock
	
	size_t total_size;
	void *pBlock;
	header_t *pHeader;

	if (size == 0) {
		pthread_mutex_unlock(&global_malloc_lock); 
		return NULL;
	}

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
		
		pthread_mutex_unlock(&global_malloc_lock);
		return (void*)(pHeader + 1);	// Return this block to the user who requested it
	}

	// If there is still no available space in free list, request it from OS using sbrk() syscall
	if ((pBlock = sbrk(total_size)) == (void *) -1) {
		pthread_mutex_unlock(&global_malloc_lock);
		return NULL;
	}

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

	pthread_mutex_unlock(&global_malloc_lock);
	// return the address after pHeader
	return (void*)(pHeader + 1);
}

// *** dcalloc (Clear Allocation)
void *dcalloc(size_t n, size_t size)
{
	// Overflow Check ***************************************************************
	/* 	
	  It's too late to check after performing multiplication (because it 
	  will already be overflowing). Therefore, we will perform the check 
	  by reversing it with division.
	*/
	// In Math: 		If (num * size) > SIZE_MAX, there is an overflow. 
	// Computer Logic: 	If (num > SIZE_MAX / size), there is an overflow. 
	// 'size == 0' check is to prevent the "Division by Zero" error.
	if (size != 0 && n > SIZE_MAX / size) {
		// Overflow! Incorrect memory allocation will occur if the operation is performed. 
		// Returning a NULL error with an ENOMEM (Not enough memory)
		errno = ENOMEM;
		return NULL;
	}

	// Calculate total size (It's safe now)
	size_t total = n * size;

	// Allocate using dalloc
	void *ptr;

	if ((ptr = dalloc(total)) != NULL)
		memset(ptr, 0, total);
	
	return ptr;
}

void *drealloc(void *ptr, size_t size)
{
	// if ptr is NULL, behave like malloc
	if (!ptr)
		return dalloc(size);

	pthread_mutex_lock(&global_malloc_lock);

	// if size is 0, behave like free
	if (size == 0) {
		dfree(ptr);
		pthread_mutex_unlock(&global_malloc_lock);
		return NULL;
	}

	// Get the current header
	header_t *pHeader = (header_t *)ptr - 1;
	size_t aligned_size = ALIGN(size);


	// Scenario A: Shrinking ************************************************
	if (pHeader->data.size >= aligned_size + sizeof(header_t) + ALIGNMENT) {
		// The split_block function divides the block and attaches the remaining part to the header->next
		split_block(pHeader, aligned_size);

		// Now try merging that newly formed remainder with its neighbor on the right.
        header_t *pRemainder = pHeader->data.pNext;       // Newly formed free space
        header_t *pNeighbor = pRemainder->data.pNext;     // The neighbor to its right

        // Is there a neighbor? AND Is the neighbor's space is free
        if (pNeighbor && pNeighbor->data.is_free)
			// Merge remainder and neighbor
			merge_next(pRemainder);

		pthread_mutex_unlock(&global_malloc_lock);
        return ptr;
	}

	// Scenario B: Expansion ************************************************
	// If it hasn't shrunk, maybe we can expand it in place?
	// Current size + header + adjacent size >= Requested size?
	header_t *pNext = pHeader->data.pNext;

	if (pNext && pNext->data.is_free && pHeader->data.size + sizeof(header_t) + pNext->data.size >= aligned_size) {
		merge_next(pHeader);
		pthread_mutex_unlock(&global_malloc_lock);
		return ptr;
	}

	// Scenario C: Relocation ***********************************************
	// No free space to expand, so we'll have to reallocate it
	void *pNewBlock = dalloc(size);

	if (pNewBlock) {
		memcpy(pNewBlock, ptr, pHeader->data.size);
		dfree(ptr);
	}

	pthread_mutex_unlock(&global_malloc_lock);
	return pNewBlock;
}

void dfree(void *pBlock)
{
	if (!pBlock)
		return;

	pthread_mutex_lock(&global_malloc_lock);
	// Go back to pHeader using pointer arithmetic
	// We gave the user (pHeader + 1), now we are returning 1.
	header_t *pHeader = (header_t *)pBlock - 1;
	pHeader->data.is_free = 1;

	coalesce();
	
	pthread_mutex_unlock(&global_malloc_lock);
}
