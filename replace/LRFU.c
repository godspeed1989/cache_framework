#include "LRFU.h"

BOOLEAN Init_LRFU (PVOID Ext, CACHE_SIZE_T Size)
{
    PLRFU_CACHE_POOL_EXT PExt = (PLRFU_CACHE_POOL_EXT)Ext;

    PExt->cold_used = 0;
    PExt->cold_size = Size / 3;
    PExt->hot_used = 0;
    PExt->hot_size = Size - PExt->cold_size;
    INIT_LIST_HEAD(&PExt->cold_list);
    INIT_LIST_HEAD(&PExt->hot_list);
    PExt->bpt_root = NULL;
    return TRUE;
}

VOID Destroy_LRFU (PVOID Ext)
{
    struct list_head *entry;
    PLRFU_CACHE_POOL_EXT PExt = (PLRFU_CACHE_POOL_EXT)Ext;

    entry = PExt->cold_list.next;
    while (entry != &PExt->cold_list)
    {
        PLRFU_CACHE_BLOCK pLRBlock = list_entry(entry, LRFU_CACHE_BLOCK, lrfu);
        entry = entry->next;
        FREE(pLRBlock);
    }
    entry = PExt->hot_list.next;
    while (entry != &PExt->hot_list)
    {
        PLRFU_CACHE_BLOCK pLRBlock = list_entry(entry, LRFU_CACHE_BLOCK, lrfu);
        entry = entry->next;
        FREE(pLRBlock);
    }
    if (PExt->bpt_root != NULL)
        Destroy_Tree(PExt->bpt_root);
}

BOOLEAN IsFull_LRFU (PCACHE_POOL CachePool)
{
    PLRFU_CACHE_POOL_EXT PExt = (PLRFU_CACHE_POOL_EXT)CachePool->Extend;;

    return (PExt->cold_used == PExt->cold_size);
}

/**
 * Get a Free Block From Cache Pool
 */
static PLRFU_CACHE_BLOCK __GetFreeBlock_LRFU (PCACHE_POOL CachePool)
{
    PLRFU_CACHE_BLOCK    pLRBlock = NULL;

    if (IsFull_LRFU(CachePool) == FALSE)
    {
        pLRBlock = (PLRFU_CACHE_BLOCK) MALLOC (sizeof(LRFU_CACHE_BLOCK));
        if (pLRBlock == NULL) goto l_error;
        ZERO(pLRBlock, sizeof(LRFU_CACHE_BLOCK));
        // Allocate Storage Space
        pLRBlock->CBlock.StorageIndex = StoragePoolAlloc(&CachePool->Storage);
        if (pLRBlock->CBlock.StorageIndex == -1) goto l_error;
        return pLRBlock;
    }
l_error:
    if (pLRBlock != NULL)
        FREE(pLRBlock);
    return NULL;
}

/**
 * Query a Cache Block from Pool By its Index
 */
BOOLEAN QueryPoolByIndex_LRFU (PCACHE_POOL CachePool, CACHE_INDEX_T Index, PCACHE_BLOCK *ppBlock)
{
    record *r;
    PLRFU_CACHE_POOL_EXT PExt = (PLRFU_CACHE_POOL_EXT)CachePool->Extend;

    r = Find_Record(PExt->bpt_root, Index);
    if (NULL == r)
        return FALSE;
    *ppBlock = r->value;
    return TRUE;
}

VOID IncreaseBlockReference_LRFU (PCACHE_POOL CachePool, PCACHE_BLOCK pBlock)
{
    PLRFU_CACHE_POOL_EXT PExt = (PLRFU_CACHE_POOL_EXT)CachePool->Extend;
    PLRFU_CACHE_BLOCK pLRBlock = container_of(pBlock, LRFU_CACHE_BLOCK, CBlock);

    pLRBlock->ref_count++;
    // Move to list head
    list_del(&pLRBlock->lrfu);
    if (pLRBlock->in_cold_list)
        list_add_head(&pLRBlock->lrfu, &PExt->cold_list);
    else
        list_add_head(&pLRBlock->lrfu, &PExt->hot_list);
}

/**
 * Add one Block to Cache Pool, When Pool is not Full
 */
PCACHE_BLOCK AddNewBlockToPool_LRFU (PCACHE_POOL CachePool, CACHE_INDEX_T Index, PVOID Data, BOOLEAN Modified)
{
    PCACHE_BLOCK        pBlock = NULL;
    PLRFU_CACHE_POOL_EXT  PExt = (PLRFU_CACHE_POOL_EXT)CachePool->Extend;
    PLRFU_CACHE_BLOCK pLRBlock = NULL;

    if ((pLRBlock = __GetFreeBlock_LRFU(CachePool)) != NULL)
    {
        pBlock = &pLRBlock->CBlock;
        pBlock->Index = Index;
        pBlock->Modified = Modified;
        StoragePoolWrite (
            &CachePool->Storage,
            pBlock->StorageIndex, 0,
            Data,
            BLOCK_SIZE
        );
        CachePool->Used++;

        PExt->cold_used++;
        pLRBlock->ref_count = 1;
        pLRBlock->in_cold_list = 1;
        list_add_head(&pLRBlock->lrfu, &PExt->cold_list);
        PExt->bpt_root = Insert(PExt->bpt_root, Index, pBlock);
    }
    return pBlock;
}

/**
 * Delete one Block from Cache Pool and Free it
 */
VOID DeleteOneBlockFromPool_LRFU (PCACHE_POOL CachePool, CACHE_INDEX_T Index)
{
    PCACHE_BLOCK       pBlock = NULL;
    PLRFU_CACHE_POOL_EXT PExt = (PLRFU_CACHE_POOL_EXT)CachePool->Extend;

    if (QueryPoolByIndex_LRFU(CachePool, Index, &pBlock) == TRUE)
    {
        PLRFU_CACHE_BLOCK pLRBlock = (PLRFU_CACHE_BLOCK)container_of(pBlock, LRFU_CACHE_BLOCK, CBlock);
        list_del(&pLRBlock->lrfu);
        PExt->bpt_root = Delete(PExt->bpt_root, Index);

        StoragePoolFree(&CachePool->Storage, pBlock->StorageIndex);
        CachePool->Used--;
        if (pLRBlock->in_cold_list)
            PExt->cold_used--;
        else
            PExt->hot_used--;
        FREE(pLRBlock);
    }
}

/**
 * Find a Cache Block to Replace, when Pool is Full
 */
PCACHE_BLOCK FindBlockToReplace_LRFU (PCACHE_POOL CachePool, CACHE_INDEX_T Index, PVOID Data, BOOLEAN Modified)
{
    ULONG               i, Count;
    PCACHE_BLOCK        pBlock = NULL;
    PLRFU_CACHE_POOL_EXT  PExt = (PLRFU_CACHE_POOL_EXT)CachePool->Extend;
    PLRFU_CACHE_BLOCK pLRBlock = NULL;

    // Backward find first refcnt < 2 in cold list
    // Move refcnt >= 2 to hot list
    pLRBlock = list_entry(PExt->cold_list.prev, LRFU_CACHE_BLOCK, lrfu);
    while (PExt->cold_used && pLRBlock->ref_count >= 2)
    {
        // remove from cold list
        PExt->cold_used--;
        list_del(&pLRBlock->lrfu);
        // move to hot list
        PExt->hot_used++;
        pLRBlock->ref_count = 0;
        pLRBlock->in_cold_list = 0;
        list_add_head(&pLRBlock->lrfu, &PExt->hot_list);

        pLRBlock = list_entry(PExt->cold_list.prev, LRFU_CACHE_BLOCK, lrfu);
    }

    // If hot list is over-full, move extras to cold list head
    // The cold list will never over-full for ...
    Count = PExt->hot_used > PExt->hot_size ?
            PExt->hot_used - PExt->hot_size : 0;
    for (i = 0; i < Count; i++)
    {
        pLRBlock = list_entry(PExt->hot_list.prev, LRFU_CACHE_BLOCK, lrfu);
        // remove from hot list
        PExt->hot_used--;
        list_del(&pLRBlock->lrfu);
        // move to cold list
        PExt->cold_used++;
        pLRBlock->ref_count = 0;
        pLRBlock->in_cold_list = 1;
        list_add_head(&pLRBlock->lrfu, &PExt->cold_list);
    }

    // add one block
    if (IsFull_LRFU(CachePool) == FALSE)
    {
        pBlock = AddNewBlockToPool_LRFU(CachePool, Index, Data, Modified);
        ASSERT(pBlock);
    }
    else // cold list is still full
    {
        pLRBlock = list_entry(PExt->cold_list.prev, LRFU_CACHE_BLOCK, lrfu);
        pBlock = &pLRBlock->CBlock;
        // remove last block at cold list
        list_del(&pLRBlock->lrfu);
        PExt->bpt_root = Delete(PExt->bpt_root, pBlock->Index);
        // update data
        pBlock->Index = Index;
        pBlock->Modified = Modified;
        StoragePoolWrite (
            &CachePool->Storage,
            pBlock->StorageIndex, 0,
            Data,
            BLOCK_SIZE
        );
        // add to cold list
        pLRBlock->ref_count = 1;
        pLRBlock->in_cold_list = 1;
        list_add_head(&pLRBlock->lrfu, &PExt->cold_list);
        PExt->bpt_root = Insert(PExt->bpt_root, pBlock->Index, pBlock);
    }

    return pBlock;
}

VOID ForEachCacheBlock_LRFU (PCACHE_POOL CachePool, VOID(*Func)(PCACHE_POOL, PCACHE_BLOCK))
{
    struct list_head             *pos;
    PCACHE_BLOCK        pBlock = NULL;
    PLRFU_CACHE_POOL_EXT  PExt = (PLRFU_CACHE_POOL_EXT)CachePool->Extend;
    PLRFU_CACHE_BLOCK pLRBlock = NULL;

    list_for_each(pos, &PExt->cold_list)
    {
        pLRBlock = list_entry(pos, LRFU_CACHE_BLOCK, lrfu);
        pBlock = &pLRBlock->CBlock;
        Func(CachePool, pBlock);
    }

    list_for_each(pos, &PExt->hot_list)
    {
        pLRBlock = list_entry(pos, LRFU_CACHE_BLOCK, lrfu);
        pBlock = &pLRBlock->CBlock;
        Func(CachePool, pBlock);
    }
}
