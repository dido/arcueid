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
#include <sys/time.h>
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


#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#ifdef HAVE_MMAP

void *__arc_aligned_mmap(size_t osize, int modulo, void **block)
{
  char *raw_mem;
  static void *last_addr = NULL;
  size_t size;
  unsigned long aligned_mem;

  ROUNDSIZE(size, osize);
  raw_mem = (char *)mmap(last_addr, size + PAGE_SIZE, PROT_READ | PROT_WRITE,
			 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (raw_mem == MAP_FAILED)
    return(NULL);
  last_addr = raw_mem + size + 2*PAGE_SIZE;
  *block = (void *)raw_mem;
  raw_mem += modulo;
  aligned_mem = ((((unsigned long)raw_mem / PAGE_SIZE) + 1)* PAGE_SIZE);
  return((void *)(aligned_mem - modulo));
}

void __arc_aligned_munmap(void *addr, size_t size)
{
  int retcode = munmap(addr, size + PAGE_SIZE);
  assert(retcode == 0);
}

#endif

/* use malloc */
void *__arc_aligned_malloc(size_t osize, int modulo, void **block)
{
  char *raw_mem;
  unsigned long aligned_mem;
  size_t size;

  ROUNDSIZE(size, osize);
  raw_mem = (char *)malloc(size + PAGE_SIZE);
  if (raw_mem == NULL)
    return(NULL);
  *block = (void *)raw_mem;
  raw_mem += modulo;
  aligned_mem = ((((unsigned long)raw_mem / PAGE_SIZE) + 1)* PAGE_SIZE);
  return((void *)(aligned_mem - modulo));
}

void __arc_aligned_free(void *addr, size_t size)
{
  free(addr);
}

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
  return((unsigend long long)time(NULL)*1000LL);
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

value arc_system(arc *c, value cmd)
{
  int len;
  char *cmdstr;

  len = FIX2INT(arc_strutflen(c, cmd));
  cmdstr = (char *)alloca(sizeof(char)*len+1);
  arc_str2cstr(c, cmd, cmdstr);
  return((system(cmdstr) == 0) ? CTRUE : CNIL);
}
