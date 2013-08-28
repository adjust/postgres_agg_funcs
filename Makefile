all:
	test -d lib || mkdir lib
	make -f src/count.mk
	make -f src/add.mk
	make -f src/uniq.mk

clean:
	make -f src/count.mk clean
	make -f src/add.mk clean
	make -f src/uniq.mk clean
	rm -rf lib

install:
	test -d /usr/local/lib/adjust || mkdir -p /usr/local/lib/adjust
	cp lib/* /usr/local/lib/adjust/
