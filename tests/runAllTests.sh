#!/bin/sh

# This script is a replacement for make check, for machines that don't
# have make installed.

TESTS='
bigxml-dtd
bigxml-embed-ref
bigxml-embed
bigxml-relaxng
bigxml-well-formed
bigxml-xsd
c14n-default-attr
c14n-newlines
c14n1
c14n2
command-help
count1
countnode1
delete1
dtd1
dtd2
dtd3
dtd4
ed-2op
ed-append
ed-backref-delete
ed-backref1
ed-backref2
ed-expr
ed-insert
ed-literal
ed-move
ed-namespace
ed-nop
ed-subnode
elem1
elem2
elem3
elem-depth
elem-uniq
escape1
exslt-ed
exslt1
external-entity
findfile1
genxml1
hello1
localname1
look1
move1
N-order
noindent1
ns1
pyx
pyx-ns
recover1
rename-attr1
rename-elem1
schema1
sel-literal
sel-if
sel-many-values
sel-root
sel-xpath-c
sel-xpath-i
sel-xpath-m
sel-xpath-v
sel1
sort1
sort2
sort3
structure1
sum1
tab1
table1
table2
table3
unicode1
update-attr1
update-elem1
valid1
xinclude1
xsl-param1
xsl-sum1'

XFAIL_TESTS='bigxml-dtd|ed-namespace'


testdir=`dirname $0`

: ${srcdir:=$testdir/..}
export srcdir

pass=0; xpass=0; fail=0; xfail=0; total=0

for t in $TESTS ; do
    $SHELL ./runTest.sh "$t" > /dev/null ; rc=$?
    echo "$t" | grep -E "$XFAIL_TESTS" ; xfail_test=$?

    echo -n "$t: "
    if [ $rc -eq 0 ] ; then
        if [ $xfail_test -eq 0 ] ; then
            echo BAD, XPASS
            xpass=`expr $xpass + 1`
        else
            echo GOOD, PASS
            pass=`expr $pass + 1`
        fi
    else
        if [ $xfail_test -eq 0 ] ; then
            echo GOOD, XFAIL
            xfail=`expr $xfail + 1`
        else
            echo BAD, FAIL
            fail=`expr $fail + 1`
        fi
    fi

    total=`expr $total + 1`
done

good=`expr $pass + $xfail`
bad=`expr $xpass + $fail`

echo "$pass passed, $xfail xfails ($good good)"
echo "$xpass xpassed, $fail fails ($bad bad)"
echo "$total total."
