# Memory Allocator

## Description

This project implements a custom, minimalistic memory allocator in C for Linux systems. It provides functional replacements for the standard `malloc()`, `calloc()`, `realloc()`, and `free()` functions. The allocator manages virtual memory using `brk()` for small heap-based allocations and `mmap()` for larger memory chunks, incorporating efficiency strategies such as block reuse, splitting, coalescing, and memory alignment.

## Objectives

* Implement low-level memory management using `brk()`, `mmap()`, and `munmap()` syscalls.
* Manage metadata using a doubly-linked list of `block_meta` structures.
* Minimize external and internal fragmentation through block splitting and coalescing.
* Ensure 8-byte memory alignment for all allocations.
* Optimize performance using heap preallocation and a "best-fit" block selection strategy.

## API Reference

The library provides the following interface as defined in `utils/osmem.h`:

* `void *os_malloc(size_t size)`: Allocates `size` bytes. Uses `brk()` for sizes below `MMAP_THRESHOLD` (128 KB) and `mmap()` otherwise.
* `void *os_calloc(size_t nmemb, size_t size)`: Allocates and zero-initializes memory for an array. Uses `brk()` for sizes below `page_size` and `mmap()` otherwise.
* `void *os_realloc(void *ptr, size_t size)`: Resizes an existing allocation. Attempts to expand current heap blocks before moving data.
* `void os_free(void *ptr)`: Marks heap blocks as free for reuse or unmaps memory allocated via `mmap()`.

## Project Structure

* `src/`: Contains the implementation of the memory allocator.
* `utils/`: Header files (`osmem.h`, `block_meta.h`) and a heap-independent `printf` implementation.
* `tests/`: Comprehensive test suite including functional snippets and reference outputs.
* `checker/`: Infrastructure for automated grading and syscall verification.

## Implementation Details

### Metadata Management

Each memory block is preceded by a `struct block_meta` header:

```
struct block_meta {
	size_t size;
	int status;
	struct block_meta *prev;
	struct block_meta *next;
};

```

### Strategies

* **Alignment**: All addresses (metadata and payload) are aligned to 8 bytes.
* **Preallocation**: On the first `malloc` call, a 128 KB chunk is allocated via `brk()` to reduce syscall overhead.
* **Best-Fit**: The allocator searches the entire list for the smallest free block that satisfies the requested size.
* **Splitting**: If a free block is significantly larger than requested, it is split into an allocated block and a new free block.
* **Coalescing**: Adjacent free blocks are merged into a single larger block during reallocation or before searching for a free block.

## Building and Testing

### Build the Library

Navigate to the `src/` directory and run:

```
make

```

This produces `libosmem.so`.

### Run Tests Locally

The project uses a Python-based checker to verify syscalls and correctness.
Requires `ltrace` and standard build tools.

```
cd tests/
make check
```

To run with memory leak detection:

```
python3 run_tests.py -m

```

### Coding Style

The project enforces strict style guidelines using `checkpatch.pl`, `cpplint`, and `shellcheck`. Run the linters via:

```
cd tests/
make lint
```

## Usage in Other Projects

To link against this library, include `osmem.h` and link the shared object:

```
gcc -I./utils -L./src my_program.c -losmem -o my_program
```
