#CONFIG
APPNAME=kv_store
CSRC=kv_store.c 
IS_LIB=yes

OBJDIR=obj
SRCDIR=src

CC=gcc
INCLUDES=./ ./inc ../string/inc
CFLAGS=-std=gnu9x -O0 -g
LIBS=

include ../../tools/default.make

$(APPNAME)-test: $(addprefix $(SRCDIR)/,$(CSRC))
	$(CC) $(CFLAGS) -DKV_STORE_TEST $^ ../string/obj/astring.o -o $@ $(LIBS) 
