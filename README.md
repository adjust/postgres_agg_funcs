# postgres_agg_funcs

This implements some aggregation helper functions for postgres hstore written in C.
This is basically a rewrite in C of the SQL implementations described in
http://big-elephants.com/2012-10/aggregations-with-pg-hstore/

## array_count

```SQL

Select array_count(Array['foo','bar','foo','baz'])
 -- => "bar"=>"1", "baz"=>"1", "foo"=>"2"
```

## hstore_add

```SQL
Select hstore_add('a=>1,b=>2'::hstore ,'a=>1,c=>2'::hstore)
-- => "a"=>"2", "b"=>"2", "c"=>"2"
```
## array_uniq

```SQL
Select array_uniq(Array[1,5,9,4,1,9])
-- => "{1,4,5,9}"
```


## Build:

run build.sh <DBNAME>


## Mac OS:

If you are on Mac OS using homebrew you can use the included Formular postgres_agg_funcs.rb
Otherwise use and adapt the build_mac.sh script to your needs.


