// SPDX-License-Identifier: BSD-3-Clause
#include "block_meta.h"
#include "osmem.h"

// include the list header
#include "memory.h"

// variable that sees if the list is initialized
int init = 0;

void *os_malloc(size_t size)
{
	// check for valid size
	if (size <= 0)
		return NULL;

	// check the list
	if (init == 0) {
		// init the list
		init_list();
		init = 1;
	}

	// get the address of the structure
	void *addr = find_best_block(size);

	// if it is needed to allocate more to the heap
	if (addr == NULL) {
		// returns the address of the structure
		addr = allocate_memory(size, MMAP_THRESHOLD);

	} else { // if a block was found
		struct block_meta *item = (struct block_meta *)addr;

		item->status = STATUS_ALLOC; // change the status
	}

	// return the address of the payload
	return (char *)addr + SIZE_STRUCT;
}

void os_free(void *ptr)
{
	// check if the address is valid
	if (ptr == NULL)
		return;

	// set the structure to free
	struct block_meta *item = (struct block_meta *)((char *)ptr - SIZE_STRUCT);

	if (item->status == STATUS_ALLOC) {
		item->status = STATUS_FREE;
		// check for merging with the prev block
		coalesce_blocks(item->prev, STATUS_FREE);

		// check for merging with the next block
		coalesce_blocks(item, STATUS_FREE);
	} else if (item->status == STATUS_MAPPED) {
		// check if it is the only node
		if (mem_list.next == item)
			mem_list.next = NULL;

		remove_block(item);
		// unmap the page
		int res = munmap((char *)ptr - SIZE_STRUCT, item->size + SIZE_STRUCT);

		DIE(res == -1, "munmap");
	}
}

void *os_calloc(size_t nmemb, size_t size)
{
	if (size == 0 || nmemb == 0)
		return NULL;

	// check the list
	if (init == 0) {
		// init the list
		init_list();
		init = 1;
	}

	size_t total_size = size * nmemb, i;
	// get the address of the structure
	void *addr = find_best_block(total_size);

	// if it is needed to allocate more to the heap
	if (addr == NULL) {
		// returns the address of the structure
		addr = allocate_memory(total_size, page_size);

	} else { // if a block was found
		struct block_meta *item = (struct block_meta *)addr;

		item->status = STATUS_ALLOC; // change the status
	}

	// get the address of the payload
	addr =  (char *)addr + SIZE_STRUCT;

	// traverse the array and init with 0 each byte
	for (i = 0; i < nmemb * size; i++)
		*((char *)addr + i) = 0;

	// return the address of the payload
	return addr;
}

void *os_realloc(void *ptr, size_t size)
{
	if (ptr == NULL)
		return os_malloc(size);

	// check for valid input
	if (size == 0) {
		os_free(ptr);
		return NULL;
	}

	// check if the block has the status free
	struct block_meta *item = (struct block_meta *)((char *)ptr - SIZE_STRUCT);

	size_t minim = item->size;

	if (item->status == STATUS_FREE)
		return NULL;

	void *addr = NULL;

	// for a mmap block
	if (item->status == STATUS_MAPPED) {
		// relocate the block
		addr = os_malloc(size);
		mem_cpy(addr, ptr, size);
		os_free(ptr); // free the old block
		return addr;
	}

	// check if the block will shrink
	if (size <= item->size) {
		// check to split the block
		if (item->size - size >= SIZE_STRUCT + calculate_size(1)) {
			// addr has the free block
			addr = (void *)split_block(item, size);

			// get the actual block
			struct block_meta *aux = (struct block_meta *)(addr);
			addr = aux->prev;
			// return the address of the payload
			return (char *)addr + SIZE_STRUCT;
		}

		return ptr;
	}


	// check if it is the last block on the heap
	if (item->next == NULL || item->next->status == STATUS_MAPPED) {
		printf("%ld %ld\n", item->size, size);
		resize_last_block(item, size);
		return item;
	}

	// traverse the next free nodes
	struct block_meta *block = item->next;

	while (block && block->status == STATUS_FREE && item->size < size) {
		// coalesce the blocks
		coalesce_blocks(item, STATUS_ALLOC);
		block = block->next;
	}

	// check the size of the block
	if (item->size >= size)
		// return the address of the payload
		return ptr;


	// calculate the minimum size
	if (item->size < minim)
		minim = item->size;

	// relocate the block
	addr = os_malloc(size);
	mem_cpy(addr, ptr, minim);
	os_free(ptr); // free the old block

	return addr;
}
