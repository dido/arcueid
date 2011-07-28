/* 
  Copyright (C) 2010 Rafael R. Sevilla

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
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA
  02110-1301 USA.
*/
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include "arcueid.h"
#include "alloc.h"
#include "../config.h"

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

double __arc_seconds(void)
{
#ifdef HAVE_CLOCK_GETTIME
  struct timespec tp;
  double t;

  if (clock_gettime(CLOCK_REALTIME, &tp) < 0) {
    /* fall back to using time() if we have an error */
    return((double)time(NULL));
  }
  t = (double)tp.tv_sec + (((double)tp.tv_nsec)/1e9);
  return(t);
#else
  /* fall back to using time(2) if clock_gettime is not available */
  return(time(NULL));
#endif
}

value arc_seconds(arc *c)
{
  return(arc_mkflonum(c, __arc_seconds()));
}

value arc_msec(arc *c)
{
  return(arc_mkflonum(c, 1000.0*__arc_seconds()));
}
