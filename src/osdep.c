/* 
  Copyright (C) 2013 Rafael R. Sevilla

  This file is part of Arcueid

  Arcueid is free software: you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library. If not, see <http://www.gnu.org/licenses/>
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
#include "arith.h"
#include "builtins.h"
#include "io.h"
#include "osdep.h"
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
  return(__arc_ull2val(c, __arc_milliseconds() / 1000ULL));
}

value arc_msec(arc *c)
{
  return(__arc_ull2val(c, __arc_milliseconds()));
}

value arc_current_process_milliseconds(arc *c)
{
  struct rusage usage;

  getrusage(RUSAGE_SELF, &usage);
  return(arc_mkflonum(c, 1000.0*((double)usage.ru_utime.tv_sec
				 + ((double)(usage.ru_utime.tv_usec))/1e6)));
}

value arc_setuid(arc *c, value uid)
{
  int rv;

  TYPECHECK(uid, T_FIXNUM);
  rv = setuid(FIX2INT(uid));
  if (rv < 0) {
    int en = errno;

    arc_err_cstrfmt(c, "setuid error (%s; errno=%d)",
		    strerror(en), en);
    return(CNIL);
  }
  return(INT2FIX(rv));
}

AFFDEF(arc_timedate)
{
  AOARG(arg);
  ARARG(ignored);
  value fnv;
  time_t tm;
  struct tm timep;
  AFBEGIN;
  (void)ignored;

  if (BOUND_P(AV(arg))) {
    AFCALL(arc_mkaff(c, arc_coerce, CNIL), AV(arg),
	   ARC_BUILTIN(c, S_FIXNUM));
    fnv = AFCRV;
    if (NIL_P(fnv)) {
      AFCALL(arc_mkaff(c, arc_coerce, CNIL), AV(arg),
	     ARC_BUILTIN(c, S_FLONUM));
      fnv = AFCRV;
      tm = (time_t)REPFLO(fnv);
    } else {
      tm = FIX2INT(fnv);
    }
  } else {
    tm = __arc_milliseconds() / 1000ULL;
  }

  if (gmtime_r(&tm, &timep) == NULL) {
    arc_err_cstrfmt(c, "error in gmtime");
    ARETURN(CNIL);
  }

  fnv = cons(c, INT2FIX(timep.tm_sec),
	     cons(c, INT2FIX(timep.tm_min),
		  cons(c, INT2FIX(timep.tm_hour),
		       cons(c, INT2FIX(timep.tm_mday),
			    cons(c, INT2FIX(timep.tm_mon + 1),
				 cons(c, INT2FIX(timep.tm_year + 1900), CNIL))))));
  ARETURN(fnv);
  AFEND;
}
AFFEND

AFFDEF(arc_quit)
{
  AOARG(exitcode);
  int ec;
  AFBEGIN;

  if (BOUND_P(AV(exitcode))) {
    TYPECHECK(AV(exitcode), T_FIXNUM);
    ec = FIX2INT(AV(exitcode));
  } else {
    ec = 0;
  }
  exit(ec);
  ARETURN(CNIL);
  AFEND;
}
AFFEND

/* XXX - fix this so that we don't always return true, and
   permit redirection of both stdin and stdout.  This basically works
   as a wrapper for arc_pipe_from */
AFFDEF(arc_system)
{
  AARG(cmd);
  AVAR(pf);
  AFBEGIN;
  WV(pf, arc_pipe_from(c, AV(cmd)));
  for (;;) {
    AFCALL(arc_mkaff(c, arc_readc, CNIL), AV(pf));
    if (NIL_P(AFCRV))
      goto finished;
    AFCALL(arc_mkaff(c, arc_writec, CNIL), AFCRV);
  }
 finished:
  /* XXX - find out a way to get the original return value of the
     command to return it properly. */
  AFCALL(arc_mkaff(c, arc_close, CNIL), AV(pf));
  ARETURN(CTRUE);
  AFEND;
}
AFFEND
