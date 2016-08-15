AC_PREREQ([2.62])
AC_INIT([XMLStarlet], [m4_esyscmd_s([git describe --tags --dirty])],
        [http://sourceforge.net/projects/xmlstar/support],
        [],
        [http://xmlstar.sourceforge.net/])
AC_REVISION(AC_PACKAGE_VERSION)

AM_INIT_AUTOMAKE([1.11 foreign -Wall subdir-objects no-define std-options parallel-tests color-tests])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE

[LIBXML_REQUIRED_VERSION=2.6.27]
[LIBXSLT_REQUIRED_VERSION=1.1.9]

XSTAR_LIB_ARGS_WITH([LIBXML], [with-src])
XSTAR_LIB_ARGS_WITH([LIBXSLT], [with-src])
XSTAR_LIB_ARGS_WITH([LIBICONV])

AC_ARG_ENABLE([static-libs],
        [AS_HELP_STRING([--enable-static-libs],
                [link statically with libxml2 and lib(e)xslt @<:@default=no@:>@])],
        [STATIC_LIBS=$enableval],
        [STATIC_LIBS=no])

AS_IF([test -r doc/xmlstarlet.1], [LACK_DOCS=no], [LACK_DOCS=yes])

AC_ARG_ENABLE([build_docs],
        [AS_HELP_STRING([--enable-build-docs],
                [include rules to build documentation files])],
        [], [enable_build_docs=$LACK_DOCS])

dnl
dnl Check the environment
dnl

AC_DEFINE([_XOPEN_SOURCE], [500], [needed to get lstat declaration in -ansi mode])

AC_CANONICAL_HOST
AC_PROG_CC
AM_CONDITIONAL([GCC], [test x$GCC = xyes])
AS_IF([test x$GCC = xyes], [CFLAGS="-ansi $CFLAGS"])
AM_PROG_CC_C_O
AC_ARG_PROGRAM          dnl Transforming Program Names When Installing
AC_PROG_SED
AC_PROG_AWK

XSTAR_LIB_CHECK([LIBXML], [xml2-config])

AS_IF([test "x$LIBXML_SRCDIR" != x],
      [LIBXML_INCDIR="$LIBXML_SRCDIR/include"])
[: ${LIBXML_INCDIR="$LIBXML_PREFIX/include/libxml2"}]

AS_IF([test "x$STATIC_LIBS" != xno],
      [LIBXML_CPPFLAGS="-I$LIBXML_INCDIR -DLIBXML_STATIC"],
      [LIBXML_CPPFLAGS="-I$LIBXML_INCDIR"])

AS_IF([test "x$STATIC_LIBS" != xno],
      [LIBXML_LIBS="$LIBXML_LIBDIR/libxml2.a"],
      [LIBXML_LIBS="-lxml2"])
[LIBXML_LDFLAGS="-L$LIBXML_LIBDIR"]


XSTAR_LIB_CHECK([LIBXSLT], [xslt-config])

AS_IF([test "x$LIBXSLT_SRCDIR" != x],
      [XSLTPROC_PATH="$LIBXSLT_SRCDIR/xsltproc:$PATH"
       LIBXSLT_INCDIR="$LIBXSLT_SRCDIR"
       LIBXSLT_LDFLAGS="-L$LIBXSLT_SRCDIR/libexslt/.libs -L$LIBXSLT_SRCDIR/libxslt/.libs"
       AS_IF([test "x$STATIC_LIBS" != xno],
             [LIBXSLT_LIBS="$LIBXSLT_SRCDIR/libexslt/.libs/libexslt.a $LIBXSLT_SRCDIR/libxslt/.libs/libxslt.a"])],
      [XSLTPROC_PATH="$PATH"
       LIBXSLT_LDFLAGS="-L$LIBXSLT_LIBDIR"
      AS_IF([test "x$STATIC_LIBS" != xno],
             [LIBXSLT_LIBS="$LIBXSLT_LIBDIR/libexslt.a $LIBXSLT_LIBDIR/libxslt.a"])])

AS_IF([test x"$enable_build_docs" = xyes],
      [AC_PATH_PROG(XSLTPROC, xsltproc, [none], [$XSLTPROC_PATH])
       AS_IF([test "$XSLTPROC" = none], [AC_MSG_ERROR([xsltproc not found, unable to build documentation])])

       AC_PATH_PROG(FOP, fop, [none])
       AM_CONDITIONAL([HAVE_FOP], [test x"$FOP" != xnone])

       AC_PATH_PROG(PDF2PS, pdf2ps, [none])
       AM_CONDITIONAL([HAVE_PDF2PS], [test x"$PDF2PS" != xnone])],

       [# these don't get used when BUILD_DOCS is false but Automake
        # will get cranky if we don't set the conditional
        AM_CONDITIONAL([HAVE_FOP], [false])
        AM_CONDITIONAL([HAVE_PDF2PS], [false])])

AM_CONDITIONAL([BUILD_DOCS], [test x"$enable_build_docs" = xyes])


AS_IF([test "x$STATIC_LIBS" = xno],
      [LIBXSLT_LIBS="-lexslt -lxslt"])

[: ${LIBXSLT_INCDIR="$LIBXSLT_PREFIX/include"}]
[LIBXSLT_CPPFLAGS="-I$LIBXSLT_INCDIR"]


[WIN32_EXTRA_LDFLAGS=]
[WIN32_EXTRA_LIBS=]

AS_CASE(["${host}"],
  [*-hpux*],
      [AS_IF([test "x$GCC" != "xyes"],
             [CFLAGS="-Ae"])],
  [*mingw*],
      [WIN32_EXTRA_LIBS='-lwsock32'
       AC_DEFINE([_WINSOCKAPI_],1,[Using the Win32 Socket implementation])])

AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_TYPE_MODE_T
AC_TYPE_SIZE_T

# The trick with USER_LIBS allows make LIBS=-lfoo to add rather than
# replace the libraries from ./configure
[USER_LIBS="$LIBS"]
[LIBS=]
# shared libraries link to dependant libraries automatically
AS_IF([test "x$STATIC_LIBS" != xno],
   [AC_SEARCH_LIBS([dlopen], [dl], [], [], "$USER_LIBS")
    AC_SEARCH_LIBS([sqrt], [m], [], [], "$USER_LIBS")
    AC_SEARCH_LIBS([iconv_open], [iconv], [],
    [AC_SEARCH_LIBS([libiconv_open], [iconv], [], [], "$USER_LIBS")], "$USER_LIBS")
    AC_SEARCH_LIBS([clock_gettime], [rt], [], [], "$USER_LIBS")
    AC_SEARCH_LIBS([deflate], [z], [], [], "$USER_LIBS")
    AC_SEARCH_LIBS([gcry_cipher_encrypt], [gcrypt], [], [], "$USER_LIBS")

    # Checks for inet libraries:
    AC_SEARCH_LIBS([gethostent], [nsl], [], [], "$USER_LIBS")
    AC_SEARCH_LIBS([setsockopt], [socket net network], [], [], "$USER_LIBS")
    AC_SEARCH_LIBS([connect], [inet], [], [], "$USER_LIBS")])

AC_CHECK_FUNCS_ONCE([lstat stat])

AC_CHECK_DECL([O_BINARY], [AC_DEFINE([HAVE_DECL_O_BINARY],1,[have O_BINARY])],
[AC_DEFINE([HAVE_DECL_O_BINARY],0,[don't have O_BINARY])], [[
#include <io.h>
#include <fcntl.h>
]])
AC_CHECK_FUNCS_ONCE([setmode])

# check for exslt*XpathCtxtRegister() functions
[OLD_CPPFLAGS="$CPPFLAGS"
 CPPFLAGS="$LIBXSLT_CPPFLAGS $LIBXML_CPPFLAGS $CPPFLAGS"]
AC_CHECK_DECL([exsltDateXpathCtxtRegister],
        [AC_DEFINE([HAVE_EXSLT_XPATH_REGISTER],1,[have exslt*XpathCtxtRegister()])
        HAVE_EXSLT_XPATH_REGISTER=1],
        [AC_DEFINE([HAVE_EXSLT_XPATH_REGISTER],0,[have exslt*XpathCtxtRegister()])
        HAVE_EXSLT_XPATH_REGISTER=0],
        [#include <libexslt/exslt.h>])

AM_CONDITIONAL([HAVE_EXSLT_XPATH_REGISTER], [test $HAVE_EXSLT_XPATH_REGISTER = 1])
[CPPFLAGS="$OLD_CPPFLAGS"]

# put warnings about missing docs at the end, so there is a better
# chance of seeing them.
AS_IF([test x$LACK_DOCS = xyes],
      [AS_IF([test "$FOP" = none],
        [AC_MSG_WARN([fop not installed, unable to build pdf documentation])])
       AS_IF([test "$PDF2PS" = none],
        [AC_MSG_WARN([pdf2ps not installed, unable to build postscript documentation])])])

# make flags
AC_SUBST([AM_CPPFLAGS], ["$LIBXSLT_CPPFLAGS $LIBXML_CPPFLAGS"])
AC_SUBST([AM_LDFLAGS], ["$LIBXSLT_LDFLAGS $LIBXML_LDFLAGS"])
AC_SUBST([LDADD], ["$LIBXSLT_LIBS $LIBXML_LIBS $WIN32_EXTRA_LIBS $LIBS"])
AC_SUBST([LIBS], ["$USER_LIBS"])
AC_SUBST([LIBXML_REQUIRED_VERSION])
AC_SUBST([LIBXSLT_REQUIRED_VERSION])

AC_SUBST([SED])

np_SILENT_RULE([FOP])
np_SILENT_RULE([DOCBOOK], [DBOOK])

AC_SUBST(PACKAGE_NAME)
AC_SUBST(PACKAGE_TARNAME)
AC_SUBST(PACKAGE_VERSION)

dnl for the spec file

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
xmlstarlet.spec
Makefile
solaris/package/sol8-sparc/pkginfo
solaris/package/sol9-sparc/pkginfo
])
AC_OUTPUT

