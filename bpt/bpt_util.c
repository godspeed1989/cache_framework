#include "bpt.h"

KEY_T CUT( KEY_T length )
{
    if (length % 2 == 0)
        return length/2;
    else
        return length/2 + 1;
}

/* give the height of the tree, which length in number of edges */
KEY_T height( node * root )
{
    KEY_T height = 0;
    node * c = root;
    while (c->is_leaf == FALSE)
    {
        c = (node *)c->pointers[0];
        height++;
    }
    return height;
}

node * Make_Node( void )
{
    node * new_node;

    new_node = (node*)MALLOC(sizeof(node));
    assert(new_node);
    // keys: order - 1
    new_node->keys = (KEY_T*)MALLOC( (order - 0) * sizeof(KEY_T) );
    assert(new_node->keys);
    // pointers: order
    new_node->pointers = (void*)MALLOC( order * sizeof(void *) );
    assert(new_node->pointers);

    new_node->is_leaf = FALSE;
    new_node->num_keys = 0;
    new_node->parent = NULL;
    new_node->next = NULL;
    return new_node;
}

void Free_Node( node * n )
{
    if(n)
    {
        FREE(n->pointers);
        FREE(n->keys);
        FREE(n);
    }
}

record * Make_Record( VAL_T value )
{
    record * new_record = (record *)MALLOC(sizeof(record));
    assert(new_record);
    new_record->value = value;
    return new_record;
}

void Free_Record( record * r )
{
    if(r)
    {
        FREE(r);
    }
}
