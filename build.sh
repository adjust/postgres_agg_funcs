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
gcc -shared -o welle_count.so count.o avltree.o
psql -c "CREATE OR REPLACE FUNCTION welle_agg(anyarray) RETURNS hstore AS '$WORKDIR/welle_count.so' LANGUAGE C;" -U postgres -h localhost $DB

gcc -I $INCLUDEDIR -fpic -c add.c -I $LIBDIR -lhstore -O3 -march=native 
gcc -shared -o add.so add.o
psql -c "CREATE OR REPLACE FUNCTION roa_add(a hstore, b hstore) RETURNS hstore AS '$WORKDIR/add.so' LANGUAGE C;" -U postgres -h localhost $DB
psql -U postgres -h localhost $DB
