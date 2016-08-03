# garbo -- global arrays
#
# makefile
#
# 2018.06.01   kiran.pamnany   Initial code
#

CC=mpicc

.SUFFIXES: .c .h .o .a
.PHONY: clean test

CFLAGS+=-Wall
CFLAGS+=-std=c11
CFLAGS+=-D_GNU_SOURCE

CFLAGS+=-I./include
CFLAGS+=-I./src

SRCS=src/garbo.c
OBJS=$(subst .c,.o, $(SRCS))

ifeq ($(DEBUG),yes)
    CFLAGS+=-O0 -g
else
    CFLAGS+=-O3
endif

TARGET=libgarbo.a

all: $(TARGET)

test: $(TARGET)
	$(MAKE) -C test

$(TARGET): $(OBJS)
	$(RM) -f $(TARGET)
	$(AR) qvs $(TARGET) $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(MAKE) -C test clean
	$(RM) -f $(TARGET) $(OBJS)

