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

// Useful (global) functions to help me find information
// about the cache
int extractedBit(int, int, int);
int tag_in_set(int, int);
int full_set(int);

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

void printAction(int, int, enum actionType);
void printCache();

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
    bit_tag = 32 - block_offset - set_index;

    // print statement
    printf("Each set in the cache contains %d lines; there are %d sets\n", blocksPerSet, numSets);

    // find information about cache
    // calculate set index, dirty bit, lru, tag, & valid bit
    for (int r = 0; r < numSets; r++)
    {
        for (int c = 0; c < blocksPerSet; c++)
        {
            cache.blocks[r * blocksPerSet + c].set = r;
            cache.blocks[r * blocksPerSet + c].dirty = 0;
            cache.blocks[r * blocksPerSet + c].lruLabel = 0;
            cache.blocks[r * blocksPerSet + c].tag = -1;
            cache.blocks[r * blocksPerSet + c].valid = 0;
        }
    }
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
    int index;
    int i = 0;
    int max = -1;

    // initalize set, tag, and offset
    tag = extractedBit(addr, bit_tag, block_offset + set_index);
    set = extractedBit(addr, set_index, block_offset);
    offset = extractedBit(addr, block_offset, 0);

    // if tag is already in the cache, update hits
    if (!write_flag)
    {
        if (tag_in_set(tag, set))
        {
            // issa hit
            printAction(addr, 1, cacheToProcessor);
            // update lru
            for (int p = 0; p < total_num_of_blocks; p++)
            {
                if (cache.blocks[p].set == set)
                {
                    if (cache.blocks[hit_index].lruLabel > cache.blocks[p].lruLabel)
                    {
                        cache.blocks[p].lruLabel++;
                    }
                }
            }
            cache.blocks[hit_index].lruLabel = 0;
            return cache.blocks[tag_in_set(tag, set)].data[offset];
        }
        // check if opening in cache
        int openSpot = 0;
        if (!tag_in_set(tag, set))
        {
            for (int p = cache.blocksPerSet * set; p < cache.blocksPerSet * (set + 1); p++)
            {
                if (!cache.blocks[p].valid)
                {
                    openSpot = p;
                }
            }
        }
        if (openSpot != 1)
        {
            int addrNew = ((int)(addr / cache.blockSize)) * cache.blockSize;
            printAction(addrNew, cache.blockSize, memoryToCache);
            int i = 0;
            // make and initialize new blocks
            cache.blocks[full_set(set)].tag = tag;
            cache.blocks[full_set(set)].valid = 1;
            cache.blocks[full_set(set)].set = set;
            cache.blocks[full_set(set)].dirty = 0;

            for (int p = addrNew; i < addrNew + cache.blockSize; p++)
            {
                cache.blocks[full_set(set)].data[i] = mem_access(p, 0, 0);
                i++;
            }
            // update lru
            for (int p = 0; p < total_num_of_blocks; p++)
            {
                if (cache.blocks[p].set == set)
                {
                    if (cache.blocks[hit_index].lruLabel > cache.blocks[p].lruLabel)
                    {
                        cache.blocks[p].lruLabel++;
                    }
                }
            }
            cache.blocks[hit_index].lruLabel = 0;
            printAction(addr, 1, cacheToProcessor);
            return cache.blocks[tag_in_set(tag, set)].data[offset];
        }

        // otherwise, evict the block
        else
        {
            for (int p = cache.blocksPerSet * set; p < cache.blocksPerSet * (set + 1); p++)
            {
                if (cache.blocks[p].lruLabel >= cache.blocks[cache.blocksPerSet * set].lruLabel)
                {
                    index = p;
                    max = cache.blocks[p].lruLabel;
                }
            }
            // get address
            int shiftOffset = (int)log2((double)cache.blockSize);
            int shiftSet = (int)log2((double)cache.numSets);
            int calculatedShift = shiftSet + shiftOffset;
            // not sure if max or index to pass in
            int tracker = cache.blocks[max].tag;
            int addrNew = ((int)(addr / cache.blockSize)) * cache.blockSize;
            // write dirty bit to memory
            if (cache.blocks[max].dirty)
            {
                printAction(tracker, cache.blockSize, cacheToMemory);
                int i = 0;
                for (int p = tracker; p < tracker + cache.blockSize; p++)
                {
                    mem_access(p, 1, cache.blocks[max].data[i]);
                    i++;
                }
            }
            else
            {
                // if not changed, evict without writing it to memory
                printAction(tracker, cache.blockSize, cacheToNowhere);
            }
            // now that it is evicted, replace spot with new block
            cache.blocks[full_set(set)].tag = tag;
            cache.blocks[full_set(set)].valid = 1;
            cache.blocks[full_set(set)].set = set;
            cache.blocks[full_set(set)].dirty = 1;
            int i = 0;
            for (int p = addrNew; p < addrNew + cache.blockSize; p++)
            {
                cache.blocks[max].data[i] = mem_access(p, 0, 0);
                i++;
            }
            // write the data
            cache.blocks[max].data[offset] = write_data;
            cache.blocks[max].dirty = 1;
            // update lru
            for (int p = 0; p < total_num_of_blocks; p++)
            {
                if (cache.blocks[p].set == set)
                {
                    if (cache.blocks[hit_index].lruLabel > cache.blocks[p].lruLabel)
                    {
                        cache.blocks[p].lruLabel++;
                    }
                }
            }
            cache.blocks[hit_index].lruLabel = 0;
            printAction(addr, 1, processorToCache);
        }
    }
    
    return 0;
}

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

// finds if tag is in set, and if so, increment cache hit
// if not cache miss
int tag_in_set(int tag, int set)
{

    for (int p = cache.blocksPerSet * set; p < cache.blocksPerSet * (set + 1); p++)
    {
        if (cache.blocks[p].set == set && cache.blocks[p].tag == tag && cache.blocks[p].valid == 1)
        {
            // issa hit
            hit_index = p;
            return 1;
        }
    }
    // issa miss
    return 0;
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