#!/bin/sh
the_test "'abc'"
the_test \"a\'c\"
the_test \'a\"c\'
the_test 'concat("a'\''", '\'\"c\'\)
the_test 'concat('\'\"\',\ \"\'a\'b\",\ \'\"\"\'\)
the_test 'concat("!@#$%^&*()_+-=~`\|'\'\",\''";:/?.>,<[]{}'\'\)
the_test '"]]>"'
