MODULE_big = lib/count
OBJS = src/count.o src/avltree.o

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
