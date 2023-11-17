// SPDX-License-Identifier: BSD-3-Clause
#include "memory.h"
#include "block_meta.h"

void *sbrk(int incr);
// TODO meybe infinite loop at calloc for block reuse////////////
// function to calculate the size using padding for 8 bits
size_t calculate_size(size_t size)
{
	// check if size is a multiple of 8
	if (size % 8 == 0)
		return size;

	int rest = size % 8;

	// return the wanted value
	return size + (8 - rest);
}

// define the size of structure with padding
#define SIZE_STRUCT calculate_size(sizeof(struct block_meta))

void *allocate_memory(size_t size, size_t limit)
{
	void *addr = NULL;
	// add the size of the structure
	size = calculate_size(size) + SIZE_STRUCT;

	// check the size
	if (size < limit) {
		// for low values
		size_t actual_size = size;
		// if it is the first block or the first block on the heap
		if (mem_list.next == NULL || mem_list.next->status == STATUS_MAPPED)
			// preallocate
			actual_size = MMAP_THRESHOLD;

		// use sbrk with the size of the block
		addr = sbrk(actual_size);

		// check for errors
		if (addr == (void *)-1)
			DIE(1, "sbrk");

		// add the first block
		add_block(addr, size, STATUS_ALLOC);

		if (actual_size == size)
			// add the second block
			add_block((void *)((char *)addr + size), actual_size - size, STATUS_FREE);

	} else {
		// use mmap on addr
		addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);

		// check for errors
		if (addr == (void *)-1)
			DIE(1, "mmap");

		// add the memory block
		add_block(addr, size, STATUS_MAPPED);
	}

	// returns the address of the structure
	return addr;
}

// initialize a linked list
void init_list(void)
{
	// initialize the values
	mem_list.next = NULL;
	mem_list.prev = NULL;
	mem_list.size = 0; // assign the initial value of the heap
	mem_list.status = STATUS_FREE;
}

// function that adds a block in the back of the list (addr is the address of the structure)
void add_block(void *addr, size_t size, int stat)
{
	// check if it is valid
	if (addr == NULL || size <= SIZE_STRUCT)
		return;

	// create a new block_meta from the provided address
	struct block_meta *new_block = (struct block_meta *)addr;
	new_block->next = NULL;
	new_block->size = size - SIZE_STRUCT;
	new_block->status = stat;

	// if the list is empty set the new block as the first block
	if (mem_list.next == NULL) {
		new_block->prev = &mem_list;
		mem_list.next = new_block;
		return;
	}

	struct block_meta *current = &mem_list;
	/*in the linked list there will be nodes that point to heap blocks
	 * and after them the nodes that point to maped pages
	 */
	if (stat == STATUS_ALLOC) {
		// traverse until the last block from the heap
		while (current->next != NULL && current->status != STATUS_MAPPED)
			current = current->next;

		// if there are no STATUS_MAPPED nodes in the list
		if (current->next == NULL) {
			// add at the end of the list
			current->next = new_block;
			new_block->prev = current;
		} else { // if there is a STATUS_MAPPED node
			// make the links
			new_block->next = current->next;
			current->next->prev = new_block;
			current->next = new_block;
			new_block->prev = current;
		}
	} else {
		// find the last node
		while (current->next != NULL)
			current = current->next;

		// connect the new block to the last block
		new_block->prev = current;
		current->next = new_block;
	}
}

// fucntion that splits a free block
struct block_meta *split_block(struct block_meta *item, size_t new_size)
{
	// get the new node
	struct block_meta *aux = (struct block_meta *)((char *)item + SIZE_STRUCT + new_size);

	if (item->next)
		item->next->prev = aux;

	// add the node
	aux->next = item->next;
	aux->prev = item;
	item->next = aux;

	// compute the new size of the free block
	aux->size = calculate_size(item->size) - new_size - SIZE_STRUCT;
	aux->status = STATUS_FREE;
	printf("%ld\n", aux->size);
	item->size = new_size;
	// the function returns the free block of size aux->size
	return aux;
}

// function that returns a pointer of the last block on the heap
struct block_meta *last_block_heap()
{
	// init the pointer to traverse the list
	struct block_meta *item = &mem_list;

	// traverse the list
	while (item->next && item->status != STATUS_MAPPED)
		item = item->next;

	if (item->status == STATUS_FREE && item != &mem_list)
		return item;

	// if there are no blocks on the heap
	return NULL;
}

// function that modifies the size of the last block on the heap
void resize_last_block(struct block_meta *block, size_t new_size)
{
	// new_size doesn t contain the SIZE_STRUCT value
	new_size = calculate_size(new_size);

	void *res;
	// if extending the last block
	if (new_size > block->size) {
		// call the sbrk function
		res = sbrk(new_size - block->size);

	} else {
		res = sbrk(block->size - new_size);
	}

	// check for errors
	if (res == (void *)-1)
		DIE(1, "sbrk");


	// modify the size of the last block
	block->size = new_size;
}

// find the best block
void *find_best_block(size_t size_payload)
{
	// check if the list is empty
	if (mem_list.next == NULL)
		return NULL;

	// calculate the size of the payload with padding
	size_payload = calculate_size(size_payload);

	// initialize variables to keep track of the best block
	struct block_meta *best_block = NULL;
	size_t min_size = 0x20002; // TODO modify

	// traverse the list of free memory blocks
	struct block_meta *current = mem_list.next;

	while (current != NULL) {
		// check if the block is free and its size is sufficient for the payload
		if (current->status == STATUS_FREE && current->size >= size_payload) {
			// update the best block if this one is smaller
			if (current->size < min_size) {
				best_block = current;
				min_size = current->size;
			}
		}
		// move to the next block
		current = current->next;
	}

	// if a suitable block was found
	if (best_block != NULL) {
		// split if necesarry
		if (best_block->size - size_payload >= SIZE_STRUCT + calculate_size(1))
			split_block(best_block, size_payload);

	} else {
		// check if the last block on heap is free
		best_block = last_block_heap();

		// if there is a block on the heap
		if (best_block != NULL && best_block->status == STATUS_FREE) {
			// make the last block on the heap bigger
			resize_last_block(best_block, size_payload);
			best_block->size = size_payload;
		}
	}

	// returns the address of the structure
	return (void *)best_block;
}

// function that merges free blocks
void coalesce_blocks(struct block_meta *item, int status) // item is the recently freed block
{
	// check if it is the head of the list or NULL
	if (item == NULL || item == &mem_list)
		return;

	// coalesce with the following block
	if (item->next && item->next->status == STATUS_FREE && item->status == status) {
		item->size += item->next->size + SIZE_STRUCT;

		item->next = item->next->next;

		if (item->next)
			item->next->prev = item;
	}
}

// function that copies the memory
void mem_cpy(void *dest, void *src, size_t size)
{
	char *ptr1 = (char *)dest;
	char *ptr2 = (char *)src;

	while (size != 0) {
		*ptr1 = *ptr2;

		ptr1++;
		ptr2++;
		size--;
	}
}

// function that removes a block from the list
void remove_block(struct block_meta *node)
{
	if (node == NULL)
		return;

	// update the next pointer of the previous node
	if (node->prev != NULL)
		node->prev->next = node->next;

	// update the previous pointer of the next node
	if (node->next != NULL)
		node->next->prev = node->prev;
}
