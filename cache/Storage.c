#include "Storage.h"

static UCHAR CheckBit(const PUCHAR p, STORAGE_INDEX_T Index)
{
    return p[Index/8] & (1 << (Index%8));
}

static VOID ReverseBit(const PUCHAR p, STORAGE_INDEX_T Index)
{
    p[Index/8] ^= (1 << Index%8);
}

static STORAGE_INDEX_T FindClearBitAndSet(const PUCHAR p, STORAGE_INDEX_T Size, STORAGE_INDEX_T HintIndex)
{
    STORAGE_INDEX_T cnt = 0;
    STORAGE_INDEX_T idx = HintIndex < Size ? HintIndex : 0;

    while (cnt < Size)
    {
        if (!CheckBit(p, idx))
        {
            ReverseBit(p, idx);
            return idx;
        }
        idx = (idx + 1) % Size;
        cnt++;
    }
    return (STORAGE_INDEX_T)-1;
}

/////////////////////////////////////////////////////////////////

BOOLEAN
InitStoragePool (PSTORAGE_POOL StoragePool, STORAGE_INDEX_T Size)
{
    ZERO(StoragePool, sizeof(STORAGE_POOL));
    StoragePool->Size = Size;
    StoragePool->TotalSize = Size * BLOCK_SIZE;

    StoragePool->Buffer = (PUCHAR)MALLOC(Size * BLOCK_SIZE);
    StoragePool->Bitmap_Buffer = (PUCHAR)MALLOC(Size/8 + 1);

    if (StoragePool->Buffer && StoragePool->Bitmap_Buffer)
    {
        ZERO(StoragePool->Buffer, Size * BLOCK_SIZE);
        ZERO(StoragePool->Bitmap_Buffer, Size/8 + 1);
        return TRUE;
    }
    FREE(StoragePool->Buffer);
    FREE(StoragePool->Bitmap_Buffer);
    ZERO(StoragePool, sizeof(STORAGE_POOL));
    return FALSE;
}

VOID
DestroyStoragePool (PSTORAGE_POOL StoragePool)
{
    FREE(StoragePool->Buffer);
    FREE(StoragePool->Bitmap_Buffer);
    ZERO(StoragePool, sizeof(STORAGE_POOL));
    return;
}

STORAGE_INDEX_T
StoragePoolAlloc (PSTORAGE_POOL StoragePool)
{
    STORAGE_INDEX_T Index;

    Index = FindClearBitAndSet(StoragePool->Bitmap_Buffer,
                StoragePool->Size, StoragePool->HintIndex);
    if (Index != -1)
    {
        StoragePool->Used++;
        StoragePool->HintIndex = Index;
    }
    else
    {
        StoragePool->HintIndex = 0;
    }
    return Index;
}

VOID
StoragePoolFree (PSTORAGE_POOL StoragePool, STORAGE_INDEX_T Index)
{
    if (CheckBit(StoragePool->Bitmap_Buffer, Index))
    {
        StoragePool->Used--;
        ReverseBit(StoragePool->Bitmap_Buffer, Index);
    }
}

VOID
StoragePoolWrite (PSTORAGE_POOL StoragePool,
                  STORAGE_INDEX_T StartIndex, ULONG Offset,
                  const PVOID Data, ULONG Len)
{
    ULONGLONG   writeOffset;
    writeOffset = StartIndex * BLOCK_SIZE + Offset;

    ASSERT((writeOffset + Len) <= StoragePool->TotalSize);
    MEMCPY(StoragePool->Buffer + writeOffset, Data, Len);
}

VOID
StoragePoolRead (PSTORAGE_POOL StoragePool, PVOID Data,
                 STORAGE_INDEX_T StartIndex, ULONG Offset, ULONG Len)
{
    ULONGLONG   readOffset;
    readOffset = StartIndex * BLOCK_SIZE + Offset;

    ASSERT((readOffset + Len) <= StoragePool->TotalSize);
    MEMCPY(Data, StoragePool->Buffer + readOffset, Len);
}
