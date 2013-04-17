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
    mpz_mul_2exp(REPBNUM(msbn), REPBNUM(msbn), 32);
    mpz_add_ui(REPBNUM(msbn), REPBNUM(msbn), ms & 0xffffffff);
#else
    int i;

    msbn = arc_mkbignuml(c, 0);
    for (i=SIZEOF_UNSIGNED_LONG_LONG-1; i>=0; i--) {
      mpz_mul_2exp(REPBNUM(msbn), REPBNUM(msbn), 8);
      mpz_add_ui(REPBNUM(msbn), REPBNUM(msbn), (ms >> (i*8)) & 0xff);
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

value arc_setuid(arc *c, value uid)
{
  TYPECHECK(uid, T_FIXNUM);
  return(INT2FIX(setuid(FIX2INT(uid))));
}
