#ifndef __SHARE_H__
#define __SHARE_H__

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define MALLOC(n)               malloc(n)
#define FREE(p)                 { if(p) free(p); }
#define ZERO(p,len)             memset(p,0,len)
#define ASSERT(expr)            assert(expr)
#define MEMCPY(dst,src,len)     memcpy(dst,src,len)

#define SECTOR_SIZE             ((unsigned long)512)
#define NSpB                    ((unsigned long)8) // Number Sectors per Block
#define BLOCK_SIZE              (SECTOR_SIZE*NSpB)

typedef unsigned long           CACHE_SIZE_T;
typedef unsigned long           CACHE_INDEX_T;
typedef unsigned long           OFFSET_T;
typedef unsigned int            LENGTH_T;

typedef void                    VOID;
typedef void*                   PVOID;
typedef unsigned char           UCHAR;
typedef unsigned char*          PUCHAR;
typedef unsigned long           ULONG;
typedef unsigned long*          PULONG;
typedef unsigned char           BOOLEAN;
typedef unsigned char*          PBOOLEAN;
typedef unsigned long long      ULONGLONG;
typedef unsigned long long*     PULONGLONG;

#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif

#define container_of(ptr, type, member) \
        ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

#endif
