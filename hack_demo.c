#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "dalloc.h"
#include "dstring.h"

void attack_1_fail();
void attack_2_success();

// ***********************************************************************
// Vulnerable Function (Unsafe dcalloc)
// ***********************************************************************
void *dcalloc_unsafe(size_t n, size_t size)
{
	// No Integer Overflow Control
	size_t total = n * size;

	printf("\n[System] Requested: %zu element x %zu byte\n", n, size);
    printf("[System] Mathematical Result (Overflow Simulation): %zu byte\n", total);

	// As a result of the overflow, the system allocates 32 bytes (a very small amount) of space.
	void *ptr = dalloc(total);

	// But we assume we're allocating a huge amount of space to the user and then memset it.
	if (ptr) 
		memset(ptr, 0, total);

	return ptr;
}
// ***********************************************************************
// Victim Structure
// ***********************************************************************
typedef struct {
	char username[16];
	int isAdmin;	// 0: Normal, 1: Admin
} UserConfig;

int main()
{
	printf("*** DEPONES LABS - HEAP OVERFLOW SIMULATION ***\n");
	printf("*** Integer Overflow Demo ***\n");

	attack_1_fail();
	attack_2_success();

    return 0;
}

// ***********************************************************************
// SCENARIO 1: FAILED ATTACK
// Logic: The victim (admin) is created first, located at a lower address in memory. 
// The hacker is located at a higher address. Since the overflow is upward, it cannot hit the victim.
// ***********************************************************************
void attack_1_fail() 
{
    printf("\n\n*** SCENARIO 1: VICTIM FIRST (LOW ADDRESS) ***\n");
    printf("Explanation: Memory loads from bottom to top (Low -> High).\n");
    printf("Since the victim is behind, the overflow will be inaccurate.\n");

    // Step 1. Create an Admin Panel as victim (Low Address)
    UserConfig *pAdminPanel = dalloc(sizeof(UserConfig));
    dstrncpy(pAdminPanel->username, "user_fail", sizeof(pAdminPanel->username));
    pAdminPanel->isAdmin = 0;   // Not admin!

    printf("1. Admin Panel created (%p)\n", pAdminPanel);
	printf("   Username: %s\n", pAdminPanel->username);
	printf("   Status: %s\n", pAdminPanel->isAdmin ? "Admin" : "User");

    // Step 2. Attack begins
    printf(" -> Attack begins...\n");

    // Create Hacker Buffer (High Address)
    // The hacker is exploiting a system vulnerability (dcalloc_unsafe).
	// The system allocates 32 bytes, but the hacker thinks they can access more.
    int *pHackerBuffer = dcalloc_unsafe(4, 8); // 32 byte

    printf(" [1] Admin Panel Address: %p\n", pAdminPanel);
    printf(" [2] Hacker Buffer Address: %p\n", pHackerBuffer);

    // Step 3: Buffer Overflow
    printf("\n3. Buffer is being overflowing...\n");

    // The hacker is writing up to the 50th element (Normally the limit should be 8)
    for (int i = 0; i < 50; i++) {
        // Did we find the target address (isAdmin)?
        if ((void*)&pHackerBuffer[i] == (void*)&pAdminPanel->isAdmin) {
            printf("   ---> [BINGO!] Target Address Hit (Offset: %d)!\n", i);
            pHackerBuffer[i] = 1; // Escalate Privilege
        } 
        else
            pHackerBuffer[i] = 0x41414141; // Trash data
    }

    // Step 4. Result
    printf("\n4. Latest Status Check\n"); 
    printf(" User: %s\n", pAdminPanel->username); 
    printf(" Authorization Status: %d\n", pAdminPanel->isAdmin);

    if (pAdminPanel->isAdmin == 1) {
        printf("\nHacked! Permission changed to 'Admin'.\n");
    	printf("This is what would have happened if there hadn't been mathematical protection inside dcalloc().\n");
    }
    else
        printf("\nSystem Secure. (If you receive this message, the hack has failed)\n");
        
    // Clean-up for next test
    dfree(pHackerBuffer);
    dfree(pAdminPanel);
}

// ---------------------------------------------------------
// SCENARIO 2: SUCCESSFUL ATTACK
// Logic: Hacker created first. (Low Address).
// Victim comes later (High Address). When hacker overflows the memory, overwrites the victim's memory.
// ---------------------------------------------------------
void attack_2_success()
{
    printf("\n\n*** SCENARIO 2: HACKER ONCE (LOW ADDRESS) ***\n");
    printf("Explanation: Hacker is below, Admin Panel is above (in the line of fire).\n");
    printf("Overflow overwrites the Admin Panel's struct.\n");

    // Step 1. Create  Hacker Buffer (Low Address)
    int *pHackerBuffer = dcalloc_unsafe(4, 8); 

    // Step 2. Create an Admin Panel as victim (High Address - Target)
    UserConfig *pAdminPanel = dalloc(sizeof(UserConfig));
    dstrncpy(pAdminPanel->username, "user_victim", sizeof(pAdminPanel->username));
    pAdminPanel->isAdmin = 0;	// Not admin!

	printf(" [1] Hacker Buffer Address: %p\n", pHackerBuffer);
	printf(" [2] Admin Panel Address: %p\n", pAdminPanel);
	    
    
    // Calculate the distance between buffers in memory
    long diff = (void*)pAdminPanel - (void*)pHackerBuffer;
    printf(" [DEBUG] Distance: %ld byte. (Attack range must exceed this)\n", diff);

    // Step 3. Attack begins (Overflow)
    printf("   -> Attack begins...\n");
    
    // 50 int = 200 bytes. It easily overcomes the 64-byte difference.
    for (int i = 0; i < 50; i++) {
        // Did we find the target address (isAdmin)?
        if ((void*)&pHackerBuffer[i] == (void*)&pAdminPanel->isAdmin) {
            printf("      ---> [BINGO] Address hit (Offset: %d)!\n", i);
            pHackerBuffer[i] = 1; // // Escalate Privilege
            break; // Target hit, stop
        } 
        else
            pHackerBuffer[i] = 0xDEADBEEF; // Progress by filling it with trash data.
    }

    // 4. Result
    if (pAdminPanel->isAdmin == 1) 
    	printf("\nHacked! New Permission Level: %d (ADMIN) of %s.\n", pAdminPanel->isAdmin, pAdminPanel->username);
    else
        printf("Hack attempt failed. (Cannot exceed attack range?)\n");

    printf("\n*** [FORENSIC REPORT] Victim's Memory Status ***\n");
    unsigned char *p = (unsigned char*)pAdminPanel;
    
    // Print the memory byte by byte, up to the size of the UserConfig (e.g., 20 bytes).
    for (int i = 0; i < sizeof(UserConfig); ++i) {
        printf("%02X ", p[i]);
        
        // When the username ends (by the 16th byte), draw a line.
        if (i == 15) printf("| "); 
    }
    
    printf("\n");
    printf("  ^-- Username Space --^   ^-- isAdmin --^\n");
    
    // Clean-up memory
    dfree(pAdminPanel);
    dfree(pHackerBuffer);
}
