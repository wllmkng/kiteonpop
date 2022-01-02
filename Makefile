TOPDIR = ../..
EXTRA_CFLAGS = -shared -g -O2 -Wall -I../lib
EXTRA_LDFLAGS = -L../lib -lapps -L. -lcppfuncs -lstdc++
SRCS = main.c
PROG = kite.so

include ${TOPDIR}/Makefile.comm