#!/bin/sh
the_test abc
the_test "a'c"
the_test 'a"c'
the_test a\'\"c
the_test \"\'a\'b\"\"
the_test '!@#$%^&*()_+-=~`\|'\''";:/?.>,<[]{}'
the_test ']]>'
