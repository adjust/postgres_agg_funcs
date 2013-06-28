CREATE OR REPLACE FUNCTION welle_count(anyarray) RETURNS hstore AS '/usr/local/lib/adjust/count.so' LANGUAGE C;
CREATE OR REPLACE FUNCTION welle_add(a hstore, b hstore) RETURNS hstore AS '/usr/local/lib/adjust/add.so' LANGUAGE C;
DROP AGGREGATE IF EXISTS welle_sum(hstore);
CREATE AGGREGATE welle_sum ( sfunc = welle_add, basetype = hstore, stype = hstore, initcond = '');
CREATE OR REPLACE FUNCTION roa_uniq(integer[]) RETURNS integer[] AS '/usr/local/lib/adjust/uniq.so' LANGUAGE C;
