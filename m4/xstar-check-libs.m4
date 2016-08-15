# serial 1 -*- mode: autoconf -*-

# XSTAR_LIB_ARGS_WITH(LIBXXX, [with-src = []])
dnl pass [with-src] to get the --with-libxxx-src option
AC_DEFUN([XSTAR_LIB_ARGS_WITH],
[m4_pushdef([LIBXXX], $1)dnl
m4_pushdef([libxxx], m4_tolower(LIBXXX))dnl
AC_ARG_WITH(libxxx-prefix,
        AS_HELP_STRING(--with-libxxx-prefix=[PFX],
                        Specify location of libxxx),
        [LIBXXX()_PREFIX="$withval"])
AC_ARG_WITH(libxxx-include-prefix,
        AS_HELP_STRING([--with-libxxx-include-prefix=[PFX]],
                        Specify location of libxxx headers),
        [LIBXXX()_INCDIR="$withval"])
AC_ARG_WITH(libxxx-libs-prefix,
        AS_HELP_STRING([--with-libxxx-libs-prefix=[PFX]],
                        Specify location of libxxx libs),
        [LIBXXX()_LIBDIR="$withval"])
m4_if($2, [with-src],
[AC_ARG_WITH(libxxx-src,
        AS_HELP_STRING([--with-libxxx-src=[DIR]],
                        For libxxx that's not installed yet (sets all three above)),
        [LIBXXX()_SRCDIR="$withval"
         LIBXXX()_LIBDIR="$LIBXXX()_SRCDIR/.libs"])])
m4_popdef([libxxx], [LIBXXX])])

# XSTAR_LIB_CHECK(LIBXXX, xxx-config)
# set LIBXXX_INCDIR and LIBXXX_LIBDIR based on location of xxx-config
# also check xxx-config --version against LIBXXX_REQUIRED_VERSION
# Ignores xxx-config if LIBXXX_SRCDIR is set
AC_DEFUN([XSTAR_LIB_CHECK],
[m4_pushdef([LIBXXX], $1)
 m4_pushdef([libxxx], m4_tolower(LIBXXX))
 m4_pushdef([xxx_config], $2)
 AS_IF([test "x$LIBXXX()_SRCDIR" != x],
       [AC_MSG_NOTICE([using libxxx src dir "$LIBXXX()_SRCDIR"])
        AC_MSG_WARN([not checking libxxx version])],
 [AC_PATH_PROG(LIBXXX()_CONFIG, xxx_config(),
               [], [$LIBXXX()_PREFIX/bin$PATH_SEPARATOR$PATH])
  AS_IF([test "x$LIBXXX()_CONFIG" = x],
  [AC_MSG_FAILURE([xxx_config not found,
  libxxx is not installed or LIBXXX()_PREFIX is not correctly defined])])
  LIBXXX()_VERSION=$($LIBXXX()_CONFIG --version)
  AS_VERSION_COMPARE([$LIBXXX()_VERSION], [$LIBXXX()_REQUIRED_VERSION],
                     [AC_MSG_ERROR([xmlstarlet needs at least libxxx version $LIBXXX()_REQUIRED_VERSION (http://www.xmlsoft.org/)])])
  AC_MSG_NOTICE([using libxxx-$LIBXXX()_VERSION])
  LIBXXX()_PREFIX=`AS_DIRNAME($LIBXXX()_CONFIG)`
  LIBXXX()_PREFIX=`AS_DIRNAME($LIBXXX()_PREFIX)`
  : ${LIBXXX()_LIBDIR="$LIBXXX()_PREFIX/lib"}])
 m4_popdef([xxx_config], [libxxx], [LIBXXX])])
