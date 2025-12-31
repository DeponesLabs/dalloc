#ifndef DALLOC_H
#define DALLOC_H

#include <stddef.h> // size_t için

void *dalloc(size_t size);
void dfree(void *ptr);

#endif
