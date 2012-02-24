/* 
  Copyright (C) 2012 Rafael R. Sevilla

  This file is part of Arcueid

  Arcueid is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; either version 3 of the
  License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
*/
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include "arcueid.h"
#include "alloc.h"
#include "../config.h"
#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
#ifndef alloca
# define alloca __builtin_alloca
#endif
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# include <stddef.h>
void *alloca (size_t);
#endif

unsigned long long __arc_milliseconds(void)
{
#ifdef HAVE_CLOCK_GETTIME
  struct timespec tp;
  unsigned long long t;

  if (clock_gettime(CLOCK_REALTIME, &tp) < 0) {
    /* fall back to using time() if we have an error */
    return((unsigned long long)time(NULL)*1000LL);
  }

  t = ((unsigned long long)tp.tv_sec)*1000LL
    + ((unsigned long long)tp.tv_nsec / 1000000LL);
  return(t);
#else
  /* fall back to using time(2) if clock_gettime is not available */
  return((unsigned long long)time(NULL)*1000LL);
#endif
}

value arc_seconds(arc *c)
{
  return(arc_mkflonum(c, __arc_milliseconds()/1000.0));
}

value arc_msec(arc *c)
{
  unsigned long long ms;

  ms = __arc_milliseconds();
  if (ms < FIXNUM_MAX) {
    return(INT2FIX(ms));
  } else {
#ifdef HAVE_GMP_H
    value msbn;

#if SIZEOF_UNSIGNED_LONG_LONG == 8
    /* feed value into the bignum 32 bits at a time */
    msbn = arc_mkbignuml(c, (ms >> 32)&0xffffffff);
    mpz_mul_2exp(REP(msbn)._bignum, REP(msbn)._bignum, 32);
    mpz_add_ui(REP(msbn)._bignum, REP(msbn)._bignum, ms & 0xffffffff);
#else
    int i;

    msbn = arc_mkbignuml(c, 0);
    for (i=SIZEOF_UNSIGNED_LONG_LONG-1; i>=0; i--) {
      mpz_mul_2exp(REP(msbn)._bignum, REP(msbn)._bignum, 8);
      mpz_add_ui(REP(msbn)._bignum, REP(msbn)._bignum, (ms >> (i*8)) & 0xff);
    }
#endif
    return(msbn);
#else
    /* floating point */
    return(arc_mkflonum(c, (double)ms));
#endif
  }
  return(CNIL);
}

value arc_current_process_milliseconds(arc *c)
{
  struct rusage usage;

  getrusage(RUSAGE_SELF, &usage);
  return(arc_mkflonum(c, 1000.0*((double)usage.ru_utime.tv_sec
				 + ((double)(usage.ru_utime.tv_usec))/1e6)));
}

value arc_timedate(arc *c, int argc, value *argv)
{
  time_t tm;
  struct tm timep;
  value fnv;

  if (argc == 0)
    tm = __arc_milliseconds() / 1000L;
  else {
    fnv = arc_coerce_fixnum(c, argv[0]);
    if (NIL_P(fnv)) {
      /* XXX - this may introduce error */
      fnv = arc_coerce_flonum(c, argv[0]);
      tm = (time_t)REP(fnv)._flonum;
    } else {
      tm = (time_t)FIX2INT(fnv);
    }
  }

  if (gmtime_r(&tm, &timep) == NULL) {
    arc_err_cstrfmt(c, "error in gmtime");
    return(CNIL);
  }

  return(cons(c, INT2FIX(timep.tm_sec),
	      cons(c, INT2FIX(timep.tm_min),
		   cons(c, INT2FIX(timep.tm_hour),
			cons(c, INT2FIX(timep.tm_mday),
			     cons(c, INT2FIX(timep.tm_mon + 1),
				  cons(c, INT2FIX(timep.tm_year + 1900), CNIL)))))));
}

/* XXX - fix this so that we don't always return true, and
   permit redirection of both stdin and stdout. */
value arc_system(arc *c, value cmd)
{
  int len;
  char *cmdstr;
  FILE *fp;
  int chr;
  value so;

  TYPECHECK(cmd, T_STRING, 1);
  len = FIX2INT(arc_strutflen(c, cmd));
  cmdstr = (char *)alloca(sizeof(char)*len+1);
  arc_str2cstr(c, cmd, cmdstr);
  fp = popen(cmdstr, "r");
  if (fp == NULL) {
    int en = errno;
    arc_err_cstrfmt(c, "system: error executing command \"%s\", (%s; errno=%d)", cmdstr, strerror(en), en);
    return(CNIL);
  }
  so = arc_stdout(c);
  while ((chr = fgetc(fp)) != EOF)
    arc_writeb(c, INT2FIX(chr), so);
  pclose(fp);
  /* XXX - find out a way to get the original return value of the
     command to return it properly. */
  return(CTRUE);
}

value arc_quit(arc *c, int argc, value *argv)
{
  int exitcode = 0;

  if (argc == 1 && TYPE(argv[0] == T_FIXNUM))
    exitcode = FIX2INT(argv[0]);
  else if (argc > 1) {
    arc_err_cstrfmt(c, "quit: too many arguments (%d for 1)", argc);
    return(CNIL);
  }
  exit(exitcode);
  return(CNIL);
}

value arc_setuid(arc *c, value uid)
{
  TYPECHECK(uid, T_FIXNUM, 1);
  return(INT2FIX(setuid(FIX2INT(uid))));
}
