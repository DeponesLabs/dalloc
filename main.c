#include <stdio.h>
#include <string.h>
#include "dalloc.h"

int main()
{
	// *******************************************************************
    // Stage 1: Basic Allocation
    // *******************************************************************
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
	printf("Diff in bytes: %td byte\n", diff); 	// %td: format specifier for ptrdiff_t
	
	// Check alignment. Address must end with 0, a multiple of 16.
    if (((unsigned long)p1 & 15) == 0)
    	printf("p1 is  16-byte aligned.\n");
    else
    	printf("p1 is not aligned! (%p)\n", p1);

	printf("\n");
	// *******************************************************************
    // Stage 2: Free & Reuse
    // *******************************************************************
	printf("*** dalloc Stage 2: Free & Reuse ***\n");

	// 1. Allocate spaces (Use previous pointers or new ones)
	// p1 was 10 bytes. Let's free it. (Check Stage 1 for p1 initialization)
	printf("--- Free p1 ---\n");
	dfree(p1);

	// 2. Request 10 bytes again
    // Expectation: dalloc should provide the old p1 address instead of a new one
    void *p3 = dalloc(10);
    printf("3. Alloc p3: %p\n", p3);

    if (p1 == p3)
		printf("The old address has been reused (Recycled)!..\n");
	else
		printf("It didn't work as expected. New area allocated!..\n");

	printf("\n");
	// *******************************************************************
    // Stage 3: Coalescing
    // *******************************************************************
	printf("*** dalloc Stage 3: Coalescing ***\n");
	
    // Allocate 3 adjacent block
    p1 = dalloc(16);
    p2 = dalloc(16);
    p3 = dalloc(16); // p3 is barrier.
    
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

	printf("\n");
	// *******************************************************************
    // STAGE 4: SPLITTING
    // *******************************************************************
	printf("*** dalloc Stage 4: Splitting ***\n");
    	    
    // Request a big block (256)
    void *sBig = dalloc(256);
    printf("1. Big block (256): %p\n", sBig);

    // Free big block
    dfree(sBig);
    printf("*** Big block freed ***\n");

    // Request small block
    printf("2. Small block requested (32 byte)...\n");
    void *sSmall1 = dalloc(32);
    printf("sSmall1: %p\n", sSmall1);

    // Check if the address is same?
    if (sSmall1 == sBig) printf("Address is same (Reuse OK).\n");

    // Request second small block (This tests SPLITTING)
    printf("3. Requesting second small block (32 byte)...\n");
    void *sSmall2 = dalloc(32);
    printf("sSmall2: %p\n", sSmall2);

    // Calc diff
    long split_diff = (char*)sSmall2 - (char*)sSmall1;
    printf("Diff: %ld byte\n", split_diff);

    if (split_diff >= 48 && split_diff <= 80)
        printf("Block found via SPLITTING. Extra part is used.\n");
    else
        printf("Splitting failed.\n");

    printf("\n");
    // *******************************************************************
    // v2.0: Realloc & Calloc
    // *******************************************************************
    printf("--- dalloc v2: Realloc & Calloc Test ---\n");
    
    // [TEST 1] Calloc
    printf("\n[TEST 1] dcalloc(5, sizeof(int))\n");
    int *arr = dcalloc(5, sizeof(int));
    
    int all_zero = 1;
    for(int i = 0; i < 5; ++i) 
        if (arr[i] != 0) all_zero = 0;
            
    if (all_zero) printf("Memory is cleaned (Zero initialized).\n");
    else printf("Memory is not cleaned!\n");

    // [TEST 2] Expansion (In-Place)
    printf("\n[TEST 2] In-Place Realloc (Expansion)\n");
    
    char *r1 = dalloc(16);
    strcpy(r1, "Hello dalloc");
    printf("r1 (Old): %p, Data: %s\n", r1, r1);

    char *rBarrier = dalloc(16); // Barrier
    dfree(rBarrier); // Free barrier to allow expansion
    printf("*** Barrier freed ***\n");

    // Reallocate r1 (32 byte) -> Should consume rBarrier
    char *r3 = drealloc(r1, 32);
    printf("r3 (New): %p, Data: %s\n", r3, r3);

    if (r1 == r3) printf("Address is same, expanded in place.\n");
    else printf("Address changed.\n");

    // [TEST 3] Shrink & Merge
    printf("\n[TEST 3] Shrink & Merge\n");
    
    // A. Allocate a huge block (64)
    void *pHuge = dalloc(128);
    printf("p_huge: %p (128 byte)\n", pHuge);

    // B. Put a barrier next to it, then free it (Create empty space)
    void *pNextFree = dalloc(32);
    dfree(pNextFree);
    printf("The block next to it freed.\n");

    // C. Shrink pHuge (to 32) 
    // The space remained should merge with the next
    void *pShrunk = drealloc(pHuge, 32);
    printf("pShrunk: %p (32 byte)\n", pShrunk);

    // D. Proof: Request 64 byte again. dalloc should return merged blocks.
    void *pProof = dalloc(64);
    printf("p_proof : %p (Checking reuse of merged space)\n", pProof);

    if ((char*)pProof > (char*)pShrunk && (char*)pProof < (char*)pShrunk + 100)
        printf("Shrank and merged..\n");
    else
        printf("Allocated from a different space.\n");

    return 0;
}
