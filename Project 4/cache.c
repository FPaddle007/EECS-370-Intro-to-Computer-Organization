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

int start;
int fin;

void printAction(int, int, enum actionType);
void printCache();

// Useful (global) functions to help me find information
// about the cache
int tag_in_set(int, int);
int full_set(int);
int tagFinder(int);
int setFinder(int);
int offsetFinder(int);

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
    int index;
    int i = 0;
    int max = 0;
    int addrNew = ((int)(addr / cache.blockSize) * cache.blockSize);

    // initalize set, tag, and offset
    tag = tagFinder(addr);
    set = setFinder(addr);
    offset = offsetFinder(addr);

    // for loop start and end conditions
    start = cache.blocksPerSet * set;
    fin = cache.blocksPerSet * (set + 1);

    // if tag is already in the cache, issa hit
    if (tag_in_set(tag, set))
    {
        hits++;
        // update lru values
        for (int p = start; p < fin; p++)
        {
            if (cache.blocks[p].set == set && cache.blocks[hit_index].lruLabel > cache.blocks[p].lruLabel)
            {
                if (cache.blocks[hit_index].lruLabel > cache.blocks[p].lruLabel)
                {
                    cache.blocks[p].lruLabel++;
                }
            }
        }
        // set new block's lru to 0
        cache.blocks[hit_index].lruLabel = 0;
        // hit in load
        if (write_flag == 0)
        {
            printAction(addr, 1, cacheToProcessor);
            return cache.blocks[hit_index].data[offset];
        }
        // if hit in sw, update dirty bit and write data to cache
        else
        {
            cache.blocks[hit_index].data[offset] = write_data;
            cache.blocks[hit_index].dirty = 1;
            dirty_bit++;
            printAction(addr, 1, processorToCache);
            return cache.blocks[hit_index].data[offset];
        }
    }
    // check load conditions
    // if tag cannot be found in cache, update misses
    else if (!tag_in_set(tag, set) && write_flag == 0)
    {
        if (write_flag == 0)
        {
            misses++;
            index = -1; // keeps track of lru
            // looking to see if cache is full (which it isn't)
            if (!full_set(set))
            {
                // if set is not full, bring in cache block
                for (int p = start; p < fin; p++)
                {
                    if (cache.blocks[p].set == set && cache.blocks[p].valid == 0)
                    {
                        if (cache.blocks[p].valid == 0)
                        {
                            cache.blocks[p].tag = tag;
                            cache.blocks[p].valid = 1;
                            cache.blocks[p].dirty = 0;
                            index = p;
                            break;
                        }
                    }
                }
                // update lru values
                for (int p = start; p < fin; p++)
                {
                    if (cache.blocks[p].set == set)
                    {
                        cache.blocks[p].lruLabel++;
                    }
                }
                // set new blocks lru to 0
                cache.blocks[index].lruLabel = 0;

                // find address of empty spot by shifting bits
                i = 0;
                i = i << bit_tag;
                i = i | cache.blocks[index].tag;
                i = i << set_index;
                i = i | set;
                i = i << block_offset;
                i = i | 0;

                printAction(addrNew, cache.blockSize, memoryToCache);
                printAction(addr, 1, cacheToProcessor);

                for (int p = 0; p < cache.blockSize; p++)
                {
                    cache.blocks[index].data[p] = mem_access(addrNew + p, 0, -1);
                    i++;
                }
            }
            // if set is in fact full, evict lru
            else
            {
                max = -1; // will keep track of lru
                for (int p = start; p < fin; p++)
                {
                    if (cache.blocks[p].set == set && cache.blocks[p].lruLabel > max)
                    {
                        max = cache.blocks[p].lruLabel; // found lru
                        index = p;
                    }
                }
                // uplate lru
                for (int p = start; p < fin; p++)
                {
                    if (cache.blocks[p].set == set)
                    {
                        cache.blocks[p].lruLabel++;
                    }
                }
                // set new cache block's lru to 0
                cache.blocks[index].lruLabel = 0;

                // find address of evicted spot by shifting bits
                i = 0;
                i = i << bit_tag;
                i = i | cache.blocks[index].tag;
                i = i << set_index;
                i = i | set;
                i = i << block_offset;
                i = i | 0;
                // write back to memory
                if (cache.blocks[index].dirty == 1)
                {
                    cache.blocks[index].dirty = 0;
                    printAction(i, cache.blockSize, cacheToMemory);

                    for (int p = 0; p < cache.blockSize; p++)
                    {
                        mem_access(i, 1, cache.blocks[index].data[p]);
                        i++;
                    }
                    dirty_bit--;
                    write_backs++;
                }
                else
                {
                    // evict without writing back to mem
                    printAction(i, cache.blockSize, cacheToNowhere);
                }

                // find address of evicted spot by shifting bits
                i = 0;
                i = i << bit_tag;
                i = i | cache.blocks[index].tag;
                i = i << set_index;
                i = i | set;
                i = i << block_offset;
                i = i | 0;
                printAction(addrNew, cache.blockSize, memoryToCache);
                printAction(addr, 1, cacheToProcessor);

                for (int p = 0; p < cache.blockSize; p++)
                {
                    cache.blocks[index].data[p] = mem_access(addrNew + p, 0, -1);
                    cache.blocks[index].tag = tag;
                    i++;
                }
            }
        }
        return cache.blocks[index].data[offset];
    }

    // check store conditions
    else if (!tag_in_set(tag, set) && write_flag == 1)
    {
        // since tag is not in set, issa miss
        misses++;
        if (!full_set(set))
        {
            index = 0;
            // intialize the new block that is open
            for (int p = start; p < fin; p++)
            {
                if (cache.blocks[p].set == set)
                {
                    if (cache.blocks[p].valid == 0)
                    {
                        cache.blocks[p].tag = tag;
                        cache.blocks[p].valid = 1;
                        cache.blocks[p].dirty = 1;
                        dirty_bit++;
                        index = p;
                        break;
                    }
                }
            }
            // update lru values
            for (int p = start; p < fin; p++)
            {
                if (cache.blocks[p].set == set)
                {
                    cache.blocks[p].lruLabel++;
                }
            }
            // set new accessed cache block's lru to 0
            cache.blocks[index].lruLabel = 0;

            // find address of open spot by shifting bits
            i = 0;
            i = i << bit_tag;
            i = i | cache.blocks[index].tag;
            i = i << set_index;
            i = i | set;
            i = i << block_offset;
            i = i | 0;

            printAction(addrNew, cache.blockSize, memoryToCache);
            printAction(addr, 1, processorToCache);

            for (int p = 0; p < cache.blockSize; p++)
            {
                cache.blocks[index].data[p] = mem_access(addrNew + p, 0, -1);
                i++;
            }
            // write data back to memory
            cache.blocks[index].data[offset] = write_data;
        }
        // if set is full, evict lru
        else
        {
            index = -1;
            max = -1;

            // have to find lru to evict
            for (int p = start; p < fin; p++)
            {
                if (cache.blocks[p].set == set && cache.blocks[p].lruLabel > max)
                {
                    if (cache.blocks[p].lruLabel > max)
                    {
                        max = cache.blocks[p].lruLabel; // found lru
                        index = p;
                    }
                }
            }
            // updating lru
            for (int p = start; p < fin; p++)
            {
                if (cache.blocks[p].set == set)
                {
                    cache.blocks[p].lruLabel++;
                }
            }
            // set the lru of the cache block we just brought in to 0
            cache.blocks[index].lruLabel = 0;

            // find address of evicted cache block by shifting bits
            i = 0;
            i = i << bit_tag;
            i = i | cache.blocks[index].tag;
            i = i << set_index;
            i = i | set;
            i = i << block_offset;
            i = i | 0;
            // write back to memory
            if (cache.blocks[index].dirty == 1)
            {
                cache.blocks[index].dirty = 0;
                printAction(i, cache.blockSize, cacheToMemory);

                for (int p = 0; p < cache.blockSize; p++)
                {
                    mem_access(i, 1, cache.blocks[index].data[p]);
                    i++;
                }
                dirty_bit--;
                write_backs++;
            }
            else
            {
                // otherwise, evict without writing back to memory
                printAction(i, cache.blockSize, cacheToNowhere);
            }
            // find address of evicted spot by shifting bits
            i = 0;
            i = i << bit_tag;
            i = i | tag;
            i = i << set_index;
            i = i | set;
            i = i << block_offset;
            i = i | 0;

            printAction(addrNew, cache.blockSize, memoryToCache);
            printAction(addr, 1, processorToCache);

            //
            for (int p = 0; p < cache.blockSize; p++)
            {
                cache.blocks[index].data[p] = mem_access(addrNew + p, 0, -1);
                cache.blocks[index].tag = tag;
                i++;
            }
            // write back to memory
            cache.blocks[index].data[offset] = write_data;
            cache.blocks[index].dirty = 1;
            dirty_bit++;
        }
    }
    return -1;
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
    printf("%s", "$$$ Main memory words accessed: ");
    printf("%d\n", get_num_mem_accesses());
    printf("%s\n", "End of run statistics:");
    printf("hits %d, misses %d, writebacks %d\n", hits, misses, write_backs);
    printf("%d dirty cache blocks left\n", dirty_bit);
}

// gets tag
int tagFinder(int addr)
{
    int tag = addr;
    int shifter = (int)log2((double)cache.blockSize);
    shifter = shifter + (int)log2((double)cache.numSets);
    tag = addr >> shifter;
    return tag;
}
// gets set
int setFinder(int addr)
{
    int set = addr;
    int hide = (int)log2((double)cache.numSets);
    int shifter = (int)log2((double)cache.blockSize);
    set = addr >> shifter;
    set = set & (int)(pow((double)2, (double)hide) - 1);
    return set;
}
// gets offset
int offsetFinder(int addr)
{
    int offset = addr;
    int hide = (int)log2((double)cache.blockSize);
    offset = offset & (int)(pow((double)2, (double)hide) - 1);
    return offset;
}
// finds if tag is already in set
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
        if (cache.blocks[p].set == set && cache.blocks[p].valid == 0)
        {
            return 0;
        }
    }
    return 1;
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