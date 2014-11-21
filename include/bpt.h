#ifndef __BPT_H__
#define __BPT_H__

#include "Cache.h"

#define SUPPORT_PRINT

#define KEY_T           CACHE_INDEX_T
#define VAL_T           PCACHE_BLOCK
#define order           (KEY_T)(16)

#ifndef BOOL
#define BOOL    int
#endif

typedef struct _node
{
    // node: array of nodes corresponding to keys
    // leaf: with a maximum of order-1 key-pointer
    //       The last pointer points to the right leaf
    void             ** pointers;
    KEY_T             * keys;
    KEY_T               num_keys;
    BOOL                is_leaf;
    struct _node      * parent;
    struct _node      * next;
} node;

typedef struct _record
{
    VAL_T               value;
} record;

KEY_T       CUT ( KEY_T length );

node *      Make_Node ( void );
void        Free_Node ( node * n );
record *    Make_Record ( VAL_T value );
void        Free_Record ( record * n );

node *      Insert ( node * root, KEY_T key, VAL_T value );
node *      Delete ( node * root, KEY_T key );

node *      Find_Leaf ( node * root, KEY_T key );
record *    Find_Record ( node * root, KEY_T key );
node*       Get_Leftmost_Leaf(node *root);
node *      Destroy_Tree ( node * root );

#ifdef SUPPORT_PRINT
void        Print_To_File( node * root );
#endif

#endif
