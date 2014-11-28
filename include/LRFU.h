#ifndef __LRFU_H__
#define __LRFU_H__

#include "bpt.h"
#include "List.h"

// inherent from basic CACHE_BLOCK
typedef struct _LRFU_CACHE_BLOCK
{
    CACHE_BLOCK             CBlock;
    // Extend
    struct list_head        lrfu;
    int                     in_cold_list;
    ULONG                   ref_count;
}LRFU_CACHE_BLOCK, *PLRFU_CACHE_BLOCK;

// LRU extension of CACHE_POOL
typedef struct _LRFU_CACHE_POOL_EXT
{
    CACHE_SIZE_T            cold_used;
    CACHE_SIZE_T            cold_size;
    CACHE_SIZE_T            hot_used;
    CACHE_SIZE_T            hot_size;
    struct list_head        cold_list;
    struct list_head        hot_list;
    node*                   bpt_root;
}LRFU_CACHE_POOL_EXT, *PLRFU_CACHE_POOL_EXT;

BOOLEAN      Init_LRFU (PVOID Ext, CACHE_SIZE_T Size);
VOID         Destroy_LRFU (PVOID Ext);

BOOLEAN      IsFull_LRFU (PCACHE_POOL CachePool);
BOOLEAN      QueryPoolByIndex_LRFU (PCACHE_POOL CachePool, CACHE_INDEX_T Index, PCACHE_BLOCK *ppBlock);
VOID         IncreaseBlockReference_LRFU (PCACHE_POOL CachePool, PCACHE_BLOCK pBlock);
PCACHE_BLOCK AddNewBlockToPool_LRFU (PCACHE_POOL CachePool, CACHE_INDEX_T Index, PVOID Data, BOOLEAN Modified);
PCACHE_BLOCK FindBlockToReplace_LRFU (PCACHE_POOL CachePool, CACHE_INDEX_T Index, PVOID Data, BOOLEAN Modified);
VOID         DeleteOneBlockFromPool_LRFU (PCACHE_POOL CachePool, CACHE_INDEX_T Index);

VOID         ForEachCacheBlock_LRFU (PCACHE_POOL CachePool, VOID(*Func)(PCACHE_POOL, PCACHE_BLOCK));

#endif
