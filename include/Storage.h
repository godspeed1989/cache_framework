#ifndef __STORAGE_H__
#define __STORAGE_H__

#include "Share.h"

/**
 * StoragePool Stub Function
 */

typedef unsigned long STORAGE_INDEX_T;

typedef struct _STORAGE_POOL
{
    STORAGE_INDEX_T     Size;
    STORAGE_INDEX_T     Used;
    ULONGLONG           TotalSize;

    PUCHAR              Buffer;
    PUCHAR              Bitmap_Buffer;
    // Opaque
    STORAGE_INDEX_T     HintIndex;
}STORAGE_POOL, *PSTORAGE_POOL;

BOOLEAN
InitStoragePool (PSTORAGE_POOL StoragePool, STORAGE_INDEX_T Size);

VOID
DestroyStoragePool (PSTORAGE_POOL StoragePool);

STORAGE_INDEX_T
StoragePoolAlloc (PSTORAGE_POOL StoragePool);

VOID
StoragePoolFree (PSTORAGE_POOL StoragePool, STORAGE_INDEX_T Index);

VOID
StoragePoolWrite (PSTORAGE_POOL StoragePool,
                  STORAGE_INDEX_T StartIndex, ULONG Offset,
                  const PVOID Data, ULONG Len);

VOID
StoragePoolRead (PSTORAGE_POOL StoragePool, PVOID Data,
                 STORAGE_INDEX_T StartIndex, ULONG Offset, ULONG Len);

#endif
