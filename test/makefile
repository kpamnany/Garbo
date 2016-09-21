# loren -- threading runtime
#
# makefile for tests
#
# 2016.06.01   kiran.pamnany   Initial code
#

CC=cc

.SUFFIXES: .c .h .o
.PHONY: clean

CFLAGS+=-Wall
CFLAGS+=-std=c11
CFLAGS+=-D_GNU_SOURCE
CFLAGS+=-fpic
CFLAGS+=-I.
CFLAGS+=-I../include
CFLAGS+=-I../src
CFLAGS+=-I$(INCLUDE)

LDFLAGS+=-dynamic
LDFLAGS+=-L..
LDFLAGS+=-lgarbo
LDFLAGS+=-Wl,--whole-archive,-ldmapp,--no-whole-archive

LDFLAGS+=-L$(MKLROOT)/lib/intel64
LDFLAGS+=-lmkl_intel_ilp64
LDFLAGS+=-lmkl_core
LDFLAGS+=-lmkl_sequential
LDFLAGS+=-lpthread
LDFLAGS+=-lm
LDFLAGS+=-ldl

SRCS=garraytest.c dtreetest.c
OBJS=$(subst .c,.o, $(SRCS))
BINS=$(subst .c,, $(SRCS))

ifeq ($(DEBUG),yes)
    CFLAGS+=-O0 -g
else
    CFLAGS+=-O3
endif

all: $(BINS)

garraytest: $(SRCS) ../libgarbo.so
	$(CC) $(CFLAGS) garraytest.c -o garraytest $(LDFLAGS)

dtreetest: dtreetest.c ../libgarbo.so
	$(CC) $(CFLAGS) -openmp dtreetest.c -o dtreetest $(LDFLAGS)

clean:
	$(RM) -f $(BINS) $(OBJS)

