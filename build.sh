#!/bin/bash

INCLUDEDIR=`pg_config --includedir-server`
LIBDIR=`pg_config --libdir`
WORKDIR=`pwd`

rm $WORKDIR/*.o $WORKDIR/*.so
gcc -I $INCLUDEDIR -fpic -c count.c avltree.c -I $LIBDIR -lhstore -O2 -march=native
gcc -shared -o count.so count.o avltree.o

gcc -I $INCLUDEDIR -fpic -c add.c -I $LIBDIR -lhstore -O2 -march=native
gcc -shared -o add.so add.o

gcc -I $INCLUDEDIR -fpic -c uniq.c -I $LIBDIR -O2 -march=native
gcc -shared -o uniq.so uniq.o
