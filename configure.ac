# Copyright (C) 2012 Rafael R. Sevilla
#
# This file is part of Arcueid
#
# Arcueid is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation; either version 3 of the
# License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, see <http://www.gnu.org/licenses/>.
#
AC_PREREQ([2.71])
AC_INIT([Arcueid],[0.1.3],[dido@imperium.ph])
AC_CONFIG_SRCDIR([src/alloc.c])

AM_INIT_AUTOMAKE([-Wall -Werror])
AC_CANONICAL_HOST
AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CC
AC_PROG_LN_S
AM_PROG_AR
AC_PROG_INSTALL
LT_INIT
AM_PROG_CC_C_O

AC_C_CONST
AC_C_INLINE

AC_C_BIGENDIAN

AC_SYS_LARGEFILE
AC_FUNC_FSEEKO

# Autoupdate added the next two lines to ensure that your configure
# script's behavior did not change.  They are probably safe to remove.
AC_CHECK_INCLUDES_DEFAULT
AC_PROG_EGREP


MATH_LIBS=-lm
AC_CHECK_HEADERS(math.h,, AC_MSG_FAILURE([Math library header seems to be missing.]))

AC_CHECK_HEADERS(sysexits.h)
AC_CHECK_HEADERS(pwd.h)
AC_CHECK_HEADERS(malloc.h)

AC_FUNC_ALLOCA

AC_ARG_WITH(readline, AS_HELP_STRING([--without-readline],[disable readline support]))
if test xno != x$with_readline; then
   AX_LIB_READLINE
   AC_SUBST(RLLIBS)
fi

AC_CHECK_SIZEOF(void *)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(unsigned long long)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(double)
AC_MSG_CHECKING([that long and void * have the same size])
if test $ac_cv_sizeof_long == $ac_cv_sizeof_void_p
then
   AC_MSG_RESULT(yes)
else
   AC_MSG_RESULT(no)
   AC_MSG_FAILURE([Arcueid requires that sizeof(long) be the same as sizeof(void) to compile])
fi
VALUE_SIZE=$ac_cv_sizeof_void_p
AC_SUBST(VALUE_SIZE)

AC_ARG_ENABLE([unit-tests], [AS_HELP_STRING([--enable-unit-tests], [enable GNU Check unit test suites @<:@default=no@:>@])], [], [enable_unit_tests=no])
SRC_SUBDIRS=src
TEST_MAKEFILE=
if test "x$enable_unit_tests" != xno; then
  PKG_CHECK_MODULES([CHECK], [check >= 0.9.4],
  	            [SRC_SUBDIRS="$SRC_SUBDIRS . test"
		     TEST_MAKEFILE=test/Makefile])
fi
AC_SUBST(SRC_SUBDIRS)
  
AC_ARG_ENABLE([bignum], [AS_HELP_STRING([--disable-bignum], [disable bignum/rational support])], [], [enable_bignum=yes])
GMP_LIBS=
if test "x$enable_bignum" != xno; then
  AC_CHECK_HEADERS(gmp.h,, AC_MSG_FAILURE([Bignum (libgmp) test failed (--disable-bignum to disable)]))
  AC_CHECK_LIB(gmp, __gmpz_init, GMP_LIBS=-lgmp)
  if test "$GMP_LIBS" != ""; then
    MP_HEADER="#include <gmp.h>"
    MP_TYPE="typedef mpz_t bignum_t;"
    MP_BVALUE="bignum_t _bignum;"
    MPQ_TYPE="typedef mpq_t rational_t;"
    MPQ_BVALUE="rational_t _rational;"
    AC_SUBST(MP_HEADER)
    AC_SUBST(MP_TYPE)
    AC_SUBST(MP_BVALUE)
    AC_SUBST(MPQ_TYPE)
    AC_SUBST(MPQ_BVALUE)
  else
    AC_MSG_FAILURE([Bignum (libgmp) test failed (--disable-bignum to disable)])
  fi
fi

AC_ARG_ENABLE([threaded-interpreter], [AS_HELP_STRING([--disable-threaded-interpreter], [disable threaded interpreter (threaded interpreter requires GCC extensions)])], [], [enable_threaded_interpreter=yes])
if test "x$enable_threaded_interpreter" != xno; then
   AC_C_LABELS_AS_VALUES
   if test "$ac_cv_labels_as_values" = yes; then
     AC_DEFINE(HAVE_THREADED_INTERPRETER, [1],
       [Define to 1 if threaded interpreter is to be used.])
   else
     AC_MSG_FAILURE([compiler does not support threaded interpreter (--disable-threaded-interpreter to disable)])
   fi
fi

AC_ARG_ENABLE([tracing], [AS_HELP_STRING([--enable-tracing], [enable bytecode tracer])], [], [enable_tracing=no])
if test "x$enable_tracing" != xno; then
  AC_DEFINE(HAVE_TRACING, [1], [Define to 1 if bytecode tracing is to be enabled.])
fi

AC_CHECK_FUNCS(clock_gettime, [], [
  AC_CHECK_LIB(rt, clock_gettime, [
    AC_DEFINE(HAVE_CLOCK_GETTIME, 1)
    EXTRA_LIBS="$EXTRA_LIBS -lrt"
  ])
])

AC_CHECK_FUNCS(posix_memalign realpath malloc_trim)

AC_ARG_WITH(epoll, AS_HELP_STRING([--without-epoll],[disable epoll support (Linux only)]))
dnl System type checks.
case "$host" in
  *-linux-*)
    if test xno != x$with_epoll; then
       AC_CHECK_HEADERS(sys/epoll.h)
       AC_CHECK_FUNCS(epoll_create)
    fi
    ;;
esac

AC_CHECK_HEADERS(poll.h sys/select.h sys/socket.h string.h strings.h)

LIBARCUEID_LIBS="-L${libdir} -larcueid $GMP_LIBS $MATH_LIBS $EXTRA_LIBS"
AC_SUBST(LIBARCUEID_LIBS)
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([arcueid.pc Makefile src/Makefile test/Makefile arc/Makefile])
AC_OUTPUT
