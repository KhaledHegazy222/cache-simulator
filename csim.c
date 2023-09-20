#include "cachelab.h"
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct
{
    bool valid;
    int tag;
    char *blocks;
    int updatedAt;
} cacheLine;

int linesNumber = 0;
int setsBits = 0;
int setsNumber = 0;
int blocksNumber = 0;
int blocksBits = 0;
char traceFile[50];
bool verbose = false;
int time = 0;

int getCacheSet(int address)
{
    return (address & (((1 << (setsBits + blocksBits)) - 1) - ((1 << blocksBits) - 1))) >> blocksBits;
}
int getCacheTag(int address)
{
    return (address & (~((1 << (setsBits + blocksBits)) - 1))) >> (blocksBits + setsBits);
}

cacheLine *getCacheLine(cacheLine *cache, int tag)
{
    int lastTime = time;
    cacheLine *lastUsedOne = cache;
    for (int i = 0; i < linesNumber; i++)
    {
        if (lastTime > cache[i].updatedAt)
        {
            lastTime = cache[i].updatedAt;
            lastUsedOne = cache + i;
        }
        if (cache[i].valid && cache[i].tag == tag)
            return (cache + i);
    }

    return lastUsedOne;
}

void loadCacheLine(cacheLine *cache, int tag)
{
    cache->valid = true;
    cache->tag = tag;
    cache->updatedAt = ++time;
}

int main(int argc, char **argv)
{
    char opt;
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            printf("Hello World\n");
            break;
        case 's':
            setsBits = atoi(optarg);
            setsNumber = 1 << setsBits;
            break;
        case 'E':
            linesNumber = atoi(optarg);
            break;
        case 'b':
            blocksBits = atoi(optarg);
            blocksNumber = 1 << blocksBits;
            break;
        case 'v':
            verbose = true;
            break;
        case 't':
            int filePathLength = strlen(optarg);
            strncpy(traceFile, optarg, filePathLength);
            break;

        default:
            break;
        }
    }

    cacheLine *cache = (cacheLine *)calloc(setsNumber * linesNumber, sizeof(cacheLine));

    FILE *filePtr = NULL;
    filePtr = fopen(traceFile, "r");
    char opCh;
    int address, size;
    int hits = 0;
    int misses = 0;
    int evictions = 0;
    while (fscanf(filePtr, " %c%x,%x", &opCh, &address, &size) != EOF)
    {

        int currentHits = 0;
        int currentMisses = 0;
        int currentEvictions = 0;

        if (opCh == 'I')
            continue;
        else if (opCh == 'M')
            currentHits++;

        int setIndex = getCacheSet(address);

        int tag = getCacheTag(address);

        cacheLine *currentLine = getCacheLine(cache + (setIndex * linesNumber), tag);

        if (!currentLine->valid)
        {
            currentMisses++;
        }
        else if (currentLine->tag == tag)
        {
            currentHits++;
        }
        else
        {

            currentEvictions++;
            currentMisses++;
        }
        hits += currentHits;
        misses += currentMisses;
        evictions += currentEvictions;
        if (verbose)
        {
            printf("%c %x,%d", opCh, address, size);
            while (currentMisses--)
                printf(" miss");
            while (currentEvictions--)
                printf(" eviction");
            while (currentHits--)
                printf(" hit");
            printf("\n");
        }

        loadCacheLine(currentLine, tag);
    }

    printSummary(hits, misses, evictions);

    free(cache);
    fclose(filePtr);
    return 0;
}
