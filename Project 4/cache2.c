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
void newBlock(int, int, int, int, int);

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
    for (int r = 0; r < MAX_CACHE_SIZE; r++)
    {
        if (r != 0)
        {
            if (((r) % cache.blocksPerSet) == 0)
            {
                setTracker++;
            }
        }
        cache.blocks[r].set = setTracker;
        cache.blocks[r].dirty = 0;
        cache.blocks[r].lruLabel = -1;
        cache.blocks[r].tag = -1;
        cache.blocks[r].valid = 0;
        for (int c = 0; c < MAX_BLOCK_SIZE; c++)
        {
            cache.blocks[r].data[c] = 0;
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

    // initalize set, tag, and offset
    tag = extractedBit(addr, bit_tag, block_offset + set_index);
    set = extractedBit(addr, set_index, block_offset);
    offset = extractedBit(addr, block_offset, 0);

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
            printAction(addr, 1, processorToCache);
            dirty_bit++;
        }
    }
    // check conditions for loads
    else if (!tag_in_set(tag, set) && write_flag == 0)
    {
        // if tag cannot be found in cache, update misses
        misses++;
        // check to see if there is an open space in cache to
        // bring in new block
        if (!full_set(set))
        {
            int opening = full_set(set);
            // initalize a new block
            newBlock(opening, tag, set, 0, 1);
            // update lru
            LRU_Updater(set, opening);
            // do shifting to get address of empty block
            int i = shifting(opening);
            printAction(i, cache.blockSize, memoryToCache);
            printAction(addr, 1, cacheToProcessor);
            for (int p = 0; p < cache.blockSize; p++)
            {
                cache.blocks[opening].data[p] = mem_access(i, 0, -1);
                i++;
            }
        }
        else
        {
            // if miss & full, evict lru
            int packedUp = get_out(set);
            // update lru
            LRU_Updater(set, packedUp);
            // do shifting to get address of kicked out block
            int i = shifting(packedUp);

            if (cache.blocks[packedUp].dirty == 1)
            {
                cache.blocks[packedUp].dirty = 0;
                printAction(i, cache.blockSize, cacheToMemory);

                for (int p = 0; p < cache.blockSize; p++)
                {
                    mem_access(i, 1, cache.blocks[packedUp].data[p]);
                    i++;
                }
                write_backs++;
                dirty_bit++;
            }
            else
            {
                printAction(i, cache.blockSize, cacheToNowhere);
            }
            i = shifting(packedUp);
            printAction(i, cache.blockSize, memoryToCache);
            printAction(addr, 1, cacheToProcessor);
            for (int p = 0; p < cache.blockSize; p++)
            {
                cache.blocks[packedUp].data[p] = mem_access(i, 0, -1);
                cache.blocks[packedUp].tag = tag;
                i++;
            }
        }
        // return cache.blocks[packedUp].data[offset];
    }
    // check store conditions
    else if (!tag_in_set(tag, set) && write_flag == 1)
    {
        // if tag is not in the set, issa miss
        misses++;
        if (!full_set(set))
        {
            // find if there are any open spots
            int opening = full_set(set);
            // initialize new block
            newBlock(opening, tag, set, 1, 1);
            dirty_bit++;
            // update lru
            LRU_Updater(set, opening);
            // find address by shifting
            int i = shifting(opening);
            printAction(i, cache.blockSize, memoryToCache);
            printAction(addr, 1, processorToCache);

            for (int p = 0; p < cache.blockSize; p++)
            {
                cache.blocks[opening].data[offset] = mem_access(i, 0, -1);
                i++;
            }
            cache.blocks[opening].data[offset] = write_data;
        }
        else{
            // if full, evict lru
            int packedUp = get_out(set);
            //update lru
            LRU_Updater(set, packedUp);
            // get address by doing shifting
            int i = shifting(packedUp);
            // set dirty bit back to 0 and write back to memory
            if(cache.blocks[packedUp].dirty == 1){
                cache.blocks[packedUp].dirty = 0;
                printAction(i, cache.blockSize, cacheToMemory);
                for(int p = 0; p < cache.blockSize; p++){
                    mem_access(i, 1, cache.blocks[packedUp].data[p]);
                    i++;
                }
                write_backs++;
                dirty_bit--;
            }
            else{
                printAction(i, cache.blockSize, cacheToNowhere);
            }
            i = shifting(packedUp);
            printAction(i, cache.blockSize, memoryToCache);
            printAction(addr, 1, processorToCache);

            for(int p = 0; p < cache.blockSize; p++){
                cache.blocks[packedUp].data[p] = mem_access(i, 0, -1);
                cache.blocks[packedUp].tag = tag;
                i++;
            }
            // write back to memory
            cache.blocks[packedUp].data[offset] = write_data;
            cache.blocks[packedUp].dirty = 1;
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
    return -1;
}

// function to help me see if the set is full
int full_set(int set)
{
    int start = cache.blocksPerSet * set;
    int fin = cache.blocksPerSet * (set + 1);
    for (int p = start; p < fin; p++)
    {
        if (cache.blocks[p].set == set && cache.blocks[p].valid == 0)
        {
            return 0;
        }
    }
    return 1;
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

void newBlock(int block, int tag, int set, int dirty, int valid)
{
    cache.blocks[block].tag = tag;
    cache.blocks[block].set = set;
    cache.blocks[block].dirty = dirty;
    cache.blocks[block].valid = valid;
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