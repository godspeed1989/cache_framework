#include "Cache.h"
#include "bpt.h"

BOOLEAN
InitCachePool (
    PCACHE_POOL         CachePool,
    CACHE_SIZE_T        Size,
    Cache_Operations   *Cache_Ops,
    PVOID               Extend,
    const char         *name
)
{
    ZERO(CachePool, sizeof(CACHE_POOL));
    if (InitStoragePool(&CachePool->Storage, Size) == FALSE)
        return FALSE;
    CachePool->Size         = Size;
    CachePool->Cache_Ops    = Cache_Ops;
    CachePool->Extend       = Extend;
    CachePool->name         = name;
    if (CachePool->Cache_Ops->Init(Extend) == FALSE)
        goto l_error;
    return TRUE;
l_error:
    DestroyStoragePool(&CachePool->Storage);
    return FALSE;
}

VOID
DestroyCachePool (
    PCACHE_POOL CachePool
)
{
    CachePool->Cache_Ops->Destroy(CachePool->Extend);
    DestroyStoragePool(&CachePool->Storage);
    ZERO(CachePool, sizeof(CACHE_POOL));
}

/**
 * Read Update Cache Pool with Buffer
 */
VOID
ReadUpdateCachePool (
    PCACHE_POOL CachePool,
    PUCHAR Buf, OFFSET_T Offset, LENGTH_T Length
)
{
    PUCHAR              origBuf;
    PCACHE_BLOCK        pBlock;
    OFFSET_T            front_offset;
    LENGTH_T            i, front_skip, end_cut, origLen;
    BOOLEAN             front_broken, end_broken;
    Cache_Operations   *ops = CachePool->Cache_Ops;

    origBuf = Buf;
    origLen = Length;

    detect_broken (Offset, Length, front_broken, end_broken,
                   front_offset, front_skip, end_cut);
    Offset /= BLOCK_SIZE;
    Length /= BLOCK_SIZE;

    if (front_broken == TRUE)
    {
        Buf += front_skip;
    }
    for (i = 0; i < Length; i++)
    {
        CACHE_INDEX_T Index = Offset + i;
        if (ops->QueryPoolByIndex(CachePool, Index, &pBlock) == TRUE)
        {
            ops->IncreaseBlockReference(CachePool, pBlock);
        }
        else if (ops->IsFull(CachePool) == FALSE)
        {
            pBlock = ops->AddNewBlockToPool(CachePool, Index, Buf, FALSE);
        }
        else
        {
            pBlock = ops->FindBlockToReplace(CachePool, Index, Buf, FALSE);
        }
        Buf += BLOCK_SIZE;
    }
    if (end_broken == TRUE)
    {
        Buf += end_cut;
    }
    ASSERT(Buf - origBuf == origLen);

}

#define _WRITE_TO_CBLOCK(pBlock,off,Buf,len)            \
    {                                                   \
        StoragePoolWrite (                              \
            &CachePool->Storage,                        \
            pBlock->StorageIndex,                       \
            off,                                        \
            Buf,                                        \
            len                                         \
        );                                              \
        ops->IncreaseBlockReference(CachePool, pBlock);\
    }
/**
 * Write Update Cache Pool with Buffer
 */
VOID
WriteUpdateCachePool (
    PCACHE_POOL CachePool,
    PUCHAR Buf, OFFSET_T Offset, LENGTH_T Length
)
{
    PUCHAR              origBuf;
    OFFSET_T            front_offset;
    LENGTH_T            i, front_skip, end_cut, origLen;
    BOOLEAN             front_broken, end_broken;
    PCACHE_BLOCK        pBlock;
    Cache_Operations   *ops = CachePool->Cache_Ops;

    origBuf = Buf;
    origLen = Length;

    detect_broken (Offset, Length, front_broken, end_broken,
                   front_offset, front_skip, end_cut);
    Offset /= BLOCK_SIZE;
    Length /= BLOCK_SIZE;

    if (front_broken)
    {
        if (ops->QueryPoolByIndex(CachePool, Offset-1, &pBlock) == TRUE)
        {
            _WRITE_TO_CBLOCK(pBlock, front_offset, Buf, front_skip);
        }
        Buf += front_skip;
    }
    for (i = 0; i < Length; i++)
    {
        CACHE_INDEX_T Index = Offset + i;
        if(ops->QueryPoolByIndex(CachePool, Index, &pBlock) == TRUE)
        {
            _WRITE_TO_CBLOCK(pBlock, 0, Buf, BLOCK_SIZE);
        }
        else if (ops->IsFull(CachePool) == FALSE)
        {
            pBlock = ops->AddNewBlockToPool(CachePool, Index, Buf, TRUE);
        }
        else
        {
            pBlock = ops->FindBlockToReplace(CachePool, Index, Buf, FALSE);
        }
        Buf += BLOCK_SIZE;
    }
    if (end_broken)
    {
        if (ops->QueryPoolByIndex(CachePool, Offset+Length, &pBlock) == TRUE)
        {
            _WRITE_TO_CBLOCK(pBlock, 0, Buf, end_cut);
        }
        Buf += end_cut;
    }
    ASSERT (Buf - origBuf == origLen);
}

/**
 * Query Cache Pool (If the _READ_ Request is Matched)
 * If it's fully matched, copy to the request buffer and return TRUE,
 * else return FALSE
 */
BOOLEAN
QueryAndCopyFromCachePool (
    PCACHE_POOL CachePool,
    PUCHAR Buf, OFFSET_T Offset, LENGTH_T Length
)
{
    PUCHAR              origBuf;
    OFFSET_T            front_offset;
    LENGTH_T            i, front_skip, end_cut, origLen;
    BOOLEAN             Ret, front_broken, end_broken;
    PCACHE_BLOCK       *ppInternalBlocks = NULL;
    Cache_Operations   *ops = CachePool->Cache_Ops;

    Ret = FALSE;
    origBuf = Buf;
    origLen = Length;

    detect_broken (Offset, Length, front_broken, end_broken,
                   front_offset, front_skip, end_cut);
    Offset /= BLOCK_SIZE;
    Length /= BLOCK_SIZE;

    ppInternalBlocks = (PCACHE_BLOCK*) MALLOC ((Length+2) * sizeof(PCACHE_BLOCK));
    if (ppInternalBlocks == NULL)
        goto l_error;

    // Query Pool to Check If Fully Matched
    if (front_broken == TRUE && ops->QueryPoolByIndex(CachePool, Offset-1, ppInternalBlocks+0) == FALSE)
        goto l_error;
    for (i = 0; i < Length; i++)
        if (ops->QueryPoolByIndex(CachePool, Offset+i, ppInternalBlocks+i+1) == FALSE)
            goto l_error;
    if (end_broken == TRUE && ops->QueryPoolByIndex(CachePool, Offset+Length, ppInternalBlocks+Length+1) == FALSE)
        goto l_error;

#define _copy_data(pBlock,off,len)                      \
        StoragePoolRead (                               \
            &CachePool->Storage,                        \
            Buf,                                        \
            pBlock->StorageIndex,                       \
            off,                                        \
            len                                         \
        );                                              \
        Buf += len;                                     \
        ops->IncreaseBlockReference(CachePool, pBlock);

    if (front_broken == TRUE)
    {
        _copy_data(ppInternalBlocks[0], front_offset, front_skip);
    }

    for (i = 0; i < Length; i++)
    {
        _copy_data(ppInternalBlocks[i+1], 0, BLOCK_SIZE);
    }

    if (end_broken == TRUE)
    {
        _copy_data(ppInternalBlocks[Length+1], 0, end_cut);
    }

    ASSERT(Buf - origBuf == origLen);
    Ret = TRUE;

l_error:
    if (ppInternalBlocks != NULL)
        FREE(ppInternalBlocks);
    return Ret;
}

/**
 * Query Cache Pool (If the _WRITE_ Request is Matched)
 * If it's fully matched, update cache and return TRUE,
 * else return FALSE
 */
BOOLEAN
QueryAndWriteToCachePool (
    PCACHE_POOL CachePool,
    PUCHAR Buf, OFFSET_T Offset, LENGTH_T Length
)
{
    PUCHAR              origBuf;
    OFFSET_T            front_offset;
    LENGTH_T            i, front_skip, end_cut, origLen;
    BOOLEAN             Ret, front_broken, end_broken;
    PCACHE_BLOCK       *ppInternalBlocks = NULL;
    Cache_Operations   *ops = CachePool->Cache_Ops;

    Ret = FALSE;
    origBuf = Buf;
    origLen = Length;

    detect_broken (Offset, Length, front_broken, end_broken,
                   front_offset, front_skip, end_cut);
    Offset /= BLOCK_SIZE;
    Length /= BLOCK_SIZE;

    ppInternalBlocks = (PCACHE_BLOCK*) MALLOC ((Length+2) * sizeof(PCACHE_BLOCK));
    if (ppInternalBlocks == NULL)
        goto l_error;

    // Query Pool to Check If Fully Matched
    if (front_broken == TRUE && ops->QueryPoolByIndex(CachePool, Offset-1, ppInternalBlocks+0) == FALSE)
        goto l_error;
    for (i = 0; i < Length; i++)
        if (ops->QueryPoolByIndex(CachePool, Offset+i, ppInternalBlocks+i+1) == FALSE)
            goto l_error;
    if (end_broken == TRUE && ops->QueryPoolByIndex(CachePool, Offset+Length, ppInternalBlocks+Length+1) == FALSE)
        goto l_error;

    if (front_broken == TRUE)
    {
        _WRITE_TO_CBLOCK(ppInternalBlocks[0], front_offset, Buf, front_skip);
        Buf += front_skip;
    }

    for (i = 0; i < Length; i++)
    {
        _WRITE_TO_CBLOCK(ppInternalBlocks[i+1], 0, Buf, BLOCK_SIZE);
        Buf += BLOCK_SIZE;
    }

    if (end_broken == TRUE)
    {
        _WRITE_TO_CBLOCK(ppInternalBlocks[Length+1], 0, Buf, end_cut);
        Buf += end_cut;
    }

    ASSERT(Buf - origBuf == origLen);
    Ret = TRUE;

l_error:
    if (ppInternalBlocks != NULL)
        FREE(ppInternalBlocks);
    return Ret;
}
