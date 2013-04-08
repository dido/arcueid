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
   3. A better garbage collector of some kind... Good question on what
      to use.  Current algorithm is just a simple mark and sweep
      collector.
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
#include "arcueid.h"
#include "alloc.h"

/* Maximum size of objects subject to BiBOP allocation */
#define MAX_BIBOP 512

/* Number of objects in each BiBOP page */
#define BIBOP_PAGE_SIZE 64

struct mm_ctx {
  /* The BiBOP free list */
  Bhdr *bibop_fl[MAX_BIBOP+1];

  /* The allocated list */
  Bhdr *alloc_head;
  int nprop;
};

#define BIBOPFL(c) (((struct mm_ctx *)c->alloc_ctx)->bibop_fl)
#define ALLOCHEAD(c) (((struct mm_ctx *)c->alloc_ctx)->alloc_head)
#define NPROP(c) (((struct mm_ctx *)c->alloc_ctx)->nprop)
static void *bibop_alloc(arc *c, size_t osize)
{
  Bhdr *h;
  size_t actual;
  char *bpage, *bptr;
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
      h->_next = BIBOPFL(c)[osize];
      BIBOPFL(c)[osize] = h;
      bptr += actual;
    }
  }
  BALLOC(h);
  h->_next = ALLOCHEAD(c);
  ALLOCHEAD(c) = h;
  return(B2D(h));
}

static void *alloc(arc *c, size_t osize)
{
  Bhdr *h;
  size_t actual;

  if (osize <= MAX_BIBOP)
    return(bibop_alloc(c, osize));

  /* Normal allocation.  Just append the block header size with
     proper alignment padding. */
  actual = osize + BHDR_ALIGN_SIZE;
  h = (Bhdr *)c->mem_alloc(actual);
  if (h == NULL) {
    fprintf(stderr, "FATAL: failed to allocate memory\n");
    exit(1);
  }
  BALLOC(h);
  h->_next = ALLOCHEAD(c);
  ALLOCHEAD(c) = h;
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
    /* When prevblk is NULL, that implies that h == ALLOC_HEAD(c) */
    ALLOCHEAD(c) = B2NB(h);
  } else {
    D2B(p, prevblk);
    p->_next = B2NB(h);
  }

  if (BSIZE(h) <= MAX_BIBOP) {
    /* For BiBOP allocated objects, freeing them just means putting the
       object back into the free list for the object size. */
    BFREE(h);
    h->_next = BIBOPFL(c)[BSIZE(h)];
    BIBOPFL(c)[BSIZE(h)] = h;
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

/* Mark a value with the propagator flag */
void __arc_markprop(arc *c, value p)
{
  struct cell *cp;

  if (IMMEDIATE_P(p))
    return;

  cp = (struct cell *)p;
  cp->_type = (cp->_type & FLAGMASK) | PROPFLAG;
  NPROP(c) = 1;
}

/* Maximum recursion depth for marking */
#define MAX_MARK_RECURSION_DEPTH 16

static void mark(arc *c, value v, int depth)
{
  typefn_t *tfn;

  if (TYPE(v) == T_SYMBOL) {
    value symid, name, bucket;

    /* Symbol marking is done by marking the hash buckets in the
       forward and reverse symbol tables.  Since the symbol tables
       are weak hashes, this prevents that particular symbol from
       becoming swept. */
    symid = INT2FIX(SYM2ID(v));
    bucket = arc_hash_lookup2(c, c->rsymtable, symid);
    mark(c, bucket, depth);
    name = REP(bucket)[2];	/* XXX - this is problematic */
    bucket = arc_hash_lookup2(c, c->symtable, name);
    mark(c, bucket, depth);
    return;
  }

  /* Other types of immediate values obviously cannot be marked */
  if (IMMEDIATE_P(v))
    return;

  if (depth > MAX_MARK_RECURSION_DEPTH) {
    /* Stop recursion, and mark the object with the propagator flag
       so that the next iteration scan can pick it up. */
    __arc_markprop(c, v);
    return;
  }

  tfn = __arc_typefn(c, v);
  MARK(v);
  /* Recurse into the object's structure at increased depth */
  tfn->marker(c, v, depth+1, mark);
}

/* Basic mark/sweep garbage collector */
static void gc(arc *c)
{
  Bhdr *ptr;
  struct cell *cp;
  void *pptr;
  typefn_t *tfn;

  c->markroots(c);
  /* Mark phase */
  while (NPROP(c)) {
    /* Look for objects marked with propagator */
    NPROP(c) = 0;
    for (ptr = ALLOCHEAD(c); ptr; ptr = B2NB(ptr)) {
      cp = (struct cell *)B2D(ptr);
      if ((cp->_type & PROPFLAG) == PROPFLAG)
	mark(c, (value)cp, 0);
    }
  }

  /* Sweep phase */
  for (ptr = ALLOCHEAD(c), pptr = NULL; ptr;) {
    cp = (struct cell *)B2D(ptr);
    if ((cp->_type & MARKFLAG) == MARKFLAG) {
      /* Marked object.  Previous pointer moves to this one. */
      pptr = (void *)cp;
      /* clear the mark flag */
      cp->_type &= FLAGMASK;
      ptr = B2NB(ptr);
    } else {
      /* Was not marked during the marking phase. Sweep it away! */
      tfn = __arc_typefn(c, (value)cp);
      tfn->sweeper(c, (value)cp);
      ptr = B2NB(ptr);
      c->free(c, (void *)cp, pptr);
      /* We don't update pptr in this case, it remains the same */
    }
  }
}

/* Default root marker */
static void markroots(arc *c)
{
  __arc_markprop(c, c->symtable);
  __arc_markprop(c, c->rsymtable);
  __arc_markprop(c, c->genv);
  __arc_markprop(c, c->vmthreads);
  __arc_markprop(c, c->typedesc);
  __arc_markprop(c, c->builtins);
  __arc_markprop(c, c->here);
}

void arc_init_memmgr(arc *c)
{
  int i;

  c->mem_alloc = sysalloc;
  c->mem_free = sysfree;
  c->markroots = markroots;
  c->gc = gc;
  c->alloc = alloc;
  c->free = free_block;
  c->alloc_ctx = (struct mm_ctx *)malloc(sizeof(struct mm_ctx));

  for (i=0; i<=MAX_BIBOP; i++)
    BIBOPFL(c)[i] = NULL;
  ALLOCHEAD(c) = NULL;
}
