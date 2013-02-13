/* 
  Copyright (C) 2013 Rafael R. Sevilla

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
/* TODO:

   1. Find a way to allow memory allocated for BiBOP pages to be returned
      to the operating system.  As it is, only memory not allocated by the
      BiBOP scheme (i.e. objects larger than MAX_BIBOP bytes) can be
      returned to the OS.  A few schemes for doing this are possible,
      but extra overhead is a concern.  Obvious ways of doing this add
      unacceptable levels of space and/or time overhead.

   2. Make BiBOP page sizes adaptive based on their size and adjust
      dynamically.

   3. Think up better conditions of when to implicitly invoke the
      garbage collector.
      As it is, the garbage collector is invoked whenever:

      (a) A BiBOP allocation fails to find a free element in the pages
          that have already been allocated.  A garbage collection cycle
	  may free up more space.
      (b) a large (i.e. non-BiBOP) object has to be allocated.  A
          garbage cycle may free up additional system memory where
	  hopefully a new large object will not fragment memory.
   4. A better garbage collector of some kind... Good question on what
      to use.  Current algorithm is just a simple mark and sweep
      collector.
 */
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include "arcueid.h"
#include "alloc.h"
#include "../config.h"

/* Maximum size of objects subject to BiBOP allocation */
#define MAX_BIBOP 512

/* Number of objects in each BiBOP page */
#define BIBOP_PAGE_SIZE 64

/* The BiBOP free list */
static Bhdr *bibop_fl[MAX_BIBOP+1];

/* The allocated list */
static Bhdr *alloc_head;

static void *bibop_alloc(arc *c, size_t osize)
{
  Bhdr *h;
  size_t actual;
  char *bpage, *bptr;
  int i, dogc = 1;

  for (;;) {
    /* Pull off an object from the BiBOP free list if there is an
       available object of that size. */
    if (bibop_fl[osize] != NULL) {
      h = bibop_fl[osize];
      bibop_fl[osize] = B2NB(bibop_fl[osize]);
      break;
    }

    /* No more free objects in pages allocated so far?  Try to gc
       before making a new page. */
    if (dogc) {
      c->gc(c);
      dogc = 0;
      continue;
    }

    /* Create a new BiBOP page if the free list for that size is
       empty.  The page base address is properly aligned since
       c->mem_alloc is guaranteed to return aligned addresses, and
       since BiBOP objects inside the page are padded to a multiple
       of the alignment, all objects inside will also by definition
       be aligned. */
    actual = ALIGN_SIZE(osize) + BHDR_ALIGN_SIZE;
    bpage = c->mem_alloc(actual * BIBOP_PAGE_SIZE);
    if (bpage == NULL) {
      fprintf(stderr, "FATAL: failed to allocate memory for BiBOP page\n");
      exit(1);
    }
    bptr = bpage;
    for (i=0; i<BIBOP_PAGE_SIZE; i++) {
      h = (Bhdr *)bptr;
      BSSIZE(h, osize);
      BFREE(h);
      h->_next = bibop_fl[osize];
      bibop_fl[osize] = h;
      bptr += actual;
    }
  }
  BALLOC(h);
  h->_next = alloc_head;
  alloc_head = h;
  return(B2D(h));
}

static void *alloc(arc *c, size_t osize)
{
  Bhdr *h;
  size_t actual;

  if (osize <= MAX_BIBOP)
    return(bibop_alloc(c, osize));

  /* Obligatory GC.  XXX - Consider whether a better approach is
     warranted */
  c->gc(c);
  /* Normal allocation.  Just append the block header size with
     proper alignment padding. */
  actual = osize + BHDR_ALIGN_SIZE;
  h = (Bhdr *)c->mem_alloc(actual);
  if (h == NULL) {
    fprintf(stderr, "FATAL: failed to allocate memory\n");
    exit(1);
  }
  BALLOC(h);
  h->_next = alloc_head;
  alloc_head = h;
  return(B2D(h));
}

/* Freeing a block requires one know the previous block in the alloc
   list.  Probably only feasible to use for the garbage collector's
   sweeper, which already traverses the allocated list. */
static void free_block(arc *c, void *blk, void *prevblk)
{
  Bhdr *h, *p;

  D2B(h, blk);
  /* Unlink the block from the alloc list. */
  if (prevblk == NULL) {
    /* When prevblk is NULL, that implies that h == alloc_head */
    alloc_head = B2NB(h);
  } else {
    D2B(p, prevblk);
    p->_next = B2NB(h);
  }

  if (BSIZE(h) <= MAX_BIBOP) {
    /* For BiBOP allocated objects, freeing them just means putting the
       object back into the free list for the object size. */
    BFREE(h);
    h->_next = bibop_fl[BSIZE(h)];
    bibop_fl[BSIZE(h)] = h;
  } else
    c->mem_free(h);
}

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

/* The actual garbage collector */

static int nprop;

/* Mark a value with the propagator flag */
void __arc_markprop(value p)
{
  struct cell *cp;

  if (IMMEDIATE_P(p))
    return;

  cp = (struct cell *)p;
  cp->_type = (cp->_type & FLAGMASK) | PROPFLAG;
  nprop = 1;
}

/* Maximum recursion depth for marking */
#define MAX_MARK_RECURSION_DEPTH 16

static void mark(arc *c, value v, int depth)
{
  struct cell *cp;

  /* Immediate values obviously cannot be marked */
  if (IMMEDIATE_P(v))
    return;

  if (depth > MAX_MARK_RECURSION_DEPTH) {
    /* Stop recursion, and mark the object with the propagator flag
       so that the next iteration scan can pick it up. */
    __arc_markprop(v);
    return;
  }
  cp = (struct cell *)v;
  /* Mark the object */
  cp->_type = (cp->_type & FLAGMASK) | MARKFLAG;
  /* Recurse into the object's structure at increased depth */
  cp->marker(c, v, depth+1, mark);
}

/* Basic mark/sweep garbage collector */
static void gc(arc *c)
{
  Bhdr *ptr;
  struct cell *cp;
  void *pptr;

  c->markroots(c);
  /* Mark phase */
  while (nprop) {
    /* Look for objects marked with propagator */
    nprop = 0;
    for (ptr = alloc_head; ptr; ptr = B2NB(ptr)) {
      cp = (struct cell *)B2D(ptr);
      if ((cp->_type & PROPFLAG) == PROPFLAG)
	mark(c, (value)cp, 0);
    }
  }

  /* Sweep phase */
  for (ptr = alloc_head, pptr = NULL; ptr;) {
    cp = (struct cell *)B2D(ptr);
    if ((cp->_type & MARKFLAG) == MARKFLAG) {
      /* Marked object.  Previous pointer moves to this one. */
      pptr = (void *)cp;
      /* clear the mark flag */
      cp->_type &= FLAGMASK;
    } else {
      /* Was not marked during the marking phase. Sweep it away! */
      cp->sweeper(c, (value)cp);
      c->free(c, (void *)cp, pptr);
      /* We don't update pptr in this case, it remains the same */
    }
    ptr = B2NB(ptr);
  }
}

void arc_set_memmgr(arc *c)
{
  int i;

  c->mem_alloc = sysalloc;
  c->mem_free = sysfree;
  c->gc = gc;
  c->alloc = alloc;
  c->free = free_block;

  for (i=0; i<=MAX_BIBOP; i++)
    bibop_fl[i] = NULL;
  alloc_head = NULL;
}
