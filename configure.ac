AC_PREREQ([2.69])
AC_INIT([mutexgear],[0.1],[oleh.derevenko@gmail.com])
AC_LANG(C)

AC_COPYRIGHT([Copyright (C) 2020, Oleh Derevenko <oleh.derevenko@gmail.com>])
AC_REVISION([$Revision: 1.00 $])

MUTEXGEAR_VERSION=0.1
AC_SUBST(MUTEXGEAR_VERSION)

#  1. Start with version information of `0:0:0' for each libtool library.
#
#  2. Update the version information only immediately before a public
#     release of your software.  More frequent updates are unnecessary,
#     and only guarantee that the current interface number gets larger
#     faster.
#
#  3. If the library source code has changed at all since the last
#     update, then increment REVISION (`C:R:A' becomes `C:r+1:A').
#
#  4. If any interfaces have been added, removed, or changed since the
#     last update, increment CURRENT, and set REVISION to 0.
#
#  5. If any interfaces have been added since the last public release,
#     then increment AGE.
#
#  6. If any interfaces have been removed since the last public release,
#     then set AGE to 0.
CURRENT=0
REVISION=0
AGE=0

AC_ARG_ENABLE(version-info,
	AS_HELP_STRING([--disable-version-info],
		[Don't encode version information into the library]),
		version_info=$enableval, version_info=yes)
if test x$version_info = xyes; then
	MUTEXGEAR_VERSION_INFO="-version-info $CURRENT:$REVISION:$AGE"
else
	MUTEXGEAR_VERSION_INFO="-avoid-version"
fi
AC_SUBST(MUTEXGEAR_VERSION_INFO)


AC_CONFIG_SRCDIR([src/wheel.c])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([1.10 foreign])

AC_PROG_MAKE_SET
${CFLAGS="-O3"}
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_CPP
# AC_PROG_AWK
AC_PROG_SED
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MKDIR_P
PKG_PROG_PKG_CONFIG
LT_INIT([disable-shared win32-dll])

AC_CHECK_HEADERS([atomic.h limits.h pthread.h stdatomic.h stddef.h stdint.h stdlib.h unistd.h])
AC_LANG_PUSH(C++)
AC_CHECK_HEADERS([atomic])
AC_LANG_POP()

AC_CHECK_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_C_VOLATILE
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

AC_CHECK_LIB([pthread], [pthread_mutex_init], [MUTEXGEAR_LIBPTHREAD=-lpthread],[MUTEXGEAR_LIBPTHREAD=])
AC_CHECK_LIB([stdc++], [main], [MUTEXGEAR_LIBSTRCXX=-lstdc++], [MUTEXGEAR_LIBSTRCXX=])
AC_CHECK_FUNCS([clock_gettime gettimeofday])
AC_CHECK_FUNC([pthread_mutexattr_getprioceiling],,AC_CHECK_LIB([pthread], [pthread_mutexattr_getprioceiling]))
AC_CHECK_FUNC([pthread_mutexattr_getprotocol],,AC_CHECK_LIB([pthread], [pthread_mutexattr_getprotocol]))

AC_SUBST([MUTEXGEAR_PRIVATE_LIBS], $MUTEXGEAR_LIBPTHREAD)
AC_SUBST([MUTEXGEAR_TEST_LIBS], $MUTEXGEAR_LIBSTRCXX)

MUTEXGEAR_HAVE_C11=0
MUTEXGEAR_HAVE_ATOMIC_INSTEADOF_STDATOMIC=0
if test $ac_cv_header_atomic_h$ac_cv_header_stdatomic_h != nono; then
  MUTEXGEAR_HAVE_C11=1
  if test $ac_cv_header_atomic_h$ac_cv_header_stdatomic_h = yesno; then
    MUTEXGEAR_HAVE_ATOMIC_INSTEADOF_STDATOMIC=1
  fi
fi
AC_SUBST([MUTEXGEAR_HAVE_C11])
AC_SUBST([MUTEXGEAR_HAVE_ATOMIC_INSTEADOF_STDATOMIC])

MUTEXGEAR_HAVE_CXX11=0
if test $ac_cv_header_atomic = yes; then
  MUTEXGEAR_HAVE_CXX11=1
fi
AC_SUBST([MUTEXGEAR_HAVE_CXX11])

MUTEXGEAR_HAVE_PTHREAD_H=0
if test $ac_cv_header_pthread_h = yes; then
  MUTEXGEAR_HAVE_PTHREAD_H=1
fi
AC_SUBST([MUTEXGEAR_HAVE_PTHREAD_H])

MUTEXGEAR_HAVE_NO_PTHREAD_MUTEXATTR_GETPRIOCEILING=0
if test $ac_cv_func_pthread_mutexattr_getprioceiling$ac_cv_lib_pthread_pthread_mutexattr_getprioceiling = nono; then
  MUTEXGEAR_HAVE_NO_PTHREAD_MUTEXATTR_GETPRIOCEILING=1
fi
AC_SUBST([MUTEXGEAR_HAVE_NO_PTHREAD_MUTEXATTR_GETPRIOCEILING])

MUTEXGEAR_HAVE_NO_PTHREAD_MUTEXATTR_GETPROTOCOL=0
if test $ac_cv_func_pthread_mutexattr_getprotocol$ac_cv_lib_pthread_pthread_mutexattr_getprotocol = nono; then
  MUTEXGEAR_HAVE_NO_PTHREAD_MUTEXATTR_GETPROTOCOL=1
fi
AC_SUBST([MUTEXGEAR_HAVE_NO_PTHREAD_MUTEXATTR_GETPROTOCOL])

MUTEXGEAR_HAVE_CLOCK_GETTIME=0
if test $ac_cv_func_clock_gettime = yes; then
  MUTEXGEAR_HAVE_CLOCK_GETTIME=1
fi
AC_SUBST([MUTEXGEAR_HAVE_CLOCK_GETTIME])

MUTEXGEAR_HAVE_GETTIMEOFDAY=0
if test $ac_cv_func_gettimeofday = yes; then
  MUTEXGEAR_HAVE_GETTIMEOFDAY=1
fi
AC_SUBST([MUTEXGEAR_HAVE_GETTIMEOFDAY])

AC_CONFIG_FILES([
 Makefile
 mutexgear.pc
 include/Makefile
 include/mutexgear/Makefile
 include/mutexgear/_confvars.h
 src/Makefile
 test/Makefile
 ])
AC_CONFIG_FILES([mutexgear-config], [chmod +x mutexgear-config])
AC_OUTPUT


BUILDDIR=`pwd`

echo "Configuration:"
echo "  Build  system type:      $build"
echo "  Host   system type:      $host"

