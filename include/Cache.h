#ifndef __CACHE_H__
#define __CACHE_H__

#include "Share.h"
#include "Storage.h"

struct _Cache_Operations;
typedef struct _Cache_Operations Cache_Operations;

typedef struct _CACHE_BLOCK
{
    CACHE_INDEX_T               Index;
    BOOLEAN                     Modified;
    STORAGE_INDEX_T             StorageIndex;
}CACHE_BLOCK, *PCACHE_BLOCK;

typedef struct _CACHE_POOL
{
    CACHE_SIZE_T                Size;
    CACHE_SIZE_T                Used;
    ULONG                       ReadHit;
    ULONG                       ReadCount;
    ULONG                       WriteHit;
    ULONG                       WriteCount;
    Cache_Operations           *Cache_Ops;
    STORAGE_POOL                Storage;
    PVOID                       Extend;
    const char                 *name;
}CACHE_POOL, *PCACHE_POOL;

/**
 * Cache Common Interface
 */
BOOLEAN
    InitCachePool (
        PCACHE_POOL         CachePool,
        CACHE_SIZE_T        Size,
        Cache_Operations   *Cache_Ops,
        PVOID               Extend,
        const char         *name
    );

VOID
    DestroyCachePool (
        PCACHE_POOL CachePool
    );

BOOLEAN
    QueryAndCopyFromCachePool (
        PCACHE_POOL CachePool,
        PUCHAR      Buf,
        OFFSET_T    Offset,
        LENGTH_T    Length
    );

BOOLEAN
    QueryAndWriteToCachePool (
        PCACHE_POOL CachePool,
        PUCHAR      Buf,
        OFFSET_T    Offset,
        LENGTH_T    Length
    );

VOID
    ReadUpdateCachePool (
        PCACHE_POOL CachePool,
        PUCHAR      Buf,
        OFFSET_T    Offset,
        LENGTH_T    Length
    );

VOID
    WriteUpdateCachePool (
        PCACHE_POOL CachePool,
        PUCHAR      Buf,
        OFFSET_T    Offset,
        LENGTH_T    Length
    );

PCACHE_BLOCK __GetFreeBlock (PCACHE_POOL CachePool);

/**
 * Internal Functions Used by Common Function
 * Implemented by special replacement algorithm
 */
typedef struct _Cache_Operations
{
    BOOLEAN      (*Init) (PVOID);
    VOID         (*Destroy) (PVOID);
    PCACHE_BLOCK (*AddNewBlockToPool) (PCACHE_POOL CachePool, CACHE_INDEX_T Index, PVOID Data, BOOLEAN Modified);
    VOID         (*DeleteOneBlockFromPool) (PCACHE_POOL CachePool, CACHE_INDEX_T Index);
    BOOLEAN      (*QueryPoolByIndex) (PCACHE_POOL CachePool, CACHE_INDEX_T Index, PCACHE_BLOCK *ppBlock);
    PCACHE_BLOCK (*FindBlockToReplace) (PCACHE_POOL CachePool, CACHE_INDEX_T Index, PVOID Data, BOOLEAN Modified);
    VOID         (*IncreaseBlockReference) (PCACHE_POOL CachePool, PCACHE_BLOCK pBlock);
    BOOLEAN      (*IsFull) (PCACHE_POOL CachePool);
    VOID         (*ForEachCacheBlock) (PCACHE_POOL CachePool, VOID(*Func)(PCACHE_POOL, PCACHE_BLOCK));
}Cache_Operations;

/*      Off
         |<-----------Len------------>|
    +----|----+-------~~-------+------|--+
    | fo | fs |  whole blocks  |  ec  |  |
    +----|----+-------~~-------+------|--+
    fo: front_offset
    fs: front_skip
    ec: end_cut
    front_broken = (fs != 0)
    end_broken = (ec != 0)
*/
#define detect_broken(Off,Len,front_broken,end_broken,          \
                      front_offset,front_skip,end_cut)          \
    do{                                                         \
        front_broken = FALSE;                                   \
        end_broken = FALSE;                                     \
        front_offset = 0;                                       \
        front_skip = 0;                                         \
        end_cut = 0;                                            \
        front_offset = Off % BLOCK_SIZE;                        \
        if(front_offset != 0)                                   \
        {                                                       \
            front_broken = TRUE;                                \
            front_skip = BLOCK_SIZE - front_offset;             \
            Off += front_skip;                                  \
            front_skip = (Len>front_skip) ? front_skip : Len;   \
            Len = Len - front_skip;                             \
        }                                                       \
        if(Len % BLOCK_SIZE != 0)                               \
        {                                                       \
            end_broken = TRUE;                                  \
            end_cut = Len % BLOCK_SIZE;                         \
            Len = Len - end_cut;                                \
        }                                                       \
    }while(0);

#endif
