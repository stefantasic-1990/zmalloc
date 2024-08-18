#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/mman.h>

#define ARENA_SIZE (16 * 1024 * 1024) // 16 MB
#define WORD_SIZE sizeof(void*)

// memory aligned header and footer sizes
#define MEMBLOCK_HEADER_SIZE ((sizeof(struct blockHeader) + WORD_SIZE - 1) & ~(WORD_SIZE - 1))
#define MEMBLOCK_FOOTER_SIZE ((sizeof(struct blockFooter) + WORD_SIZE - 1) & ~(WORD_SIZE - 1))

#define SMALLEST_BLOCK_SIZE (MEMBLOCK_HEADER_SIZE + MEMBLOCK_FOOTER_SIZE + WORD_SIZE)

// memory block header
struct blockHeader {
    struct blockHeader* next;
    struct blockHeader* prev;
    size_t size;
    bool free;
};

// memory block footer
struct blockFooter {
    size_t size;
    bool free;
};

struct blockHeader* freelist = NULL;

// map additional memory into free list
void mapMoreMemory() {
    // map a large block of memory
    void* arena = mmap(
        NULL, 
        ARENA_SIZE, 
        PROT_READ | PROT_WRITE, 
        MAP_PRIVATE | MAP_ANONYMOUS, 
        -1, 
        0
    );

    if (arena == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    // initialize header and footer, add to free list
    struct blockHeader* arena_header = arena;
    arena_header->next = freelist;
    arena_header->prev = NULL;
    arena_header->size = (ARENA_SIZE - MEMBLOCK_HEADER_SIZE - MEMBLOCK_FOOTER_SIZE);
    arena_header->free = true;

    struct blockFooter* arena_footer = ((char*)arena_header + MEMBLOCK_HEADER_SIZE + arena_header->size);
    arena_footer->size = arena_header->size;
    arena_footer->free = arena_header->free;

    freelist->prev = arena_header;
    freelist = arena_header;

    return 1;
}

// remove memory block from free list
void* removeBlock(struct blockHeader* block) {
    block->prev->next = block->next;
    block->next->prev = block->prev;
    return 1;
}

// coalesce memory block
void blockCoalesce(struct blockHeader* block) {
    
}

// split memory block
void blockSplit(struct blockHeader* block, size_t size) {
    // get pointer to new block footer
    struct blockFooter* block_footer = (char*)block + MEMBLOCK_HEADER_SIZE + size;
    // get pointer to split block header
    struct blockHeader* split_block_header = (char*)block + MEMBLOCK_HEADER_SIZE + MEMBLOCK_FOOTER_SIZE + size;
    // get pointer to split block footer
    struct blockFooter* split_block_footer = (char*)block + MEMBLOCK_HEADER_SIZE + (block->size - size);

    //adjust next and prev pointers
    split_block_header->next = block->next;
    split_block_header->next->prev = split_block_header;
    split_block_header->prev = block;
    block->next = split_block_header;
}

// allocate memory from the free list
void* ymalloc(size_t size) {
    // if requested block size is zero, return NULL
    if (size == 0) return NULL;

    // adjust requested size to account for memory alignment and header/footer
    size_t aligned_size = (MEMBLOCK_HEADER_SIZE + MEMBLOCK_FOOTER_SIZE + size + WORD_SIZE - 1) & ~(WORD_SIZE -1);

    struct blockHeader* block = freelist;

    // search through the free list for a suitable block
    while (block) {
        // if block size is larger than the request size and can be split
        if (block->size >= (size + SMALLEST_BLOCK_SIZE)) {
            blockSplit(block, size);
            removeBlock(block);
            return (void*)((char*)block + MEMBLOCK_HEADER_SIZE);
        // if block size is large enough to fit the request size
        } else if (block->size > size) {
            removeBlock(block);
            return (void*)((char*)block + MEMBLOCK_HEADER_SIZE);
        }
        block = block->next;
    }
    // if no suitable block found, request more memory from the system, split and return block
    mapMoreMemory();
    block = freelist;
    blockSplit(block, size);
    removeBlock(block);
    return (void*)((char*)block + MEMBLOCK_HEADER_SIZE);
}

// deallocate memory back into the free list
void yfree(void* ptr) {
    
}

// print the current free list details
void yprintfl() {
    struct blockHeader* block = freelist;
    int i = 1; // block number

    while (block) {
        printf("Free list block (%i) has a data size of (%zu) bytes\n", i, block->size);
        block = block->next;
        i++;
    }
}