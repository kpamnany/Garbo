# loren -- threading runtime
#
# makefile for tests
#
# 2016.06.01   kiran.pamnany   Initial code
#

CC=mpiicc

.SUFFIXES: .c .h .o
.PHONY: clean

CFLAGS+=-Wall
CFLAGS+=-std=c11
CFLAGS+=-D_GNU_SOURCE
CFLAGS+=-mkl
CFLAGS+=-I.
CFLAGS+=-I../include
CFLAGS+=-I../src

LDOBJS+=-lpthread
LDOBJS+=../libgarbo.so

SRCS=garraytest.c dtreetest.c
OBJS=$(subst .c,.o, $(SRCS))
BINS=$(subst .c,, $(SRCS))

ifeq ($(DEBUG),yes)
    CFLAGS+=-O0 -g
else
    CFLAGS+=-O3
endif

all: $(BINS)

$(BINS): $(SRCS) ../libgarbo.so
	$(CC) $(CFLAGS) garraytest.c -o garraytest $(LDOBJS)
	$(CC) $(CFLAGS) -qopenmp dtreetest.c -o dtreetest $(LDOBJS)

clean:
	$(RM) -f $(BINS) $(OBJS)

