WORKDIR = $(shell pwd)

CC = gcc
AR = ar
LD = gcc -shared

CFLAGS = -fpic
LIB = -lhstore
LIBDIR = -I$(shell pg_config --libdir)
INC = -I$(shell pg_config --includedir-server)
INSTALL_DIR = /usr/local/lib/adjust

CFLAGS_DEBUG = $(CFLAGS) -g
LIB_DEBUG = $(LIB)
LIBDIR_DEBUG = $(LIBDIR)
INC_DEBUG = $(INC)
OBJDIR_DEBUG = obj/Debug
OUT_DEBUG = lib/Debug

CFLAGS_RELEASE = $(CFLAGS) -O2 -march=native
LIB_RELEASE = $(LIB)
LIBDIR_RELEASE = $(LIBDIR)
INC_RELEASE = $(INC)
OBJDIR_RELEASE = obj/Release
OUT_RELEASE = lib/Release

OBJ_DEBUG_COUNT = $(OBJDIR_DEBUG)/count.o $(OBJDIR_DEBUG)/avltree.o
OBJ_DEBUG_ADD = $(OBJDIR_DEBUG)/add.o
OBJ_DEBUG_UNIQ = $(OBJDIR_DEBUG)/uniq.o

OBJ_RELEASE_COUNT = $(OBJDIR_RELEASE)/count.o $(OBJDIR_RELEASE)/avltree.o
OBJ_RELEASE_ADD = $(OBJDIR_RELEASE)/add.o
OBJ_RELEASE_UNIQ = $(OBJDIR_RELEASE)/uniq.o

all: debug release

clean: clean_debug clean_release
	rm -rf obj
	rm -rf lib

install:
	test -d $(INSTALL_DIR) || mkdir -p $(INSTALL_DIR)
	cp $(OUT_RELEASE)/count.so $(INSTALL_DIR)
	cp $(OUT_RELEASE)/add.so $(INSTALL_DIR)
	cp $(OUT_RELEASE)/uniq.so $(INSTALL_DIR)

before_debug:
	test -d lib/Debug || mkdir -p lib/Debug
	test -d $(OBJDIR_DEBUG) || mkdir -p $(OBJDIR_DEBUG)

before_release:
	test -d lib/Release || mkdir -p lib/Release
	test -d $(OBJDIR_Release) || mkdir -p $(OBJDIR_Release)

after_debug:

after_release:

debug: before_debug out_debug_count out_debug_add out_debug_uniq after_debug

release: before_release out_release_count out_release_add out_release_uniq after_release

out_debug_count: $(OBJ_DEBUG_COUNT)
	$(LD) $(OBJ_DEBUG_COUNT) -o $(OUT_DEBUG)/count.so

out_debug_add: $(OBJ_DEBUG_ADD)
	$(LD) $(OBJ_DEBUG_ADD) -o $(OUT_DEBUG)/add.so

out_debug_uniq: $(OBJ_DEBUG_UNIQ)
	$(LD) $(OBJ_DEBUG_UNIQ) -o $(OUT_DEBUG)/uniq.so

out_release_count: $(OBJ_RELEASE_COUNT)
	$(LD) $(OBJ_RELEASE_COUNT) -o $(OUT_RELEASE)/count.so

out_release_add: $(OBJ_RELEASE_ADD)
	$(LD) $(OBJ_RELEASE_ADD) -o $(OUT_RELEASE)/add.so

out_release_uniq: $(OBJ_RELEASE_UNIQ)
	$(LD) $(OBJ_RELEASE_UNIQ) -o $(OUT_RELEASE)/uniq.so


$(OBJDIR_DEBUG)/count.o: count.c
	$(CC) $(CFLAGS_DEBUG) $(INC_DEBUG) -c count.c -o $(OBJDIR_DEBUG)/count.o

$(OBJDIR_DEBUG)/avltree.o: avltree.c
	$(CC) $(CFLAGS_DEBUG) $(INC_DEBUG) -c avltree.c -o $(OBJDIR_DEBUG)/avltree.o

$(OBJDIR_DEBUG)/add.o: add.c
	$(CC) $(CFLAGS_DEBUG) $(INC_DEBUG) -c add.c -o $(OBJDIR_DEBUG)/add.o

$(OBJDIR_DEBUG)/uniq.o: uniq.c
	$(CC) $(CFLAGS_DEBUG) $(INC_DEBUG) -c uniq.c -o $(OBJDIR_DEBUG)/uniq.o

$(OBJDIR_RELEASE)/count.o: count.c
	$(CC) $(CFLAGS_RELEASE) $(INC_RELEASE) -c count.c -o $(OBJDIR_RELEASE)/count.o

$(OBJDIR_RELEASE)/avltree.o: avltree.c
	$(CC) $(CFLAGS_RELEASE) $(INC_RELEASE) -c avltree.c -o $(OBJDIR_RELEASE)/avltree.o

$(OBJDIR_RELEASE)/add.o: add.c
	$(CC) $(CFLAGS_RELEASE) $(INC_RELEASE) -c add.c -o $(OBJDIR_RELEASE)/add.o

$(OBJDIR_RELEASE)/uniq.o: uniq.c
	$(CC) $(CFLAGS_RELEASE) $(INC_RELEASE) -c uniq.c -o $(OBJDIR_RELEASE)/uniq.o

clean_debug:
	rm -rf $(OBJ_DEBUG) $(OUT_DEBUG)
	rm -rf lib/Debug
	rm -rf $(OBJDIR_DEBUG)

clean_release:
	rm -rf $(OBJ_RELEASE) $(OUT_RELEASE)
	rm -rf lib/Debug
	rm -rf $(OBJDIR_RELEASE)
