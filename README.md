**`dalloc`: Userspace Memory Allocator**

`dalloc` is a minimalist implementation of the standard C library memory management functions (`malloc`, `free`, `realloc`) developed for research and educational purposes.
The project explores low-level memory handling via POSIX system calls (`sbrk`), emphasizing manual heap traversal, fragmentation analysis, and strict 16-byte memory alignment suitable for modern SIMD architectures. It serves as a study in system programming fundamentals and memory overhead management.
