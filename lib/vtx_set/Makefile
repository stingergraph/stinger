#CONFIG
APPNAME=vtx_set
CSRC=vtx_set.c
IS_LIB=yes

OBJDIR=obj
SRCDIR=src

CC=gcc
INCLUDES=./ ./inc 
CFLAGS=-std=gnu99 -O3
LIBS=

include ../../tools/default.make

$(APPNAME)-test: $(addprefix $(SRCDIR)/,$(CSRC))
	$(CC) $(CFLAGS) -DINT_HT_SEQ_TEST $^ -o $@ $(LIBS) -lrt
