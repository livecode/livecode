#!/bin/sh

set -e
set -x

GENTLE=../../gentle/gentle
GRTS=../../gentle/grts.o
REFLEX=../../reflex/reflex
LIB=../../lib
LEX=lex
YACC=yacc
CC=cc

$GENTLE virtual.g
$REFLEX
$LEX gen.l
$YACC gen.y

$CC -c y.tab.c
$CC -c lex.yy.c
$CC -c virtual.c
$CC -c machine.c

$CC -o virtual \
   virtual.o machine.o \
   y.tab.o lex.yy.o \
   $LIB/idents.o $LIB/errmsg.o $LIB/output.o $LIB/main.o \
   $GRTS

virtual testfile
