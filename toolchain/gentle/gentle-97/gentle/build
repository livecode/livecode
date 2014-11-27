#!/bin/sh

set -e
set -x

CC=cc
CFLAGS=

$CC $CFLAGS -c cyfront.c
$CC $CFLAGS -c main.c
$CC $CFLAGS -c input.c
$CC $CFLAGS -c symtab.c
$CC $CFLAGS -c msg.c
$CC $CFLAGS -c output.c
$CC $CFLAGS -c yytab.c
$CC $CFLAGS -c grts.c

$CC $CFLAGS -o gentle \
   cyfront.o main.o input.o symtab.o msg.o output.o yytab.o grts.o
