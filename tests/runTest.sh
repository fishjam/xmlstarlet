#!/bin/sh

abs_builddir=$PWD
export abs_builddir

TEST=`basename $1`
cd "$srcdir"/examples || exit 77

srcdir=.. $SHELL ./$TEST | tr -d "\r" \
    | diff -u - results/$TEST.out
