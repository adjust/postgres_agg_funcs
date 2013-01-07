require 'formula'
class PostgresAggFuncs < Formula
  head 'git@github.com:adeven/postgres_agg_funcs.git',:using => :git, :branch => 'master'
  url 'git@github.com:adeven/postgres_agg_funcs.git', :using => :git, :tag => 'v0.0.3'
  homepage 'https://github.com/adeven/postgres_agg_funcs'
  depends_on 'postgresql'
  depends_on 'ossp-uuid'
  version 'v0.0.3'

  def install

    (lib/'adjust').install compile("count.c avltree.c", "count.so")

    (lib/'adjust').install compile("add.c")

    (lib/'adjust').install compile("uniq.c")
  end


  def caveats;
    <<-EOS.undent
     create the functions with:

     CREATE OR REPLACE FUNCTION array_count(anyarray)
     RETURNS hstore AS
     '/usr/local/lib/adjust/count.so', 'welle_count' LANGUAGE C IMMUTABLE;

    CREATE OR REPLACE FUNCTION hstore_add(a hstore, b hstore)
     RETURNS hstore AS
     '/usr/local/lib/adjust/add.so', 'welle_add' LANGUAGE C IMMUTABLE;

    CREATE OR REPLACE FUNCTION array_uniq(integer[])
         RETURNS integer[] AS
         '/usr/local/lib/adjust/uniq.so', 'roa_uniq' LANGUAGE C IMMUTABLE;
    EOS
  end


  def compile(srcfile, sofile = nil)

    ofile  = srcfile.gsub('.c', '.o')
    sofile ||= srcfile.gsub('.c', '.so')

    xml2_ing          = "/usr/include/libxml2/"
    ossp_inc          = (ossp_uuid/'include')
    psql_server_inc   = (postgresql/'include/server/')
    psql_internal_inc = (postgresql/'include/internal/')
    psql_lib_dir      = (postgresql/'lib')
    ossp_lib_dir      = (ossp_uuid/'lib')
    psql_bin          = "/usr/local/bin/postgres"

    warnflags = "-Wall -Wmissing-prototypes -Wpointer-arith -Wdeclaration-after-statement -Wendif-labels \
                 -Wmissing-format-attribute -Wformat-security".gsub(/[ \n]+/, ' ').strip
    fflags    = "-fno-strict-aliasing -fwrapv"


    system "cc -I #{ossp_inc} -I #{psql_server_inc} -I #{psql_internal_inc} -I #{xml2_ing} -I . #{warnflags} #{fflags} -c #{srcfile}"
    system "cc -I #{ossp_inc} #{warnflags} #{fflags} -L #{psql_lib_dir} -L #{ossp_lib_dir} -Wl,-dead_strip_dylibs -bundle -bundle_loader #{psql_bin} -o #{sofile} #{ofile}"

    Dir[sofile]
  end

  def postgresql
    # Follow the PostgreSQL linked keg back to the active Postgres installation
    # as it is common for people to avoid upgrading Postgres.
    Formula.factory('postgresql').linked_keg.realpath
  end

  def ossp_uuid
    Formula.factory('ossp-uuid').linked_keg.realpath
  end
end
