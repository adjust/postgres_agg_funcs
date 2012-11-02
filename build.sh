#!/bin/bash
if [ ! $# -eq 1 ]; then
    echo "usage: ./build.sh <dbname>"
    exit 0 
fi

DB=$1
INCLUDEDIR=`pg_config --includedir-server`
LIBDIR=`pg_config --libdir`
WORKDIR=`pwd`
gcc -I $INCLUDEDIR -fpic -c count.c -I $LIBDIR -lhstore -O3 -march=native
gcc -shared -o count.so count.o
psql -c "CREATE OR REPLACE FUNCTION roa_agg(anyarray) RETURNS hstore AS '$WORKDIR/count.so' LANGUAGE C;" -U postgres -h localhost $DB
psql -U postgres -h localhost $DB
