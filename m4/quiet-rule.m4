# serial 1 -*- mode: autoconf -*-
# SILENT_RULE(varname, [echo = varname])
# defines V_varname to use for silent make rules
AC_DEFUN([np_SILENT_RULE],[dnl
m4_pushdef([varname], $1)dnl
m4_pushdef([echoname], [m4_default($2, varname)])dnl
# make silent rule for $1
AC_SUBST(V_[]varname,  ["\$(V_[]varname[]_$AM_V)"])dnl
AC_SUBST(V_[]varname[]_, ["\$(V_[]varname[]_$AM_DEFAULT_VERBOSITY)"])dnl
AC_SUBST(V_[]varname[]_0, ['@echo "  echoname	 [$]@";'])dnl
m4_popdef([varname], [echoname])dnl
])
