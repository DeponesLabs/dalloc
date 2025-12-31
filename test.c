#include <stdio.h>
#include "dalloc.h"

int main()
{
	/*
	printf("*** dalloc Stage 1 Test ***\n ");

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
	//////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////

	/*
	printf("*** dalloc Stage 2: Free & Reuse ***\n");

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
	*/

	//////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////

	/*
	printf("*** dalloc Stage 3: Coalescing ***\n");
	
    // Allocate 3 adjacent block
    void *p1 = dalloc(16);
    void *p2 = dalloc(16);
    void *p3 = dalloc(16); // p3 is barrier.
    
    printf("p1: %p (16 byte)\n", p1);
    printf("p2: %p (16 byte)\n", p2);
    printf("p3: %p (16 byte) [Barrier]\n", p3);

    // Free up p1 and p2
    dfree(p1);
    dfree(p2);
    // Two adjacent free spaces created in memory.: [FREE 16] [FREE 16] [USED p3]
    printf("*** p1 ve p2 freed  ***\n");

    // Normally, if we request 32 bytes, p1 (16) and p2 (16) alone is not enough (16)
    // If Coalesce works: p1 + Header + p2 will combine to form a huge block. 
    // And dalloc will give us back the address of p1.
    void *pBigBlock = dalloc(32); 
    printf("Request for a big block (32 byte)...\n");
    printf("pBigBlock: %p\n", pBigBlock);

    if (pBigBlock == p1)
    	printf("Freed blocks successfully merged.\n");
    else
    	printf("It didn't work as expected. New space allocated or another block from the free list has been given.\n");

	*/

	//////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////

	printf("*** dalloc Stage 4: Splitting ***\n");
	
    // Request a big block
    printf("1. Request for a big block (256 byte)...\n");
    void *pBigBlock = dalloc(256);
    printf("pBigBlock: %p\n", pBigBlock);

    // Free big block
    dfree(pBigBlock);
    printf("*** pBigBlock Free edildi ***\n");

    // Request small block
    printf("2. Small block requested (32 byte)...\n");
    void *pSmallBlock1 = dalloc(32);
    printf("pSmallBlock1: %p\n", pSmallBlock1);

    // Check if the address is same?
    if (pSmallBlock1 == pBigBlock)
        printf("Address is same (Reuse OK).\n");
    else
        printf("It didn't work as expected. Address is different!\n");
    
    // 4. İkinci küçük parçayı iste
    // EĞER SPLIT ÇALIŞIYORSA:
    // Bu istek için sbrk çağrılmamalı, p_big'in artan kısmından verilmeli.
    // Adresi de p_small1 + 32 + Header kadar ileride olmalı.

    // Request second small block
    printf("3. Requesting second small block (32 byte)...\n");
    void *pSmallBlock2 = dalloc(32);
    printf("pSmallBlock2: %p\n", pSmallBlock2);

    // Calc diff
    long diff = (char*)pSmallBlock2 - (char*)pSmallBlock1;
    printf("Diff: %ld byte\n", diff);

    // Header(32) + Data(32) = Diff must be 64 byte
    if (diff >= 48 && diff <= 80)	// Check it in a tolerated manner. Consider alignment
        printf("Block found. Extra part is used.\n");
    else
        printf("It didn't work as expected. Diff is larger than it should be. A new sbrk syscall may have been done");
       
    return 0;
}
