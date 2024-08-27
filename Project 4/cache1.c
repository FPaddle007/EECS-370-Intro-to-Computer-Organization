/*
 * EECS 370, University of Michigan
 * Project 4: LC-2K Cache Simulator
 * Instructions are found in the project spec.
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_CACHE_SIZE 256
#define MAX_BLOCK_SIZE 256

extern int mem_access(int addr, int write_flag, int write_data);
extern int get_num_mem_accesses();

enum actionType
{
    cacheToProcessor,
    processorToCache,
    memoryToCache,
    cacheToMemory,
    cacheToNowhere
};

typedef struct blockStruct
{
    int data[MAX_BLOCK_SIZE];
    int dirty;
    int lruLabel;
    int set;
    int tag;
    int valid;
} blockStruct;

typedef struct cacheStruct
{
    blockStruct blocks[MAX_CACHE_SIZE];
    int blockSize;
    int numSets;
    int blocksPerSet;
} cacheStruct;

/* Global Cache variable */
cacheStruct cache;

int block_offset = 0;
int total_num_of_blocks = 0;
int bit_tag = 0;
int size = 0;
int set_index = 0;
int hit_index = -1;

int hits = 0;
int misses = 0;
int write_backs = 0;
int dirty_bit = 0;
int setTracker = 0;

void printAction(int, int, enum actionType);
void printCache();
// Useful (global) functions to help me find information
// about the cache
int extractedBit(int, int, int);
int tag_in_set(int, int);
int full_set(int);
int get_out(int);
void LRU_Updater(int, int);
int shifting(int);

/*
 * Set up the cache with given command line parameters. This is
 * called once in main(). You must implement this function.
 */
void cache_init(int blockSize, int numSets, int blocksPerSet)
{

    // print statement
    printf("Simulating a cache with %d total lines; each line has %d words\n", numSets, blockSize);

    // initalize the given parameters
    cache.blocksPerSet = blocksPerSet;
    cache.blockSize = blockSize;
    cache.numSets = numSets;

    // math to calculate cache information
    set_index = (int)log2((double)numSets);
    block_offset = (int)log2((double)blockSize);
    size = blockSize * numSets * blocksPerSet;
    total_num_of_blocks = numSets * blocksPerSet;
    bit_tag = 16 - block_offset - set_index;

    // print statement
    printf("Each set in the cache contains %d lines; there are %d sets\n", blocksPerSet, numSets);

    // find information about cache
    // calculate set index, dirty bit, lru, tag, & valid bit
    for (int i = 0; i < MAX_CACHE_SIZE; i++)
    {
        cache.blocks[i].valid = 0;
        cache.blocks[i].tag = 0;
        cache.blocks[i].set = 0;
        cache.blocks[i].dirty = 0;
        cache.blocks[i].lruLabel = -1;
        for (int j = 0; j < MAX_BLOCK_SIZE; j++)
        {
            cache.blocks[i].data[j] = 0;
        }
    }
    return;
}

/*
 * Access the cache. This is the main part of the project,
 * and should call printAction as is appropriate.
 * It should only call mem_access when absolutely necessary.
 * addr is a 16-bit LC2K word address.
 * write_flag is 0 for reads (fetch/lw) and 1 for writes (sw).
 * write_data is a word, and is only valid if write_flag is 1.
 * The return of mem_access is undefined if write_flag is 1.
 * Thus the return of cache_access is undefined if write_flag is 1.
 */

// Full assoc is set assoc with 1 set
// Direct assoc is set assoc with # of sets = # of blocks
int cache_access(int addr, int write_flag, int write_data)
{

    int tag;
    int set;
    int offset;
    int i = 0;

    // initalize set, tag, and offset
    tag = extractedBit(addr, bit_tag, block_offset + set_index);
    set = extractedBit(addr, set_index, block_offset);
    offset = extractedBit(addr, block_offset, 1);

    // if tag is already in the cache, update hits
    if (tag_in_set(tag, set))
    {
        hits++;
        // update lru values
        LRU_Updater(set, tag_in_set(tag, set));

        // if hit for loads
        if (write_flag == 0)
        {
            printAction(addr, 1, cacheToProcessor);
            return cache.blocks[hit_index].data[offset];
        }
        // if hit for sw, update dirty bit and write data to cache
        else
        {
            cache.blocks[hit_index].data[offset] = write_data;
            cache.blocks[hit_index].dirty = 1;
            dirty_bit++;
            printAction(addr, 1, processorToCache);
            // return cache.blocks[hit_index].data[offset];
        }
    }
    // for loads
    //  if tag cannot be found in cache, increment misses
    else if ((!tag_in_set(tag, set)) && write_flag == 0)
    {
        misses++;

        // Set is not full
        if (!full_set(set))
        {
            printAction(addr, cache.blockSize, memoryToCache);
            int opening = full_set(set);
            // if set is not full, bring in cache block
            cache.blocks[opening].tag = tag;
            cache.blocks[opening].valid = 1;
            cache.blocks[opening].set = set;
            cache.blocks[opening].dirty = 0;
            // i = addr gets me to 0
            // addr is what i want from mem
            for (int p = addr; p < addr + cache.blockSize; p++)
            {
                cache.blocks[opening].data[i - addr] = mem_access(i, 0, 0);
            }
            // update LRU
            LRU_Updater(set, opening);
            printAction(addr, 1, cacheToProcessor);
            return cache.blocks[opening].data[offset];
        }
        // if it's full, evict a block to bring in the new one
        else
        {
            int packedUp = get_out(set);
            int tracker = shifting(packedUp);
            // write dirty block to memory
            if (cache.blocks[packedUp].dirty)
            {
                printAction(tracker, cache.blockSize, cacheToMemory);
                for (int p = tracker; p < tracker + cache.blockSize; p++)
                {
                    mem_access(p, 1, cache.blocks[packedUp].data[i - tracker]);
                }
                write_backs++;
                dirty_bit--;
            }
            else
            {
                // evict without writing back data to memory
                printAction(tracker, cache.blockSize, cacheToNowhere);
            }
            // replace evicted block with new cache block
            cache.blocks[packedUp].tag = tag;
            cache.blocks[packedUp].valid = 1;
            cache.blocks[packedUp].set = set;
            cache.blocks[packedUp].dirty = 0;
            printAction(addr, cache.blockSize, memoryToCache);

            for (int p = addr; p < addr + cache.blockSize; p++)
            {
                cache.blocks[packedUp].data[i - addr] = mem_access(i, 0, 0);
            }

            LRU_Updater(set, packedUp);
            printAction(addr, 1, cacheToProcessor);
            return cache.blocks[packedUp].data[offset];
        }
    }
    // check stores
    else if ((!tag_in_set(tag, set)) && write_flag == 1)
    {
        // if issa miss, increment misses
        misses++;
        // set is not full (check for openings)
        if (!full_set(set))
        {
            int opening = get_out(set);
            printAction(addr, cache.blockSize, memoryToCache);

            cache.blocks[opening].tag = tag;
            cache.blocks[opening].valid = 1;
            cache.blocks[opening].set = set;
            cache.blocks[opening].dirty = 1;
            dirty_bit++;

            for (int p = addr; p < addr + cache.blockSize; p++)
            {
                cache.blocks[opening].data[i - addr] = mem_access(i, 0, 0);
            }
            // write data to memory
            cache.blocks[opening].data[offset] = write_data;
            cache.blocks[opening].dirty = 1;
            LRU_Updater(set, opening);
            printAction(addr, 1, processorToCache);
        }
        else
        {
            // evict lru (while updating memory if needed) and replace it

            int packedUp = get_out(set);
            int tracker = shifting(packedUp);

            if (cache.blocks[packedUp].dirty)
            {
                printAction(tracker, cache.blockSize, cacheToMemory);
                // write dirty block to memory
                for (int p = tracker; p < tracker + cache.blockSize; p++)
                {
                    mem_access(p, 1, cache.blocks[packedUp].data[i - tracker]);
                }
                write_backs++;
                dirty_bit++;
            }
            else
            {
                // evict without writing data back to memory
                printAction(i, cache.blockSize, cacheToNowhere);
            }
            // bring in new cache block
            printAction(addr, cache.blockSize, memoryToCache);
            cache.blocks[packedUp].tag = tag;
            cache.blocks[packedUp].valid = 1;
            cache.blocks[packedUp].set = set;
            cache.blocks[packedUp].dirty = 1;

            for (int p = addr; p < addr + cache.blockSize; p++)
            {
                cache.blocks[packedUp].data[i - addr] = mem_access(i, 0, 0);
            }
            // finally write back to memory
            cache.blocks[packedUp].data[offset] = write_data;
            cache.blocks[packedUp].dirty = 1;
            dirty_bit++;
            LRU_Updater(set, packedUp);
            printAction(addr, 1, processorToCache);
        }
    }
    return 0;
}

// return mem_access(addr, write_flag, write_data);

/*
 * print end of run statistics like in the spec. This is not required,
 * but is very helpful in debugging.
 * This should be called once a halt is reached.
 * DO NOT delete this function, or else it won't compile.
 * DO NOT print $$$ in this function
 */
void printStats()
{
    printf("%s", "Main memory words accessed: ");
    printf("%d\n", get_num_mem_accesses());
    printf("%s\n", "End of run statistics:");
    printf("hits %d, misses %d, writebacks %d\n", hits, misses, write_backs);
    printf("%d dirty cache blocks left\n", dirty_bit);
}

// function to help find the extracted bits
int extractedBit(int num, int i, int j)
{
    return (((1 << i) - 1) & (num >> (j)));
}

// finds if tag is in set, and if so, issa hit
// if not, cache miss
int tag_in_set(int tag, int set)
{

    for (int p = cache.blocksPerSet * set; p < cache.blocksPerSet * (set + 1); p++)
    {
        if (cache.blocks[p].set == set && cache.blocks[p].tag == tag && cache.blocks[p].valid == 1)
        {
            // issa hit
            hit_index = p;
            return hit_index;
        }
    }
    // issa miss
    return -1;
}

// evicts the lru
int get_out(int set)
{
    int max = cache.blocks[cache.blocksPerSet * set].lruLabel;
    int index = 0;
    for (int p = cache.blocksPerSet * set; p < cache.blocksPerSet * (set + 1); p++)
    {
        if (cache.blocks[p].lruLabel >= max)
        {
            index = p;
            max = cache.blocks[p].lruLabel;
        }
    }
    return index;
}

//  get address for me by shifting bits
int shifting(int block)
{
    int shift = block_offset + set_index;
    int tracker = cache.blocks[block].tag;
    int shift_set = cache.blocks[block].set;
    tracker = tracker << shift;
    if (cache.numSets > 1)
    {
        shift_set = shift_set << block_offset;
        tracker |= shift_set;
    }
    return tracker;
}

// function to help me see if the set is full
int full_set(int set)
{

    for (int p = cache.blocksPerSet * set; p < cache.blocksPerSet * (set + 1); p++)
    {
        if (cache.blocks[p].valid == 0)
        {
            return p;
        }
    }
    return -1;
}

// updates lru
void LRU_Updater(int set, int prev_blocks)
{
    for (int p = cache.blocksPerSet * set; p < cache.blocksPerSet * (set + 1); p++)
    {
        if (p != prev_blocks)
        {
            cache.blocks[p].lruLabel = cache.blocks[p].lruLabel + 1;
        }
        else
        {
            cache.blocks[p].lruLabel = 0;
        }
    }
}

/*
 * Log the specifics of each cache action.
 *
 * address is the starting word address of the range of data being transferred.
 * size is the size of the range of data being transferred.
 * type specifies the source and destination of the data being transferred.
 *  -    cacheToProcessor: reading data from the cache to the processor
 *  -    processorToCache: writing data from the processor to the cache
 *  -    memoryToCache: reading data from the memory to the cache
 *  -    cacheToMemory: evicting cache data and writing it to the memory
 *  -    cacheToNowhere: evicting cache data and throwing it away
 */
void printAction(int address, int size, enum actionType type)
{
    printf("$$$ transferring word [%d-%d] ", address, address + size - 1);

    if (type == cacheToProcessor)
    {
        printf("from the cache to the processor\n");
    }
    else if (type == processorToCache)
    {
        printf("from the processor to the cache\n");
    }
    else if (type == memoryToCache)
    {
        printf("from the memory to the cache\n");
    }
    else if (type == cacheToMemory)
    {
        printf("from the cache to the memory\n");
    }
    else if (type == cacheToNowhere)
    {
        printf("from the cache to nowhere\n");
    }
}

/*
 * Prints the cache based on the configurations of the struct
 * This is for debugging only and is not graded, so you may
 * modify it, but that is not recommended.
 */
void printCache()
{
    printf("\ncache:\n");
    for (int set = 0; set < cache.numSets; ++set)
    {
        printf("\tset %i:\n", set);
        for (int block = 0; block < cache.blocksPerSet; ++block)
        {
            printf("\t\t[ %i ]: {", block);
            for (int index = 0; index < cache.blockSize; ++index)
            {
                printf(" %i", cache.blocks[set * cache.blocksPerSet + block].data[index]);
            }
            printf(" }\n");
        }
    }
    printf("end cache\n");
}