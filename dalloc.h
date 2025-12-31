#ifndef DALLOC_H
#define DALLOC_H

#include <stddef.h>

// *** Core API (v1.0) ***
void *dalloc(size_t size);
void dfree(void *ptr);

// *** Advanced API (v2.0) ***

/*
 * calloc: Allocates a numeric array of elements, each with a size of 'size'.
 * Difference: It delivers memory filled with '0' (zero-initialize).
 */
void *dcalloc(size_t num, size_t size);

/*
 * realloc: Updates the size of the block indicated by ptr to 'new_size'.
 * - If space is available, it expands in place (in-place expansion).
 * - If not, it creates new space, moves the data (memcpy), and deletes the old one.
 */
void *drealloc(void *ptr, size_t new_size);

#endif
