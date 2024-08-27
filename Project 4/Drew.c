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
extern int get_num_mem_accesses(void);

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

void printAction(int, int, enum actionType);
void printCache();
int getTag(int addr);
int getSet(int addr);
int getOffs(int addr);
int hitMiss(int tag, int set);
void updateLRU(int set, int used);
int evict(int set);
int findOpening(int set);
int getAddr(int block);

/*
 * Set up the cache with given command line parameters. This is
 * called once in main(). You must implement this function.
 */
void cache_init(int blockSize, int numSets, int blocksPerSet)
{
    cache.blockSize = blockSize;
    cache.numSets = numSets;
    cache.blocksPerSet = blocksPerSet;

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

void block_init(int block, int dirty, int set, int tag, int valid)
{
    cache.blocks[block].tag = tag;
    cache.blocks[block].valid = valid;
    cache.blocks[block].set = set;
    cache.blocks[block].dirty = dirty;
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
int cache_access(int addr, int write_flag, int write_data)
{
    int tag = getTag(addr);
    int set = getSet(addr);
    int offs = getOffs(addr);

    // load or fetch
    if (!write_flag)
    {
        // check to see if it's already in the cache
        if (hitMiss(tag, set) != -1)
        {
            // HIT
            printAction(addr, 1, cacheToProcessor);
            updateLRU(set, hitMiss(tag, set));
            return cache.blocks[hitMiss(tag, set)].data[offs];

            // there is an opening in the cache
        }
        else if (findOpening(set) != -1)
        {
            int newAddr = ((int)(addr / cache.blockSize)) * cache.blockSize;
            printAction(newAddr, cache.blockSize, memoryToCache);
            int openBlock = findOpening(set);
            int j = 0;
            // Make a new block
            block_init(openBlock, 0, set, tag, 1);
            for (int i = newAddr; i < newAddr + cache.blockSize; i++)
            {
                cache.blocks[openBlock].data[j] = mem_access(i, 0, 0);
                j++;
            }
            updateLRU(set, openBlock);
            printAction(addr, 1, cacheToProcessor);
            return cache.blocks[openBlock].data[offs];

            // else evict a block
        }
        else
        {
            int evictedBlock = evict(set);
            int recon = getAddr(evictedBlock);
            int newAddr = ((int)(addr / cache.blockSize)) * cache.blockSize;
            // Write back dirty block to mem
            if (cache.blocks[evictedBlock].dirty)
            {
                printAction(recon, cache.blockSize, cacheToMemory);
                int j = 0;
                for (int i = recon; i < recon + cache.blockSize; i++)
                {
                    mem_access(i, 1, cache.blocks[evictedBlock].data[j]);
                    j++;
                }
            }
            else
            {
                // evict w/out writing back to mem
                printAction(recon, cache.blockSize, cacheToNowhere);
            }
            // now replace this spot in cache
            block_init(evictedBlock, 0, set, tag, 1);
            printAction(newAddr, cache.blockSize, memoryToCache);
            int j = 0;
            for (int i = newAddr; i < newAddr + cache.blockSize; i++)
            {
                cache.blocks[evictedBlock].data[j] = mem_access(i, 0, 0);
                j++;
            }
            updateLRU(set, evictedBlock);
            printAction(addr, 1, cacheToProcessor);
            return cache.blocks[evictedBlock].data[offs];
        }
    }

    // store
    else if (write_flag)
    {
        // check to see if it's in the cache
        if (hitMiss(tag, set) != -1)
        {
            // HIT
            cache.blocks[hitMiss(tag, set)].data[offs] = write_data;
            cache.blocks[hitMiss(tag, set)].dirty = 1;
            updateLRU(set, hitMiss(tag, set));
            printAction(addr, 1, processorToCache);
        }
        else if (findOpening(set) != -1)
        {
            // else check if there are any openings in the cache
            int openBlock = findOpening(set);
            // allocate
            int newAddr = ((int)(addr / cache.blockSize)) * cache.blockSize;
            printAction(newAddr, cache.blockSize, memoryToCache);

            block_init(openBlock, 1, set, tag, 1);
            int j = 0;
            for (int i = newAddr; i < newAddr + cache.blockSize; i++)
            {
                cache.blocks[openBlock].data[j] = mem_access(i, 0, 0);
                j++;
            }
            // write
            cache.blocks[openBlock].data[offs] = write_data;
            cache.blocks[openBlock].dirty = 1;
            updateLRU(set, openBlock);
            printAction(addr, 1, processorToCache);
        }
        else
        {
            // else evict LRU (updating mem if necessary) and replace
            int evictedBlock = evict(set);
            int recon = getAddr(evictedBlock);
            int newAddr = ((int)(addr / cache.blockSize)) * cache.blockSize;
            if (cache.blocks[evictedBlock].dirty)
            {
                printAction(recon, cache.blockSize, cacheToMemory);
                // write back dirty block to mem
                int j = 0;
                for (int i = recon; i < recon + cache.blockSize; i++)
                {
                    mem_access(i, 1, cache.blocks[evictedBlock].data[j]);
                    j++;
                }
            }
            else
            {
                printAction(recon, cache.blockSize, cacheToNowhere);
                // evict w/out writing back to mem
            }
            // now replace this spot in cache
            printAction(newAddr, cache.blockSize, memoryToCache);
            int j = 0;
            block_init(evictedBlock, 1, set, tag, 1);
            for (int i = newAddr; i < newAddr + cache.blockSize; i++)
            {
                cache.blocks[evictedBlock].data[j] = mem_access(i, 0, 0);
                j++;
            }
            // write
            cache.blocks[evictedBlock].data[offs] = write_data;
            cache.blocks[evictedBlock].dirty = 1;
            updateLRU(set, evictedBlock);
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
    return;
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

int getTag(int addr)
{
    int tag = addr;
    int toShift = (int)log2((double)cache.blockSize);
    toShift += (int)log2((double)cache.numSets);
    tag = addr >> toShift;
    return tag;
}

int getSet(int addr)
{
    int set = addr;
    int mask = (int)log2((double)cache.numSets);
    int toShift = (int)log2((double)cache.blockSize);
    set = addr >> toShift;
    set &= (int)(pow((double)2, (double)mask) - 1);
    return set;
}

int getOffs(int addr)
{
    int offs = addr;
    int mask = (int)log2((double)cache.blockSize);
    offs &= (int)(pow((double)2, (double)mask) - 1);
    return offs;
}

int hitMiss(int tag, int set)
{
    int start = cache.blocksPerSet * set;
    int end = cache.blocksPerSet * (set + 1);
    for (int i = start; i < end; i++)
    {
        if (cache.blocks[i].set == set && cache.blocks[i].tag == tag && cache.blocks[i].valid)
        {
            // HIT
            return i;
        }
    }
    // Indicates a miss
    return -1;
}

void updateLRU(int set, int usedBlock)
{
    int start = cache.blocksPerSet * set;
    int end = cache.blocksPerSet * (set + 1);
    for (int i = start; i < end; i++)
    {
        if (i != usedBlock)
        {
            cache.blocks[i].lruLabel = cache.blocks[i].lruLabel + 1;
        }
        else
        {
            cache.blocks[i].lruLabel = 0;
        }
    }
}

int evict(int set)
{
    int start = cache.blocksPerSet * set;
    int end = cache.blocksPerSet * (set + 1);
    int maxIndex = start;
    int maxLRU = cache.blocks[start].lruLabel;

    for (int i = start; i < end; i++)
    {
        if (cache.blocks[i].lruLabel >= maxLRU)
        {
            maxIndex = i;
            maxLRU = cache.blocks[i].lruLabel;
        }
    }
    return maxIndex;
}

int findOpening(int set)
{
    int start = cache.blocksPerSet * set;
    int end = cache.blocksPerSet * (set + 1);
    for (int i = start; i < end; i++)
    {
        if (!cache.blocks[i].valid)
        {
            return i;
        }
    }
    return -1;
}

int getAddr(int block)
{
    int numShiftOff = (int)log2((double)cache.blockSize);
    int numShiftSet = (int)log2((double)cache.numSets);
    int numShift = numShiftOff + numShiftSet;
    int recon = cache.blocks[block].tag;
    recon = recon << numShift;
    if (cache.numSets > 1)
    {
        int append = cache.blocks[block].set;
        append = append << numShiftOff;
        recon |= append;
    }
    return recon;
}