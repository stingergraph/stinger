include ../../make.inc

PREFIX=../../src
OBJ_PREFIX=../../obj
STINGER_SRC=$(PREFIX)/stinger.c $(PREFIX)/stinger-deprecated.c $(PREFIX)/stinger-utils.c $(PREFIX)/timer.c $(PREFIX)/xmalloc.c
GENSTREAMS_SRC=../../genstreams
COMPAT_DIR=../../compat
INCLUDE_DIR=../../include
CFLAGS+=-I$(INCLUDE_DIR)

.PHONY:	all
all: main

ifdef USE_GETOPT_NETBSD
  GETOPT_SRC=$(GENSTREAMS_SRC)/getopt-netbsd/getopt_long.c
  CPPFLAGS+=-I$(GENSTREAMS_SRC)/getopt-netbsd
endif

ifdef FOR_XMT
  BLECHIO=xmt-luc-blech.o
  BLECHIOGEN=xmt-luc-blech-gen.o
  MAINPL=main.pl
  MAINPLFLAG=-pl $(MAINPL)
  GENSTREAMSPL=gen-streams.pl
  GENSTREAMSPLFLAG=-pl $(GENSTREAMSPL)
else
  STINGER_SRC+=$(OBJ_PREFIX)/x86-full-empty.o
  LDLIBS += -lstdc++
endif

COMM_SRC=community-update.c community.c graph-el.c sorts.c

main:	main.c $(COMM_SRC) $(STINGER_SRC) xmt-luc.o #$(BLECHIO)
	$(CC) $(MAINPLFLAG) $(CPPFLAGS) $(CFLAGS) -o $@ $^ \
		$(LDFLAGS) $(LDLIBS)

$(OBJ_PREFIX)/x86-full-empty.o:	$(PREFIX)/x86-full-empty.c
	$(CC) $(MAINPLFLAG) $(CPPFLAGS) $(subst O1,O0,$(subst O2,O0,$(subst O3,O0,$(CFLAGS)))) -c -o $(OBJ_PREFIX)/x86-full-empty.o $^

gen-streams:	$(GENSTREAMS_SRC)/gen-streams.c $(GENSTREAMS_SRC)/prng.c $(GENSTREAMS_SRC)/rmat.c $(PREFIX)/timer.c $(PREFIX)/xmalloc.c $(BLECHIOGEN) $(GETOPT_SRC)
	$(CC) $(GENSTREAMSPLFLAG) $(CPPFLAGS) $(CFLAGS) -o $@ $^ \
		$(LDFLAGS) $(LDLIBS)

xmt-luc-blech.o:	$(COMPAT_DIR)/xmt-luc-blech.cc
	$(CXX) $(MAINPLFLAG) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
xmt-luc-blech-gen.o:	$(COMPAT_DIR)/xmt-luc-blech.cc
	$(CXX) $(GENSTREAMPLFLAG) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

xmt-luc.o:	xmt-luc.cc xmt-luc.h

.PHONY:	clean
clean:
	rm -f main gen-streams $(BLECHIO) $(BLECHIOGEN) \
		$(MAINPL) $(GENSTREAMSPL)
