#CONFIG
APPNAME=fmemopen
CSRC=fmemopen.c
IS_LIB=yes

OBJDIR=obj
SRCDIR=src

CC=gcc
INCLUDES=./ ./inc
CFLAGS=-std=gnu99 -O3
LIBS=

include ../../tools/default.make

$(APPNAME)-test: $(SRCDIR)/$(CSRC)
	$(CC) $(CFLAGS) -DINT_HT_SEQ_TEST $^ -o $@ $(LIBS)
