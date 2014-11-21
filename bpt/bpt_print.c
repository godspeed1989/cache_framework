#include "bpt.h"

#ifdef SUPPORT_PRINT
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static node * queue = NULL;

static
void enqueue( node * new_node )
{
    node * c;
    if (queue == NULL) {
        queue = new_node;
        queue->next = NULL;
    }
    else {
        c = queue;
        while(c->next != NULL) {
            c = c->next;
        }
        c->next = new_node;
        new_node->next = NULL;
    }
}

static
node * dequeue( void )
{
    node * n = queue;
    queue = queue->next;
    n->next = NULL;
    return n;
}

void Print_To_File( node * root )
{
    FILE *fp;
    time_t tim;
    struct tm *at;
    char filename[64];

    node * n = NULL;
    KEY_T i = 0;

    time(&tim);
    at = localtime(&tim);
    strftime(filename, 63, "%Y%m%d%H%M%S.dot", at);
    fp = fopen(filename, "w");
    if( fp == NULL ) {
        printf("open %s error\n", filename);
        return;
    }
    fprintf(fp, "digraph {\n");
    fprintf(fp, "graph[ordering=\"out\"];\n");
    fprintf(fp, "node[fontcolor=\"#990000\",shape=plaintext];\n");
    fprintf(fp, "edge[arrowsize=0.6,fontsize=6];\n");
    if( root == NULL ) {
        fprintf(fp, "null[shape=box]\n");
        return;
    }
    queue = NULL;
    enqueue(root);
    while ( queue != NULL ) {
        n = dequeue();
        fprintf(fp, "n%p[label=\"", n);
        for (i = 0; i < n->num_keys; i++) {
            fprintf(fp, " %lu ", n->keys[i]);
        }
        fprintf(fp, "\",shape=box];\n");
        if (n->is_leaf == FALSE) {
            for (i = 0; i <= n->num_keys; i++) {
                fprintf(fp, " n%p -> n%p;\n", n, n->pointers[i]);
                enqueue(n->pointers[i]);
            }
        }
        else {
            if(n->pointers[order - 1])
                fprintf(fp, " n%p -> n%p[constraint=FALSE];\n", n, n->pointers[order - 1]);
        }
    }
    fprintf(fp, "}\n");
    fclose(fp);
}
#endif
