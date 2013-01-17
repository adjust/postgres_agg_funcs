CREATE OR REPLACE FUNCTION welle_count(anyarray) RETURNS hstore AS '$WORKDIR/count.so' LANGUAGE C;
CREATE OR REPLACE FUNCTION welle_add(a hstore, b hstore) RETURNS hstore AS '$WORKDIR/add.so' LANGUAGE C;
DROP AGGREGATE IF EXISTS welle_sum(hstore);
CREATE AGGREGATE welle_sum ( sfunc = welle_add, basetype = hstore, stype = hstore, initcond = '');
CREATE OR REPLACE FUNCTION roa_uniq(integer[]) RETURNS integer[] AS '$WORKDIR/uniq.so' LANGUAGE C;
