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

$GENTLE polish.g

$REFLEX

$LEX gen.l

$YACC gen.y

$CC -o polish \
   polish.c \
   lex.yy.c \
   y.tab.c \
   $LIB/errmsg.o \
   $LIB/main.o \
   $GRTS

polish testfile
