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
psql -U postgres -h localhost $DB -c "CREATE OR REPLACE FUNCTION welle_count(anyarray) RETURNS hstore AS '$WORKDIR/count.so' LANGUAGE C;"

gcc -I $INCLUDEDIR -fpic -c add.c -I $LIBDIR -lhstore -O3 -march=native
gcc -shared -o add.so add.o
psql -U postgres -h localhost $DB -c "CREATE OR REPLACE FUNCTION welle_add(a hstore, b hstore) RETURNS hstore AS '$WORKDIR/add.so' LANGUAGE C;"
# use the following line to create the aggregate welle_sum
# would be preferable to use something along the lines of "CREATE OR REPLACE AGGREGATE"
# psql -U postgres -h localhost $DB -c "CREATE AGGREGATE welle_sum ( sfunc = welle_add, basetype = hstore, stype = hstore, initcond = '');"
psql -U postgres -h localhost $DB
