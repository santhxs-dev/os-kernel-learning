#include "heap.h"
#include "kernel.h"
#include "status.h"
#include "memory/memory.h"
#include <stdbool.h>

static int heap_validate_table(void* ptr, void* end, struct heap_table* table)
{
    int res = 0;

    size_t table_size = (size_t)(end - ptr);                    // 0x01000000 - 0x07300000
    size_t total_blocks = table_size / PEACHOS_HEAP_BLOCK_SIZE; // (0x01000000 - 0x07300000) / 4096
    if (table->total != total_blocks)
    {
        res = -EINVARG;
        goto out;
    }

out:
    return res;
}

static bool heap_validate_alignment(void* ptr)
{
    return ((unsigned int)ptr % PEACHOS_HEAP_BLOCK_SIZE) == 0;
}

int heap_create(struct heap* heap, void* ptr, void* end, struct heap_table* table)
{
    int res = 0;

    if (!heap_validate_alignment(ptr) || !heap_validate_alignment(end))
    {
        res = -EINVARG;
        goto out;
    }

    memset(heap, 0, sizeof(struct heap));
    heap->saddr = ptr;
    heap->table = table;

    res = heap_validate_table(ptr, end, table);
    if (res < 0)
    {
        goto out;
    }

    size_t table_size = sizeof(HEAP_BLOCK_TABLE_ENTRY) * table->total;
    memset(table->entries, HEAP_BLOCK_TABLE_ENTRY_FREE, table_size);

out:
    return res;
}

static uint32_t heap_align_value_to_upper(uint32_t val)
{
    // e.g. with 5000
    if ((val % PEACHOS_HEAP_BLOCK_SIZE) == 0) // false
    {
        return val;
    }

    val = (val - ( val % PEACHOS_HEAP_BLOCK_SIZE)); // 5000 - (5000 - 4096)
                                                    // 5000 - 904 = 4096
    val += PEACHOS_HEAP_BLOCK_SIZE;                 // = 8192
    return val;
}

static int heap_get_entry_type(HEAP_BLOCK_TABLE_ENTRY entry)
{
    return entry & 0x0f; // Give us the first four bits
                // 1111
}

int heap_get_start_block(struct heap* heap, uint32_t total_blocks)
{
    struct heap_table* table = heap->table;
    int bc = 0;  // Block count - number of consecutive free blocks found
    int bs = -1; // Block start - index of the free starting block found

    for (size_t i = 0; i < table->total; i++)
    {                                              // 0x00
        if (heap_get_entry_type(table->entries[i]) != HEAP_BLOCK_TABLE_ENTRY_FREE)
        {
            bc = 0; // Reset: not a free block
            bs = -1; // Reset: we haven't found it yet
            continue;
        }

        // If this is the first block
        if (bs == -1)
        {
            bs = i; // Saves the starting position
        }
        bc++; // Count how many consecutive free blocks we found

        // If we find enough free blocks
        if (bc == total_blocks)
        {
            break;
        }
    }

    // If we don't find any free blocks enough
    if (bs == -1)
    {
        return -ENOMEM;
    }
    
    // Returns the index of the first free block found
    return bs;

}

void* heap_block_to_address(struct heap* heap, int block)
{  
                        // 8192
    return heap->saddr + (block * PEACHOS_HEAP_BLOCK_SIZE) ;
}

void heap_mark_blocks_taken(struct heap* heap, int start_block, int total_blocks)
{
    // Calculate the last block of the allocation.
    // Example: start_block = 0, total_blocks = 4 -> end_block = 3 (blocks 0,1,2,3).
    int end_block = (start_block + total_blocks) - 1;

    // Set the initial entry with the TAKEN flag and mark it as the FIRST block of the allocation.
    HEAP_BLOCK_TABLE_ENTRY entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN | HEAP_BLOCK_IS_FIRST;

    // If the allocation spans more than one block, add the HAS_NEXT flag.
    if (total_blocks > 1)
    {
        entry |= HEAP_BLOCK_HAS_NEXT;
    }

    // Iterate over all blocks in the allocation range.
    for (int i = start_block; i <= end_block; i++)
    {
        // Mark the current block with the current entry value.
        heap->table->entries[i] = entry;

        // For the next iteration (subsequent blocks), set the entry to TAKEN only.
        // Only the first block has the IS_FIRST flag.
        entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN;

        // If the current block is not the second to last block, set HAS_NEXT.
        // This ensures that all blocks except the last have HAS_NEXT.
        if (i != end_block - 1)
        {
            entry |= HEAP_BLOCK_HAS_NEXT;
        }
    }
}


void* heap_malloc_blocks(struct heap* heap, uint32_t total_blocks)
{
    void* address = 0;

    int start_block = heap_get_start_block(heap, total_blocks);
    if (start_block < 0)
    {
        goto out;
    }

    address = heap_block_to_address(heap, start_block);

    // Mark the blocks as taken
    heap_mark_blocks_taken(heap, start_block, total_blocks);

out:
    return address;
}

void heap_mark_blocks_free(struct heap* heap, int starting_block)
{
    struct heap_table* table = heap->table;
    for (int i = starting_block; i < (int)table->total; i++)
    {
        HEAP_BLOCK_TABLE_ENTRY entry = table->entries[i];
        table->entries[i] = HEAP_BLOCK_TABLE_ENTRY_FREE;

        // If doesn't has the next bit set
        if (!(entry & HEAP_BLOCK_HAS_NEXT))
        {
            break;
        }
    }
}

int heap_address_to_block(struct heap* heap, void* address)
{
    return ((int)(address - heap->saddr)) / PEACHOS_HEAP_BLOCK_SIZE;
    //          (0x1001000 - 0x1000000)   / 4096
    //          (16781312  -  16777216)   / 4096
    //          = 4096 / 4096
    //          = 1
}

void* heap_malloc(struct heap* heap, size_t size)
{
    // e.g. with 5000 
    size_t aligned_size = heap_align_value_to_upper(size);          // 8192
    uint32_t total_blocks = aligned_size / PEACHOS_HEAP_BLOCK_SIZE; // 2
    return heap_malloc_blocks(heap, total_blocks);
}

void heap_free(struct heap* heap, void* ptr)
{
    heap_mark_blocks_free(heap, heap_address_to_block(heap, ptr));
}