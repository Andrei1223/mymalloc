# mymalloc

This is a simple memory allocator implementation in C, providing functionalities for dynamic memory allocation and deallocation. The allocator includes os_malloc, os_calloc, os_realloc, and os_free functions.

## Functions

os_malloc(size_t size)
Allocates size bytes and returns a pointer to the allocated memory. Chunks smaller than MMAP_THRESHOLD are allocated with brk(), while larger chunks use mmap(). The memory is uninitialized. Passing 0 as size will return NULL.

os_calloc(size_t nmemb, size_t size)
Allocates memory for an array of nmemb elements of size size bytes each and returns a pointer to the allocated memory. Chunks smaller than page_size are allocated with brk(), while larger chunks use mmap(). The memory is set to zero. Passing 0 as nmemb or size will return NULL.

os_realloc(void *ptr, size_t size)
Changes the size of the memory block pointed to by ptr to size bytes. If the size is smaller than the previously allocated size, the memory block will be truncated. If ptr points to a block on the heap, os_realloc() will try to expand the block first, rather than moving it. The block will be reallocated and its contents copied if needed. When attempting to expand a block followed by multiple free blocks, os_realloc() will coalesce them one at a time. Calling os_realloc() on a block that has STATUS_FREE should return NULL. Passing NULL as ptr will have the same effect as os_malloc(size), and passing 0 as size will have the same effect as os_free(ptr).

os_free(void *ptr)
Frees memory previously allocated by os_malloc(), os_calloc(), or os_realloc(). os_free() will mark the memory as free and reuse it in future allocations. In the case of mapped memory blocks, os_free() will call munmap().

## General

Allocations that increase the heap size will only expand the last block if it is free.
sbrk() can be used instead of brk(), as on Linux sbrk() is implemented using brk().
Do NOT use mremap().
Check the error code returned by every syscall using the DIE() macro.
Implementation

## Memory Alignment
Allocated memory is aligned to 8 bytes as required by 64-bit systems.
Block Reuse
Memory blocks are managed using the struct block_meta, which includes size, status, previous, and next pointers.
Both struct block_meta and the payload of a block are aligned to 8 bytes.
Split Block
Blocks are split to avoid internal memory fragmentation. The split will not occur if the remaining size is not big enough for another block.
Coalesce Blocks
Block coalescing is used to merge adjacent free blocks and reduce external memory fragmentation.
Find Best Block
The allocator uses a "find best" strategy to reuse a free block with a size closer to the needed size, reducing the number of future operations.
Heap Preallocation
A relatively large chunk of memory (e.g., 128 kilobytes) is preallocated when the heap is used for the first time, reducing the number of future brk() syscalls.
Note: Detailed information about the implementation is provided in the code comments.

