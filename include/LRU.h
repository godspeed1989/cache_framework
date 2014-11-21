#ifndef __LRU_H__
#define __LRU_H__

#include "bpt.h"
#include "List.h"

// inherent from basic CACHE_BLOCK
typedef struct _LRU_CACHE_BLOCK
{
    CACHE_BLOCK             CBlock;
    struct list_head        lru;
}LRU_CACHE_BLOCK, *PLRU_CACHE_BLOCK;

// LRU extension of CACHE_POOL
typedef struct _LRU_CACHE_POOL_EXT
{
    struct list_head        list;
    node*                   bpt_root;
}LRU_CACHE_POOL_EXT, *PLRU_CACHE_POOL_EXT;

BOOLEAN      Init_LRU (PVOID Ext);
VOID         Destroy_LRU (PVOID Ext);

BOOLEAN      IsFull_LRU (PCACHE_POOL CachePool);
BOOLEAN      QueryPoolByIndex_LRU (PCACHE_POOL CachePool, CACHE_INDEX_T Index, PCACHE_BLOCK *ppBlock);
VOID         IncreaseBlockReference_LRU (PCACHE_POOL CachePool, PCACHE_BLOCK pBlock);
PCACHE_BLOCK AddNewBlockToPool_LRU (PCACHE_POOL CachePool, CACHE_INDEX_T Index, PVOID Data, BOOLEAN Modified);
PCACHE_BLOCK FindBlockToReplace_LRU (PCACHE_POOL CachePool, CACHE_INDEX_T Index, PVOID Data, BOOLEAN Modified);
VOID         DeleteOneBlockFromPool_LRU (PCACHE_POOL CachePool, CACHE_INDEX_T Index);

VOID         ForEachCacheBlock_LRU (PCACHE_POOL CachePool, VOID(*Func)(PCACHE_POOL, PCACHE_BLOCK));

#endif
