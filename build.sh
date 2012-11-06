#!/bin/bash
if [ ! $# -eq 1 ]; then
    echo "usage: ./build.sh <dbname>"
    exit 0 
fi

DB=$1
INCLUDEDIR=`pg_config --includedir-server`
LIBDIR=`pg_config --libdir`
WORKDIR=`pwd`

gcc -I $INCLUDEDIR -fpic -c count.c avltree.c -I $LIBDIR -lhstore -O3 -march=native 
gcc -shared -o count.so count.o avltree.o
psql -c "CREATE OR REPLACE FUNCTION welle_count(anyarray) RETURNS hstore AS '$WORKDIR/count.so' LANGUAGE C;" -U postgres -h localhost $DB

gcc -I $INCLUDEDIR -fpic -c add.c -I $LIBDIR -lhstore -O3 -march=native 
gcc -shared -o add.so add.o
psql -c "CREATE OR REPLACE FUNCTION welle_add(a hstore, b hstore) RETURNS hstore AS '$WORKDIR/add.so' LANGUAGE C;" -U postgres -h localhost $DB
psql -U postgres -h localhost $DB
