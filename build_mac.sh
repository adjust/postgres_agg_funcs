#!/bin/bash
if [ ! $# -eq 1 ]; then
    echo "usage: ./build.sh <dbname>"
    exit 0
fi

DB=$1
WORKDIR=`pwd`

XML2_INC="/usr/include/libxml2/"
OSSP_INC="/usr/local/Cellar/ossp-uuid/1.6.2/include"
PSQL_SERVER_INC="/usr/local/Cellar/postgresql/9.2.1/include/server/"
PSQL_INTERNAL_INC="/usr/local/Cellar/postgresql/9.2.1/include/internal/"
PSQL_LIB_DIR="/usr/local/Cellar/postgresql/9.2.1/lib/"
OSSP_LIB_DIR="/usr/local/Cellar/ossp-uuid/1.6.2/lib"
PSQL_BIN="/usr/local/bin/postgres"

WARNFLAGS="-Wall \
           -Wmissing-prototypes \
           -Wpointer-arith \
           -Wdeclaration-after-statement \
           -Wendif-labels \
           -Wmissing-format-attribute \
           -Wformat-security"
FFLAGS="-fno-strict-aliasing \
        -fwrapv"

i="count.c avltree.c"
OFILE=${i//".c"/".o"}
SOFILE="count.so"
cc -I $OSSP_INC -I $PSQL_SERVER_INC -I $PSQL_INTERNAL_INC -I $XML2_INC -I $WORKDIR \
    $WARNFLAGS $FFLAGS -c $i

cc -I $OSSP_INC $WARNFLAGS $FFLAGS -L $PSQL_LIB_DIR -L $OSSP_LIB_DIR \
   -Wl,-dead_strip_dylibs \
   -bundle -bundle_loader \
   $PSQL_BIN -o $SOFILE $OFILE

i="add.c"
OFILE=${i/".c"/".o"}
SOFILE=${i/".c"/".so"}
cc -O3 -I $OSSP_INC -I $PSQL_SERVER_INC -I $PSQL_INTERNAL_INC -I $XML2_INC -I $WORKDIR \
    $WARNFLAGS $FFLAGS -c -o $OFILE $i

cc -I $OSSP_INC $WARNFLAGS $FFLAGS -L $PSQL_LIB_DIR -L $OSSP_LIB_DIR \
   -Wl,-dead_strip_dylibs \
   -bundle -bundle_loader \
   $PSQL_BIN -o $SOFILE $OFILE

i="uniq.c"
OFILE=${i/".c"/".o"}
SOFILE=${i/".c"/".so"}
cc -I $OSSP_INC -I $PSQL_SERVER_INC -I $PSQL_INTERNAL_INC -I $XML2_INC -I $WORKDIR \
    $WARNFLAGS $FFLAGS -c -o $OFILE $i

cc -I $OSSP_INC $WARNFLAGS $FFLAGS -L $PSQL_LIB_DIR -L $OSSP_LIB_DIR \
   -Wl,-dead_strip_dylibs \
   -bundle -bundle_loader \
   $PSQL_BIN -o $SOFILE $OFILE

psql -h localhost $DB -c "CREATE OR REPLACE FUNCTION welle_count(anyarray) RETURNS hstore AS '$WORKDIR/count.so' LANGUAGE C;"
psql -h localhost $DB -c "CREATE OR REPLACE FUNCTION welle_add(a hstore, b hstore) RETURNS hstore AS '$WORKDIR/add.so' LANGUAGE C;"
psql -h localhost $DB -c "DROP AGGREGATE IF EXISTS welle_sum(hstore);"
psql -h localhost $DB -c "CREATE AGGREGATE welle_sum ( sfunc = welle_add, basetype = hstore, stype = hstore, initcond = '');"
psql -h localhost $DB -c "CREATE OR REPLACE FUNCTION roa_uniq(integer[]) RETURNS anyarray AS '$WORKDIR/uniq.so' LANGUAGE C;"
