#include <stdio.h>
#include "dalloc.h"

int main()
{
	printf("--- dalloc Stage 0 Test ---\n ");

	// 1. Allocate 10 byte
	void *p1 = dalloc(10);
	printf("Address of p1: %p (Size: 10)\n", p1);

	// 2. Allocate 20 byte
	void *p2 = dalloc(20);
	printf("Address of p2: %p (Size: 20)\n", p2);

	// Check the address difference.
	// Casting char * for pointer arithmetic
	
	// Get the difference between two pointers as type 'ptrdiff_t'. 
	// But to tell the compiler to "count this as bytes" when performing subtraction,
	// convert the pointers to type (char *).
	ptrdiff_t diff = (char *)p2 - (char *)p1;
	printf("Diff in bytes: %td byte\n", diff); 	// %td: ptrdiff_t için format specifier
	
	if (diff == 10) 
		printf("Successful! The heap grew contiguous as expected.\n");
	else 
		printf("There is a different situation (No alignment or metadata).\n");
	
	return 0;
}
