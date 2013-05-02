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
#include "arith.h"

static int nprop;		/* propagator flag */
static int mutator;		/* current mutator colour */
static int marker;		/* current marker colour */
static int sweeper;		/* current sweeper colour */
static arc *__arc_handle=NULL;	/* current arc handle */

#define PROPAGATOR 3		/* default propagator colour */

#define SETMARK(b) if (BCOLOUR(b) != mutator) { BSCOLOUR(b, PROPAGATOR); nprop = 1; }
static inline void MARKPROP(value v)
{
  if (TYPE(v) == T_SYMBOL) {
    value symid, name, bucket;
    arc *c = __arc_handle;

    symid = INT2FIX(SYM2ID(v));
    bucket = arc_hash_lookup2(c, c->rsymtable, symid);
    MARKPROP(bucket);
    name = REP(bucket)[2];	/* XXX - this is problematic */
    bucket = arc_hash_lookup2(c, c->symtable, name);
    MARKPROP(bucket);
    return;
  }
  if (!IMMEDIATE_P(v)) {
    Bhdr *p; D2B(p, (void *)(v)); SETMARK(p);
  }
}

static void *bibop_alloc(arc *c, size_t osize)
{
  Bhdr *h, *bpage;
  size_t actual;
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
       c->mem_alloc is guaranteed to return aligned addresses, and
       since BiBOP objects inside the page are padded to a multiple
       of the alignment, all objects inside will also by definition
       be aligned. */
    actual = ALIGN_SIZE(osize) + BHDR_ALIGN_SIZE;
    bpage = (Bhdr *)c->mem_alloc(actual * BIBOP_PAGE_SIZE + BHDR_ALIGN_SIZE);
    if (bpage == NULL) {
      fprintf(stderr, "FATAL: failed to allocate memory for BiBOP page\n");
      exit(1);
    }
    BSSIZE(bpage, actual * BIBOP_PAGE_SIZE + BHDR_ALIGN_SIZE);
    bpage->_next = BIBOPPG(c)[osize];
    BIBOPPG(c)[osize] = bpage;
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
  USEDMEM(c) += osize;
  BSSIZE(h, osize);
  BALLOC(h);
  BSCOLOUR(h, mutator);		/* set to mutator colour by default */
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
  USEDMEM(c) += osize;
  BSSIZE(h, osize);
  BALLOC(h);
  BSCOLOUR(h, mutator);		/* set to mutator colour by default */
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

  USEDMEM(c) -= BSIZE(h);
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

/* The write barrier.  As required by VCGC, this marks the destination
   with the propagator. */
inline void __arc_wb(value dest, value src)
{
  MARKPROP(dest);
}

/* Maximum recursion depth for marking */
#define MAX_MARK_RECURSION 16

static void mark(arc *c, value v, int depth)
{
  typefn_t *tfn;
  Bhdr *h;

  if (TYPE(v) == T_SYMBOL && !NIL_P(c->symtable) && !NIL_P(c->rsymtable)) {
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

  D2B(h, (void *)v);

  /* special case: for a negative depth, just mark the object
     with mutator colour, do not recurse into it.  Presently used for
     thread stack marker. */
  if (depth < 0) {
    --VISIT(c);
    BSCOLOUR(h, mutator);
    return;
  }

  SETMARK(h);
  if (--VISIT(c) >= 0 && depth < MAX_MARK_RECURSION) {
    BSCOLOUR(h, mutator);
    /* Recurse into the object's structure at increased depth */
    tfn = __arc_typefn(c, v);
    tfn->marker(c, v, depth+1, mark);
  }
}

/* Go over all the allocated BiBOP pages and find ones which are
   completely empty. We have to rebuild the free lists for that
   size as well. */
static void free_unused_bibop(arc *c)
{
  Bhdr *bpage, *h, *prev, *nfl, *pfl, *pflt;
  char *bptr;
  int empty, i, j, actual;

  for (i=0; i<=MAX_BIBOP; i++) {
    actual = ALIGN_SIZE(i) + BHDR_ALIGN_SIZE;
    prev = NULL;
    nfl = NULL;
    for (bpage = BIBOPPG(c)[i]; bpage;) {
      bptr = B2D(bpage);
      empty = 1;
      pfl = pflt = NULL;
      for (j=0; j<BIBOP_PAGE_SIZE; j++) {
	h = (Bhdr *)bptr;
	if (BALLOCP(h)) {
	  empty = 0;
	  break;
	} else {
	  if (pflt == NULL)
	    pflt = h;
	  h->_next = pfl;
	  pfl = h;
	}
	bptr += actual;
      }
      h = B2NB(bpage);
      if (empty) {
	/* We are empty.  Unlink the page to be freed. */
	if (prev == NULL) {
	  BIBOPPG(c)[i] = h;
	} else {
	  prev->_next = h;
	}
	c->mem_free(bpage);
      } else {
	/* Not empty.  Previous pointer moves to it. */
	prev = bpage;
	/* Also join pfl to nfl if it isn't null */
	if (pflt != NULL && pfl != NULL) {
	  pflt->_next = nfl;
	  nfl = pfl;
	}
      }
      bpage = h;
    }
    /* Now that we have a new free list, use it */
    BIBOPFL(c)[i] = nfl;
  }
}

/* VCGC */

static int gc(arc *c)
{
  value v;
  unsigned long long gcst, gcet;
  int retval = 0;
  typefn_t *tfn;

  gcst = __arc_milliseconds();
  if (GCPTR(c) == NULL) {
    GCPTR(c) = ALLOCHEAD(c);
    GCPPTR(c) = CNIL;
  }

  for (VISIT(c) = MMVAR(c, gcquantum); VISIT(c) > 0;) {
    if (GCPTR(c) == NULL)
      break;			/* last heap block */
    v = (value)B2D(GCPTR(c));
    if (BCOLOUR(GCPTR(c)) == PROPAGATOR) {
      /* Recursively mark propagators */
      mark(c, v, 0);
    } else if (BCOLOUR(GCPTR(c)) == sweeper) {
      tfn = __arc_typefn(c, v);
      tfn->sweeper(c, v);
      GCPTR(c) = B2NB(GCPTR(c));
      c->free(c, (void *)v, GCPPTR(c));
      continue;
    }
    GCPPTR(c) = (void *)v;
    GCPTR(c) = B2NB(GCPTR(c));
  }

  if (GCPTR(c) != NULL)		/* completed iteration? */
    goto endgc;

  if (nprop == 0) { 		/* completed the epoch? */
    MMVAR(c, gcepochs)++;
    MMVAR(c, gccolour)++;
    mutator = MMVAR(c, gccolour) % 3;
    marker = (MMVAR(c, gccolour) - 1) % 3;
    sweeper = (MMVAR(c, gccolour) - 2) % 3;
    c->markroots(c);
    retval = 1;
    free_unused_bibop(c);
    malloc_trim(0);
  }
  nprop = 0;
 endgc:
  gcet = __arc_milliseconds();
  GCMS(c) += gcet - gcst;
  return(retval);
}

/* Default root marker */
static void markroots(arc *c)
{
  MARKPROP(c->symtable);
  MARKPROP(c->rsymtable);
  MARKPROP(c->genv);
  MARKPROP(c->builtins);
  MARKPROP(c->typedesc);
  MARKPROP(c->curthread);
  MARKPROP(c->vmthreads);
  MARKPROP(c->declarations);
#ifdef HAVE_TRACING
  MARKPROP(c->tracethread);
#endif
}

value arc_current_gc_milliseconds(arc *c)
{
  return(__arc_ull2val(c, GCMS(c)));
}

value arc_memory(arc *c)
{
  return(__arc_ull2val(c, USEDMEM(c)));
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

  for (i=0; i<=MAX_BIBOP; i++) {
    BIBOPFL(c)[i] = NULL;
    BIBOPPG(c)[i] = NULL;
  }
  ALLOCHEAD(c) = NULL;
  GCMS(c) = 0ULL;
  USEDMEM(c) = 0ULL;

  nprop = 0;
  MMVAR(c, gcepochs) = 0;
  MMVAR(c, gccolour) = 3;
  MMVAR(c, gcquantum) = 8192;	/* default GC quantum */
  GCPTR(c) = NULL;
  mutator = 0;
  marker = 1;
  sweeper = 2;
  __arc_handle = c;
}
