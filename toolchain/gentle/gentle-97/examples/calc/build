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

$GENTLE calc.g

$REFLEX

$LEX gen.l

$YACC gen.y

$CC -o calc \
   calc.c \
   lex.yy.c \
   y.tab.c \
   $LIB/errmsg.o \
   $LIB/main.o \
   $GRTS

calc testfile
