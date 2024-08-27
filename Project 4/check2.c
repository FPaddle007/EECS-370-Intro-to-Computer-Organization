/*
 * EECS 370, University of Michigan
 * Project 4: LC-2K Cache Simulator
 * Instructions are found in the project spec.
 */
#include <stdio.h>
#include <math.h>
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
int h, m, wb, db = 0;
int bOffset, setIndex, totalSize, tagBit, totalBlock = 0;
int hitIdx = -1;
void printAction(int, int, enum actionType);
void printCache();
int bitExtracted1(int, int, int);
int isTagInSet(int, int);
int isSetFull(int);
/*
 * Set up the cache with given command line parameters. This is
 * called once in main(). You must implement this function.
 */
void cache_init(int blockSize, int numSets, int blocksPerSet)
{
    printf("Simulating a cache with %d total lines; each line has %d words\n",
           numSets, blockSize);
    cache.blockSize = blockSize;
    cache.numSets = numSets;
    cache.blocksPerSet = blocksPerSet;
    bOffset = log2(blockSize);
    setIndex = log2(numSets);
    totalSize = blockSize * numSets * blocksPerSet;
    totalBlock = numSets * blocksPerSet;
    tagBit = 32 - bOffset - setIndex;
    printf("Each set in the cache contains %d lines; there are %d sets\n",
           blocksPerSet, numSets);
    for (int i = 0; i < numSets; ++i)
    {
        for (int j = 0; j < blocksPerSet; ++j)
        {
            cache.blocks[i * blocksPerSet + j].set = i;
            cache.blocks[i * blocksPerSet + j].dirty = 0;
            cache.blocks[i * blocksPerSet + j].lruLabel = 0;
            cache.blocks[i * blocksPerSet + j].tag = -1;
            cache.blocks[i * blocksPerSet + j].valid = 0;
        }
    }
}
/*
 * Fully associative = set-associative with 1 set
 * Direct-mapped = set-associative with #sets = #blocks
 */
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
int cache_access(int addr, int write_flag, int write_data)
{
    // TODO: TEST THE SELF-MODIFYING CASE AND WRITE SOME TEST CASES.
    int set = bitExtracted1(addr, setIndex, bOffset + 1);
    int tag = bitExtracted1(addr, tagBit, bOffset + setIndex + 1);
    // printf("%d\n", set);
    if (bOffset == 0 && setIndex == 0)
    {
        tag = addr;
    }
    int off = bitExtracted1(addr, bOffset, 1);
    // printf("%d\n", addr);
    // printf("%d\n", tag);
    if (isTagInSet(tag, set))
    {
        // printf("HIT\n");
        h++;
        for (int i = 0; i < totalBlock; ++i)
        {
            if (cache.blocks[i].set == set && cache.blocks[hitIdx].lruLabel >
                                                  cache.blocks[i].lruLabel)
            {
                cache.blocks[i].lruLabel++;
            }
        }
        cache.blocks[hitIdx].lruLabel = 0;
        if (write_flag == 0)
        {
            printAction(addr, 1, cacheToProcessor);
            return cache.blocks[hitIdx].data[off];
        }
        else
        {
            cache.blocks[hitIdx].data[off] = write_data;
            cache.blocks[hitIdx].dirty = 1;
            db++;
            printAction(addr, 1, processorToCache);
        }
    }
    else if (!isTagInSet(tag, set) && write_flag == 0)
    {
        // printf("RAN LW\n");
        // printCache();
        m++;
        int idx = -1;
        if (!isSetFull(set))
        {
            // for (int i = 0; i < totalBlock; ++i)
            // {
            // printf("%d, ", cache.blocks[i].lruLabel);
            // }
            // printf("\n");
            // find lru
            for (int i = 0; i < totalBlock; ++i)
            {
                if (cache.blocks[i].set == set && cache.blocks[i].valid == 0)
                {
                    cache.blocks[i].tag = tag;
                    cache.blocks[i].valid = 1;
                    idx = i;
                    break;
                }
            }
            for (int i = 0; i < totalBlock; ++i)
            {
                if (cache.blocks[i].set == set)
                {
                    cache.blocks[i].lruLabel++;
                }
            }
            cache.blocks[idx].lruLabel = 0;
            int a = 0;
            a <<= tagBit;
            a |= cache.blocks[idx].tag;
            a <<= setIndex;
            a |= set;
            a <<= bOffset;
            a |= 0;
            printAction(a, cache.blockSize, memoryToCache);
            printAction(addr, 1, cacheToProcessor);
            for (int i = 0; i < cache.blockSize; ++i)
            {
                cache.blocks[idx].data[i] = mem_access(a, 0, -1);
                a += 1;
            }
        }
        else
        {
            int maxUse = -1;
            // find lru
            // for (int i = 0; i < totalBlock; ++i)
            // {
            // printf("%d, ", cache.blocks[i].lruLabel);
            // }
            // printf("\n");
            for (int i = 0; i < totalBlock; ++i)
            {
                if (cache.blocks[i].set == set && cache.blocks[i].lruLabel >
                                                      maxUse)
                {
                    maxUse = cache.blocks[i].lruLabel;
                    idx = i;
                }
            }
            // update lru
            for (int i = 0; i < totalBlock; ++i)
            {
                if (cache.blocks[i].set == set)
                {
                    cache.blocks[i].lruLabel++;
                }
            }
            cache.blocks[idx].lruLabel = 0;
            int a = 0;
            a <<= tagBit;
            a |= cache.blocks[idx].tag;
            a <<= setIndex;
            a |= set;
            a <<= bOffset;
            a |= 0;
            // TODO: FIX cache to nowhere
            if (cache.blocks[idx].dirty == 1)
            {
                cache.blocks[idx].dirty = 0;
                printAction(a, cache.blockSize, cacheToMemory);
                for (int i = 0; i < cache.blockSize; ++i)
                {
                    mem_access(a, 1, cache.blocks[idx].data[i]);
                    a += 1;
                }
                wb++;
                db--;
            }
            else
            {
                printAction(a, cache.blockSize, cacheToNowhere);
            }
            a = 0;
            a <<= tagBit;
            a |= tag;
            a <<= setIndex;
            a |= set;
            a <<= bOffset;
            a |= 0;
            printAction(a, cache.blockSize, memoryToCache);
            printAction(addr, 1, cacheToProcessor);
            for (int i = 0; i < cache.blockSize; ++i)
            {
                cache.blocks[idx].data[i] = mem_access(a, 0, -1);
                cache.blocks[idx].tag = tag;
                a += 1;
            }
        }
        return cache.blocks[idx].data[off];
    }
    else if (!isTagInSet(tag, set) && write_flag == 1)
    {
        // printf("RAN SW\n");
        m++;
        if (!isSetFull(set))
        {
            int idx = 0;
            for (int i = 0; i < totalBlock; ++i)
            {
                if (cache.blocks[i].set == set && cache.blocks[i].valid == 0)
                {
                    cache.blocks[i].tag = tag;
                    cache.blocks[i].valid = 1;
                    cache.blocks[i].dirty = 1;
                    db++;
                    idx = i;
                    break;
                }
            }
            for (int i = 0; i < totalBlock; ++i)
            {
                if (cache.blocks[i].set == set)
                {
                    cache.blocks[i].lruLabel++;
                }
            }
            cache.blocks[idx].lruLabel = 0;
            int a = 0;
            a <<= tagBit;
            a |= cache.blocks[idx].tag;
            a <<= setIndex;
            a |= set;
            a <<= bOffset;
            a |= 0;
            printAction(a, cache.blockSize, memoryToCache);
            printAction(addr, 1, processorToCache);
            for (int i = 0; i < cache.blockSize; ++i)
            {
                cache.blocks[idx].data[i] = mem_access(a, 0, -1);
                a += 1;
            }
            cache.blocks[idx].data[off] = write_data;
        }
        else
        {
            int idx = -1;
            int maxUse = -1;
            for (int i = 0; i < totalBlock; ++i)
            {
                if (cache.blocks[i].set == set && cache.blocks[i].lruLabel >
                                                      maxUse)
                {
                    maxUse = cache.blocks[i].lruLabel;
                    idx = i;
                }
            }
            for (int i = 0; i < totalBlock; ++i)
            {
                if (cache.blocks[i].set == set)
                {
                    cache.blocks[i].lruLabel++;
                }
            }
            cache.blocks[idx].lruLabel = 0;
            int a = 0;
            a <<= tagBit;
            a |= cache.blocks[idx].tag;
            a <<= setIndex;
            a |= set;
            a <<= bOffset;
            a |= 0;
            if (cache.blocks[idx].dirty == 1)
            {
                cache.blocks[idx].dirty = 0;
                printAction(a, cache.blockSize, cacheToMemory);
                for (int i = 0; i < cache.blockSize; ++i)
                {
                    mem_access(a, 1, cache.blocks[idx].data[i]);
                    a += 1;
                }
                wb++;
                db--;
            }
            else
            {
                printAction(a, cache.blockSize, cacheToNowhere);
            }
            a = 0;
            a <<= tagBit;
            a |= tag;
            a <<= setIndex;
            a |= set;
            a <<= bOffset;
            a |= 0;
            printAction(a, cache.blockSize, memoryToCache);
            printAction(addr, 1, processorToCache);
            for (int i = 0; i < cache.blockSize; ++i)
            {
                cache.blocks[idx].data[i] = mem_access(a, 0, -1);
                cache.blocks[idx].tag = tag;
                a += 1;
            }
            cache.blocks[idx].data[off] = write_data;
            cache.blocks[idx].dirty = 1;
            db++;
        }
    }
    return -1;
}

int isTagInSet(int tag, int set)
{
    for (int i = 0; i < totalBlock; ++i)
    {
        if (cache.blocks[i].set == set && cache.blocks[i].tag == tag &&
            cache.blocks[i].valid == 1)
        {
            hitIdx = i;
            return 1;
        }
    }
    return 0;
}

int isSetFull(int set)
{
    for (int i = 0; i < totalBlock; ++i)
    {
        if (cache.blocks[i].set == set && cache.blocks[i].valid == 0)
        {
            return 0;
        }
    }
    return 1;
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
    printf("hits %d, misses %d, writebacks %d\n", h, m, wb);
    printf("%d dirty cache blocks left\n", db);
}
/*
 * Log the specifics of each cache action.
 *
 * address is the starting word address of the range of data being transferred.
 * size is the size of the range of data being transferred.
 * type specifies the source and destination of the data being transferred.
 * - cacheToProcessor: reading data from the cache to the processor
 * - processorToCache: writing data from the processor to the cache
 * - memoryToCache: reading data from the memory to the cache
 * - cacheToMemory: evicting cache data and writing it to the memory
 * - cacheToNowhere: evicting cache data and throwing it away
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
                printf(" %i", cache.blocks[set * cache.blocksPerSet +
                                           block]
                                  .data[index]);
            }
            printf(" }\n");
        }
    }
    printf("end cache\n");
}
int bitExtracted1(int number, int k, int p)
{
    return (((1 << k) - 1) & (number >> (p - 1)));
}