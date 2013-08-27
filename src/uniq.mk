MODULE_big = lib/uniq
OBJS = src/uniq.o

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
