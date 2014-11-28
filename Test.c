#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "Cache.h"

#define MAIN_MEM_SIZE_MB        ((OFFSET_T)100)
#define MAIN_MEM_SIZE_BYTE      (MAIN_MEM_SIZE_MB << 20)
#define CACHE_POOL_SIZE_MB      ((OFFSET_T)5)
#define CACHE_POOL_NUM_BLOCKS   ((CACHE_POOL_SIZE_MB << 20)/(BLOCK_SIZE))

#define     LRU     1
#define     LRFU    1

#if LRU
#include "LRU.h"
static CACHE_POOL LRU_Cache_Pool;
static Cache_Operations LRU_Operations =
{
    .Init                   = Init_LRU,
    .Destroy                = Destroy_LRU,
    .AddNewBlockToPool      = AddNewBlockToPool_LRU,
    .DeleteOneBlockFromPool = DeleteOneBlockFromPool_LRU,
    .QueryPoolByIndex       = QueryPoolByIndex_LRU,
    .FindBlockToReplace     = FindBlockToReplace_LRU,
    .IncreaseBlockReference = IncreaseBlockReference_LRU,
    .IsFull                 = IsFull_LRU,
    .ForEachCacheBlock      = ForEachCacheBlock_LRU,
};
static LRU_CACHE_POOL_EXT LRU_Pool_Ext;
#endif

#if LRFU
#include "LRFU.h"
static CACHE_POOL LRFU_Cache_Pool;
static Cache_Operations LRFU_Operations =
{
    .Init                   = Init_LRFU,
    .Destroy                = Destroy_LRFU,
    .AddNewBlockToPool      = AddNewBlockToPool_LRFU,
    .DeleteOneBlockFromPool = DeleteOneBlockFromPool_LRFU,
    .QueryPoolByIndex       = QueryPoolByIndex_LRFU,
    .FindBlockToReplace     = FindBlockToReplace_LRFU,
    .IncreaseBlockReference = IncreaseBlockReference_LRFU,
    .IsFull                 = IsFull_LRFU,
    .ForEachCacheBlock      = ForEachCacheBlock_LRFU,
};
static LRFU_CACHE_POOL_EXT LRFU_Pool_Ext;
#endif

// Cache Pools using different replace algorithm
static PCACHE_POOL Cache_Pools[] =
{
#if LRU
    &LRU_Cache_Pool,
#endif
#if LRFU
    &LRFU_Cache_Pool,
#endif
    NULL
};

static PUCHAR Main_Mem;
static PUCHAR Verify_Mem;

void do_test_all (void);

int main ()
{
    printf("MAIN_MEM_SIZE_BYTE:     %lu\n", MAIN_MEM_SIZE_BYTE);
    printf("BLOCK_SIZE:             %lu\n", BLOCK_SIZE);
    printf("CACHE_POOL_NUM_BLOCKS:  %lu\n", CACHE_POOL_NUM_BLOCKS);
    printf("\n");

#if LRU
    if (InitCachePool(&LRU_Cache_Pool, CACHE_POOL_NUM_BLOCKS,
                      &LRU_Operations, &LRU_Pool_Ext, "LRU") == FALSE)
    {
        printf("Init LRU Cache Pool Error\n");
        return -1;
    }
#endif
#if LRFU
    if (InitCachePool(&LRFU_Cache_Pool, CACHE_POOL_NUM_BLOCKS,
                      &LRFU_Operations, &LRFU_Pool_Ext, "LRFU") == FALSE)
    {
        printf("Init LRFU Cache Pool Error\n");
        return -1;
    }
#endif

    do_test_all();

#if LRU
    DestroyCachePool(&LRU_Cache_Pool);
#endif
#if LRFU
    DestroyCachePool(&LRFU_Cache_Pool);
#endif
    return 0;
}

#define READ    1
#define WRITE   0

static void do_request (int type, PUCHAR Buffer, OFFSET_T Offset, LENGTH_T Length)
{
    int i;
    for (i = 0; Cache_Pools[i] != NULL; i++)
    {
        if (type == READ)
        {
            Cache_Pools[i]->ReadCount++;
            // Cache Full Hitted
            if (QueryAndCopyFromCachePool(Cache_Pools[i], Buffer, Offset, Length) == TRUE)
            {
                assert(!memcmp(Main_Mem+Offset, Buffer, Length));
                Cache_Pools[i]->ReadHit++;
            }
            else
            {
                // read from main memory
                memcpy(Buffer, Main_Mem+Offset, Length);
                ReadUpdateCachePool (Cache_Pools[i], Buffer, Offset, Length);
            }
        }
        else // WRITE
        {
            Cache_Pools[i]->WriteCount++;
            // WriteThrough: write to main memory
            memcpy(Main_Mem+Offset, Buffer, Length);
            WriteUpdateCachePool (Cache_Pools[i], Buffer, Offset, Length);
        }
    }
}

static VOID verify_block_func (PCACHE_POOL CachePool, PCACHE_BLOCK pBlock)
{
    PUCHAR BlockData = (PUCHAR)malloc(BLOCK_SIZE);

    StoragePoolRead(&CachePool->Storage, BlockData, pBlock->StorageIndex, 0, BLOCK_SIZE);
    if (memcmp(BlockData, Main_Mem+pBlock->Index*BLOCK_SIZE, BLOCK_SIZE))
    {
        printf("%s: verify block error\n", CachePool->name);
        assert(0);
    }

    free(BlockData);
}

static void data_rand (const void* p, int size)
{
    int i;
    unsigned char *ptr = (unsigned char *)p;
    for (i = 0; i < size; i++)
        *(ptr+i) = (unsigned char)rand();
}

void zipf_pick(int num, int Range, double A, int *out);

void do_test_all (void)
{
    #define         RUN_SECOND      (5)
    #define         MAX_REQ_BLK     (100)
    #define         BUFFER_SIZE     (MAX_REQ_BLK*BLOCK_SIZE)
    #define         LOOP_SIZE       (200)
    time_t          start;
    unsigned long   loop;
    int             i, type, zipf_out[LOOP_SIZE];
    OFFSET_T        Offset;
    LENGTH_T        Length;
    PUCHAR          ReqBuffer;

    loop = 0;
    start = time(NULL);
    srand((unsigned int)start);

    ReqBuffer = (PUCHAR)malloc(BUFFER_SIZE);
    Main_Mem = (PUCHAR)malloc(MAIN_MEM_SIZE_BYTE);
    Verify_Mem = (PUCHAR)malloc(MAIN_MEM_SIZE_BYTE);
    assert(ReqBuffer && Main_Mem && Verify_Mem);

    memset(ReqBuffer, 0, BUFFER_SIZE);
    memset(Main_Mem, 0, MAIN_MEM_SIZE_BYTE);
    memset(Verify_Mem, 0, MAIN_MEM_SIZE_BYTE);

    for (;;)
    {
        // run RUN_SECONDS test
        if (time(NULL)-start > RUN_SECOND)
            break;

        zipf_pick(LOOP_SIZE, 100, 0.25, zipf_out);
        for (i = 0; i < LOOP_SIZE; i++)
        {
            // generate pseudo request
            type = rand() & 1;
            Offset = zipf_out[i] * MAIN_MEM_SIZE_BYTE / 100;
            Length = (rand() % MAX_REQ_BLK) * BLOCK_SIZE + rand() % BLOCK_SIZE;
            assert(MAIN_MEM_SIZE_BYTE > Offset);
            if (Length > (MAIN_MEM_SIZE_BYTE - Offset))
                Length = MAIN_MEM_SIZE_BYTE - Offset;
            data_rand(ReqBuffer, Length);

            // apply request to verification memory
            if (type == WRITE)
            {
                memcpy(Verify_Mem+Offset, ReqBuffer, Length);
            }

            do_request(type, ReqBuffer, Offset, Length);
        }

        loop++;
    }

    printf("do_test(): %lu loops in %lu s\n",
            loop*LOOP_SIZE, (unsigned long)(time(NULL)-start));

    printf("Verify Main Memory......\n");
    if (memcmp(Main_Mem, Verify_Mem, MAIN_MEM_SIZE_BYTE))
    {
        FILE *vm, *mm;
        vm = fopen("vm.hex", "wb");
        mm = fopen("mm.hex", "wb");
        fwrite(Verify_Mem, MAIN_MEM_SIZE_BYTE, 1, vm);
        fwrite(Main_Mem, MAIN_MEM_SIZE_BYTE, 1, mm);
        fclose(vm);
        fclose(mm);

        printf("Error: Main_Mem != Verify_Mem\n");
        goto clean;
    }

    for (i = 0; Cache_Pools[i] != NULL; i++)
    {
        printf("-------%s-------\n", Cache_Pools[i]->name);
        printf("Verify each cache block......\n");
        Cache_Pools[i]->Cache_Ops->ForEachCacheBlock(Cache_Pools[i], verify_block_func);
        printf("Usage: %lu / %lu\n", Cache_Pools[i]->Used, Cache_Pools[i]->Size);
        printf("Store: %lu / %lu\n", Cache_Pools[i]->Storage.Used, Cache_Pools[i]->Storage.Size);
        printf("Read:  %lu / %lu\n", Cache_Pools[i]->ReadHit, Cache_Pools[i]->ReadCount);
        printf("Write: %lu / %lu\n", Cache_Pools[i]->WriteHit, Cache_Pools[i]->WriteCount);
    }

clean:
    free(ReqBuffer);
    free(Main_Mem);
    free(Verify_Mem);
}

/*
 * 生成zipf分布的随机概率分布
 */
static void generate_pf(double *pf, int R, double A)
{
    int i;
    double sum = 0.0;
    const double C = 1.0;   // 一般取1
    for (i = 0; i < R; i++)
    {
        sum += C/pow((double)(i+2), A);
    }
    for (i = 0; i < R; i++)
    {
        if (i == 0)
            pf[i] = C/pow((double)(i+2), A)/sum;
        else
            pf[i] = pf[i-1] + C/pow((double)(i+2), A)/sum;
    }
}

/**
 * num  : 输出个数
 * A    : 分布在前半部分的百分比，0<A<1
 * Range: 概率分布范围
 * out  : 结果输出，out[i] = [0, Range)
 */
void zipf_pick(int num, int Range, double A, int *out)
{
    int i;
    double *pf; // 值为0~1之间, 是单个f(r)的累加值

    A = 1.0 - A;
    pf = (double*)malloc(Range*sizeof(double));
    generate_pf (pf, Range, A);

    srand((unsigned int)time(NULL));
    for (i = 0; i < num; i++)
    {
        int index = 0;
        double data = (double)rand()/RAND_MAX;
        while (data > pf[index])
            index++;
        out[i] = index;
    }
    free(pf);
}
