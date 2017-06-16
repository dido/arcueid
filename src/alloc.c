/* 
  Copyright (C) 2017 Rafael R. Sevilla

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
#include "../config.h"
#ifdef HAVE_POSIX_MEMALIGN
#define _XOPEN_SOURCE 600
#endif

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include "arcueid.h"
#include "alloc.h"

#define MMVAR(c, var) (((struct mm_ctx *)c->mm_ctx)->var)
#define BIBOPFL(c) (MMVAR(c, bibop_fl))
#define BIBOPPG(c) (MMVAR(c, bibop_pages))
#define USEDMEM(c) (MMVAR(c, usedmem))
#define ALLOCMEM(c) (MMVAR(c, allocmem))
#define ALLOCHEAD(c) (MMVAR(c, allochead))

#ifdef HAVE_POSIX_MEMALIGN

static void *sysalloc(size_t size)
{
  void *memptr;

  if (posix_memalign(&memptr, ALIGN, size) == 0)
    return(memptr);
  return(NULL);
}

static void sysfree(void *ptr)
{
  free(ptr);
}

#else

/* Used on systems that don't provide posix_memalign.  Based on a
   solution found here:

   http://stackoverflow.com/questions/196329/osx-lacks-memalign
 */
static void *sysalloc(size_t size)
{
  void *mem;
  char *amem;

  mem = malloc(size + (ALIGN-1) + sizeof(void *));
  if (mem == NULL)
    return(NULL);
  amem = ((char *)mem) + sizeof(void *);
  amem += ALIGN - ((value)amem & (ALIGN - 1));

  ((void **)amem)[-1] = mem;
  return((void *)amem);
}

static void sysfree(void *ptr)
{
  if (ptr == NULL)
    return(NULL);
  free(((void **)ptr)[-1]);
}

#endif

/* Allocate a new object using the BiBOP allocator */
static void *bibop_alloc(arc *c, size_t osize)
{
  Bhdr *h, *bpage;
  size_t actual, alignsize;
  char *bptr;
  int i;

  for (;;) {
    /* Pull off an object from the BiBOP free list if there is an
       available object of that size. */
    if (BIBOPFL(c)[osize] != NULL) {
      h = BIBOPFL(c)[osize];
      BIBOPFL(c)[osize] = B2NB(BIBOPFL(c)[osize]);
      break;
    }

    /* Create a new BiBOP page if the free list for that size is
       empty.  The page base address is properly aligned since
       sysalloc() is guaranteed to return aligned addresses, and
       since BiBOP objects inside the page are padded to a multiple
       of the alignment, all objects inside will also by definition
       be aligned. */
    actual = ALIGN_SIZE(osize) + BHDR_ALIGN_SIZE;
    alignsize = actual * BIBOP_PAGE_SIZE + BHDR_ALIGN_SIZE;
    bpage = (Bhdr *)sysalloc(alignsize);
    if (bpage == NULL)
      __arc_fatal("failed to allocate memory for BiBOP page", errno);
    ALLOCMEM(c) += alignsize;
    /* link the new BiBOP page into the BiBOP page list */
    BSSIZE(bpage, alignsize);
    bpage->_next = BIBOPPG(c)[osize];
    BIBOPPG(c)[osize] = bpage;
    /* Initialise the new BiBOP page */
    bptr = B2D(bpage);
    for (i=0; i<BIBOP_PAGE_SIZE; i++) {
      h = (Bhdr *)bptr;
      BSSIZE(h, osize);
      BFREE(h);
      h->_next = BIBOPFL(c)[osize];
      BIBOPFL(c)[osize] = h;
      bptr += actual;
    }
  }
  /* Mark used memory, and prepare the new object for use */
  USEDMEM(c) += osize;
  BSSIZE(h, osize);
  BALLOC(h);
  h->_next = ALLOCHEAD(c);
  ALLOCHEAD(c) = h;
  return(B2D(h));
}

static void *alloc(arc *c, size_t osize)
{
  Bhdr *h;
  size_t actual;

  /* Use BiBOP allocator for smaller objects of up to the maximum size */
  if (osize <= MAX_BIBOP)
    return(bibop_alloc(c, osize));

  /* Normal allocation. Just append the block header size with proper
     alignment padding. */
  actual = osize + BHDR_ALIGN_SIZE;
  h = (Bhdr *)sysalloc(alignsize);
  if (h == NULL)
    __arc_fatal("failed to allocate memory", errno);
  ALLOCMEM(c) += alignsize;
  USEDMEM(c) += osize;
  BALLOC(h);
  h->_next = ALLOCHEAD(c);
  ALLOCHEAD(c) = h;
  return(B2D(h));
}
