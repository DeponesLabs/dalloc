#ifndef DSTRING_H
#define DSTRING_H

#include <stddef.h>

/**
 * @brief Safe string copy function (Depones Safe Copy).
 *
 * Unlike standard strncpy, this function guarantees that the destination buffer
 * is always null-terminated.
 *
 * @param dst   Destination buffer.
 * @param src   Source string.
 * @param size  Total size of the destination buffer (sizeof(buffer)).
 * * @return size_t Total length of the source string (src).
 * If the return value is >= size, truncation occurred.
 *
 * @note This function does not handle overlapping buffers.
 */
size_t dstrncpy(char *dst, const char *src, size_t size);

#endif // DSTRING_H
