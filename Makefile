MODULES = add count uniq
#EXTENSION = add
#DATA = isbn_issn--1.0.sql
#DOCS = README.isbn_issn

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
