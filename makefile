# garbo -- global arrays + dtree
#
# makefile
#
# 2018.06.01   kiran.pamnany   Initial code
#

CC=mpiicc

.SUFFIXES: .c .h .o .a
.PHONY: clean test

CFLAGS+=-Wall
CFLAGS+=-std=c11
CFLAGS+=-D_GNU_SOURCE
CFLAGS+=-fpic
CFLAGS+=-I./include
CFLAGS+=-I./src

SRCS=src/garbo.c src/garray.c src/dtree.c src/log.c
OBJS=$(subst .c,.o, $(SRCS))

ifeq ($(DEBUG),yes)
    CFLAGS+=-O0 -g
else
    CFLAGS+=-O2
endif

TARGET=libgarbo.so

all: $(TARGET)

test: $(TARGET)
	$(MAKE) -C test

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -shared -o $(TARGET) $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(MAKE) -C test clean
	$(RM) -f $(TARGET) $(OBJS)

