#include <stdio.h>
#include "string.h"

// Depones Safe String Copy
// Returns the total length of the 'src' file. 
// If the returned value is >= size, it means a truncation has occurred.

size_t dstrncpy(char *dst, const char *src, size_t size) 
{
    const char *src_start = src;
    size_t left = size;

    // Copy if there is space in the target buffer
    if (left > 0) {
        // Subtract 1 to reserve space for NULL character (\0)
        while (--left != 0) {
            if ((*dst++ = *src++) == '\0') {
                // If the `src` (short string) ends, it's done
                // The `dst` already ends with `\0`
                return (src - src_start - 1); 
            }
        }
    }

    // If the buffer is full but the src hasn't completed
    if (size > 0) {
        *dst = '\0'; // Always use null-terminate!
    }

    // Calculate the length of the rest of the src (for truncation check)
    while (*src++)
        ;

    return (src - src_start - 1);
}
