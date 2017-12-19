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

/*! \fn static void *bibop_alloc(mm_ctx *c, size_t osize)
    \brief Allocates using the BiBOP allocator
    \param c The memory allocator context
    \param osize The size of the memory block to be allocated.

    Allocate memory using the BiBOP allocator. The size _osize_ must
    be less than or equal to MAX_BIBOP.
 */
static void *bibop_alloc(struct mm_ctx *c, size_t osize)
{
  struct Bhdr *h, *bpage;
  size_t aosize, pagesize;
  char *bptr;
  int i;
  struct BBPhdr *newpage;

  for (;;) {
    /* Pull off an object from the BiBOP free list for that size if
       there is such a thing available. */
    if (c->bibop_fl[osize] != NULL) {
      h = c->bibop_fl[osize];
      c->bibop_fl[osize] = B2NB(h);
      break;
    }
    /* Create a new BiBOP page if the free list for that size is
       empty. First, we need to compute the actual size of a BiBOP
       object, which is the aligned size of the object plus the block
       header. */
    aosize = ALIGN_SIZE(osize) + BHDR_ALIGN_SIZE;
    /* Then, we need to compute the size of the entire page. A page
       consists of the Bhdr for the page itself, the BiBOP header,
       and the actual objects themselves. */
    pagesize = BHDR_ALIGN_SIZE + BBPHDR_ALIGN_SIZE + aosize * BIBOP_PAGE_SIZE;
    bpage = (struct Bhdr *)sysalloc(pagesize);
    if (bpage == NULL)
      __arc_fatal("failed to allocate memory for BiBOP page", errno);
    c->allocmem += pagesize;
    /* Link the new page into the BiBOP page list. */
    newpage = (struct BBPhdr *)B2D(bpage);
    newpage->next = c->bibop_pages[osize];
    c->bibop_pages[osize] = newpage;

    /* Initialise the new BiBOP page */
    bptr = newpage->_dummy + BBHPAD;
    for (i=0; i<BIBOP_PAGE_SIZE; i++) {
      h = (struct Bhdr *)bptr;
      BSSIZE(h, osize);
      BFREE(h);
      h->u._next = c->bibop_fl[osize];
      c->bibop_fl[osize] = h;
      bptr += aosize;
    }
  }

  /* Mark used memory, and prepare the new object for use */
  c->usedmem += osize;
  BSSIZE(h, osize);
  BALLOC(h);
  return(B2D(h));
}

void __arc_init_mm_ctx(struct mm_ctx *c)
{
  int i;

  for (i=0; i<=MAX_BIBOP; i++) {
    c->bibop_fl[i] = NULL;
    c->bibop_pages[i] = NULL;
  }
  c->allocmem = 0LL;
  c->usedmem = 0LL;
}
