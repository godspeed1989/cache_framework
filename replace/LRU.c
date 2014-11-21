#include "LRU.h"

BOOLEAN Init_LRU (PVOID Ext)
{
    PLRU_CACHE_POOL_EXT PExt = (PLRU_CACHE_POOL_EXT)Ext;

    INIT_LIST_HEAD(&PExt->list);
    PExt->bpt_root = NULL;
    return TRUE;
}

VOID Destroy_LRU (PVOID Ext)
{
    struct list_head *entry;
    PLRU_CACHE_POOL_EXT PExt = (PLRU_CACHE_POOL_EXT)Ext;

    entry = PExt->list.next;
    while (entry != &PExt->list)
    {
        PLRU_CACHE_BLOCK pLBlock = list_entry(entry, LRU_CACHE_BLOCK, lru);
        entry = entry->next;
        FREE(pLBlock);
    }
    if (PExt->bpt_root != NULL)
        Destroy_Tree(PExt->bpt_root);
}

BOOLEAN IsFull_LRU (PCACHE_POOL CachePool)
{
    return (CachePool->Size == CachePool->Used);
}

/**
 * Get a Free Block From Cache Pool
 */
static PLRU_CACHE_BLOCK __GetFreeBlock_LRU (PCACHE_POOL CachePool)
{
    PLRU_CACHE_BLOCK    pLBlock = NULL;

    if (IsFull_LRU(CachePool) == FALSE)
    {
        pLBlock = (PLRU_CACHE_BLOCK) MALLOC (sizeof(LRU_CACHE_BLOCK));
        if (pLBlock == NULL) goto l_error;
        // Allocate Storage Space
        pLBlock->CBlock.StorageIndex = StoragePoolAlloc(&CachePool->Storage);
        if (pLBlock->CBlock.StorageIndex == -1) goto l_error;
        return pLBlock;
    }
l_error:
    if (pLBlock != NULL)
        FREE(pLBlock);
    return NULL;
}

/**
 * Query a Cache Block from Pool By its Index
 */
BOOLEAN QueryPoolByIndex_LRU (PCACHE_POOL CachePool, CACHE_INDEX_T Index, PCACHE_BLOCK *ppBlock)
{
    record *r;
    PLRU_CACHE_POOL_EXT PExt = (PLRU_CACHE_POOL_EXT)CachePool->Extend;

    r = Find_Record(PExt->bpt_root, Index);
    if (NULL == r)
        return FALSE;
    *ppBlock = r->value;
    return TRUE;
}

VOID IncreaseBlockReference_LRU (PCACHE_POOL CachePool, PCACHE_BLOCK pBlock)
{
    PLRU_CACHE_POOL_EXT PExt = (PLRU_CACHE_POOL_EXT)CachePool->Extend;
    PLRU_CACHE_BLOCK pLBlock = container_of(pBlock, LRU_CACHE_BLOCK, CBlock);

    // Move to LRU list head
    list_del(&pLBlock->lru);
    list_add_head(&pLBlock->lru, &PExt->list);
}

/**
 * Add one Block to Cache Pool, When Pool is not Full
 */
PCACHE_BLOCK AddNewBlockToPool_LRU (PCACHE_POOL CachePool, CACHE_INDEX_T Index, PVOID Data, BOOLEAN Modified)
{
    PCACHE_BLOCK      pBlock = NULL;
    PLRU_CACHE_POOL_EXT PExt = (PLRU_CACHE_POOL_EXT)CachePool->Extend;
    PLRU_CACHE_BLOCK pLBlock = NULL;

    if ((pLBlock = __GetFreeBlock_LRU(CachePool)) != NULL)
    {
        pBlock = &pLBlock->CBlock;
        pBlock->Index = Index;
        pBlock->Modified = Modified;
        StoragePoolWrite (
            &CachePool->Storage,
            pBlock->StorageIndex, 0,
            Data,
            BLOCK_SIZE
        );
        CachePool->Used++;

        list_add_head(&pLBlock->lru, &PExt->list);
        PExt->bpt_root = Insert(PExt->bpt_root, Index, pBlock);
    }
    return pBlock;
}

/**
 * Delete one Block from Cache Pool and Free it
 */
VOID DeleteOneBlockFromPool_LRU (PCACHE_POOL CachePool, CACHE_INDEX_T Index)
{
    PCACHE_BLOCK      pBlock = NULL;
    PLRU_CACHE_POOL_EXT PExt = (PLRU_CACHE_POOL_EXT)CachePool->Extend;

    if (QueryPoolByIndex_LRU(CachePool, Index, &pBlock) == TRUE)
    {
        PLRU_CACHE_BLOCK pLBlock = (PLRU_CACHE_BLOCK)container_of(pBlock, LRU_CACHE_BLOCK, CBlock);
        list_del(&pLBlock->lru);
        PExt->bpt_root = Delete(PExt->bpt_root, Index);

        StoragePoolFree(&CachePool->Storage, pBlock->StorageIndex);
        CachePool->Used--;
    }
}

/**
 * Find a Cache Block to Replace, when Pool is Full
 */
PCACHE_BLOCK FindBlockToReplace_LRU (PCACHE_POOL CachePool, CACHE_INDEX_T Index, PVOID Data, BOOLEAN Modified)
{
    PCACHE_BLOCK      pBlock = NULL;
    PLRU_CACHE_POOL_EXT PExt = (PLRU_CACHE_POOL_EXT)CachePool->Extend;
    PLRU_CACHE_BLOCK pLBlock = NULL;

    pLBlock = list_entry(PExt->list.prev, LRU_CACHE_BLOCK, lru);
    pBlock = &pLBlock->CBlock;

    {
        PExt->bpt_root = Delete(PExt->bpt_root, pBlock->Index);

        pBlock->Index = Index;
        pBlock->Modified = Modified;
        StoragePoolWrite (
            &CachePool->Storage,
            pBlock->StorageIndex, 0,
            Data,
            BLOCK_SIZE
        );

        list_del(&pLBlock->lru);
        list_add_head(&pLBlock->lru, &PExt->list);
        PExt->bpt_root = Insert(PExt->bpt_root, Index, pBlock);
    }

    return pBlock;
}

VOID ForEachCacheBlock_LRU (PCACHE_POOL CachePool, VOID(*Func)(PCACHE_POOL, PCACHE_BLOCK))
{
    struct list_head           *pos;
    PCACHE_BLOCK      pBlock = NULL;
    PLRU_CACHE_POOL_EXT PExt = (PLRU_CACHE_POOL_EXT)CachePool->Extend;
    PLRU_CACHE_BLOCK pLBlock = NULL;

    list_for_each(pos, &PExt->list)
    {
        pLBlock = list_entry(pos, LRU_CACHE_BLOCK, lru);
        pBlock = &pLBlock->CBlock;
        Func(CachePool, pBlock);
    }
}
