#ifndef _AvlTree_H
#define _AvlTree_H

struct AvlNode;
typedef struct AvlNode *Position;
typedef struct AvlNode *AvlTree;

AvlTree makeEmpty( AvlTree t );
Position find( char * key, int keylen, AvlTree t );
AvlTree insert( char * key, int keylen, int value, AvlTree t );
int value( Position p );

#endif
