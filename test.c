#include <stdio.h>
#include "dalloc.h"

int main()
{
	/*
	printf("--- dalloc Stage 1 Test ---\n ");

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
	
	// Check alignment. Address must end with 0, a multiple of 16.
    if (((unsigned long)p1 & 15) == 0)
    	printf("p1 is  16-byte aligned.\n");
    else
    	printf("p1 is not aligned! (%p)\n", p1);
	*/
	////////////////////////////////////////////////////////////////////////////

	printf("--- dalloc Stage 2: Free & Reuse ---\n");

	// 1. Allocate spaces
	void *p1 = dalloc(10);
	printf("1. Alloc p1: %p\n", p1);
	
	void *p2 = dalloc(20);
	printf("2. Alloc p2: %p\n", p2);

	// 2. Free-up p1
	printf("--- Free p1 ---\n");
	dfree(p1);

	// 3. Request 10 bytes again
	// Expectation: dalloc should provide the OLD p1 address instead of a new one!
	void *p3 = dalloc(10);
	printf("3. Alloc p3: %p\n", p3);
	
	if (p1 == p3)
	    printf("The old address has been reused (Recycled)!..\n");
	else
		printf("It didn't work as expected. New area allocated!..\n");
		
	return 0;
}
