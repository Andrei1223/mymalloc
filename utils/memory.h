#include "block_meta.h"
#include <sys/mman.h>
#include <errno.h>
#include "printf.h"

// all the functions from memory.c to be used in osmem.c
void init_list();
void *find_best_block(size_t size_payload);
void add_block(void *addr, size_t size, int stat);
void *allocate_memory(size_t size, size_t limit);
void coalesce_blocks(struct block_meta *item, int status);
size_t calculate_size(size_t size);
void mem_cpy(void *dest, void *src, size_t size);
struct block_meta *last_block_heap();
void resize_last_block(struct block_meta *block, size_t new_size);
struct block_meta *split_block(struct block_meta *item, size_t new_size);
void remove_block(struct block_meta* node);

#define SIZE_STRUCT calculate_size(sizeof(struct block_meta))
#define MMAP_THRESHOLD 0x20000
#define page_size 4096