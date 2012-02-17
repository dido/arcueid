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
/* This default memory allocator and garbage collector maintains a list
   of allocated blocks to do its work, and uses the VCGC garbage
   collection algorithm by Huelsbergen and Winterbottom (ISMM 1998). */
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include "arcueid.h"
#include "alloc.h"
#include "arith.h"
#include "vmengine.h"
#include "../config.h"

/* Maximum size of objects subject to BIBOP allocation */
#define MAX_BIBOP 512

/* Number of objects in each BIBOP page */
#define BIBOP_PAGE_SIZE 64

static int visit;

static Bhdr *bibop_fl[MAX_BIBOP+1];
static Bhdr *gcptr = NULL;
static Bhdr *alloc_head = NULL;
static Bhdr *alloc_tail = NULL;
static int gce = 0, gct = 1;
unsigned long long gcepochs = 0;
static unsigned long long gccolor = 3;
static unsigned long long gcnruns = 0;
static unsigned long long markcount = 0;
static unsigned long long sweepcount = 0;
static unsigned long long allocated = 0;
static unsigned long long freed = 0;
static unsigned long long scannedmem = 0;
static unsigned long long usedmem = 0;
static unsigned long long immutable = 0;
int nprop = 0;
int __arc_mutator = 0;
static int marker = 1;
static int sweeper = 2;
#define mutator __arc_mutator
#define propagator PROPAGATOR_COLOR
#define MAX_MARK_RECURSION 64
static unsigned long long gc_milliseconds = 0ULL;

Bhdr *__arc_get_heap_start(void)
{
  return(alloc_head);
}

static void free_bibop(struct arc *c, void *blk)
{
  Bhdr *h;

  D2B(h, blk);
  h->magic = MAGIC_B;
  h->next = bibop_fl[h->size];
  bibop_fl[h->size] = h;
}

static void free_block(struct arc *c, void *blk)
{
  Bhdr *h;
  size_t nsize;

  D2B(h, blk);
  if (h->prev == NULL)
    alloc_head = h->next;
  else
    h->prev->next = h->next;
  if (h->next == NULL)
    alloc_tail = h->prev;
  else
    h->next->prev = h->prev;
  nsize = h->size + BHDRSIZE + ALIGN - 1;
  freed += nsize;
  if (h->size <= MAX_BIBOP)
    free_bibop(c, blk);
  else
    c->mem_free(h->block);
}

static void *alloc(arc *c, size_t osize);

static void *bibop_alloc(arc *c, size_t osize)
{
  Bhdr *h;
  size_t actual;
  char *bpage, *bptr;
  int i;

  for (;;) {
    if (bibop_fl[osize] != NULL) {
      h = bibop_fl[osize];
      bibop_fl[osize] = B2NB(bibop_fl[osize]);
      break;
    }
    /* Create a new BIBOP page if the free list is empty */
    actual = osize + BHDRSIZE;
    /* round actual to the closest multiple of align */
    if ((actual % ALIGN) != 0)
      actual = ALIGN*((actual + ALIGN) / ALIGN);
    bpage = (char *)alloc(c, actual * BIBOP_PAGE_SIZE);
    BLOCK_IMM(bpage);
    bptr = bpage;
    for (i=0; i<BIBOP_PAGE_SIZE; i++) {
      h = (Bhdr *)bptr;
      h->magic = MAGIC_B;
      h->color = propagator;
      h->size = osize;
      h->next = bibop_fl[osize];
      bibop_fl[osize] = h;
      bptr += actual;
    }
    /* after this the free list for that size should be fine */
  }
  /* Perform the allocation actions on the new block gotten
     from the BIBOP list. */
  h->magic = MAGIC_A;
  h->size = osize;
  h->color = mutator;
  if (alloc_head == NULL && alloc_tail == NULL) {
    alloc_tail = alloc_head = h;
    alloc_head->prev = NULL;
    alloc_head->next = NULL;
  } else {
    assert(alloc_head != NULL && alloc_tail != NULL);
    h->prev = alloc_tail;
    h->next = alloc_tail->next;
    alloc_tail->next = h;
    alloc_tail = h;
  }
  return(B2D(h));
}

static void *alloc(arc *c, size_t osize)
{
  void *ptr;
  Bhdr *h;
  size_t actual;

  if (osize <= MAX_BIBOP)
    return(bibop_alloc(c, osize));

  /* Normal allocation */
  actual = osize + BHDRSIZE + ALIGN - 1;
  ptr = c->mem_alloc(actual);
  if (ptr == NULL) {
    fprintf(stderr, "FATAL: failed to allocate memory\n");
    exit(1);
  }
  allocated += actual;
  /* We have to position Bhdr inside the allocated memory
     such that Bhdr->data is at an aligned address. */
  D2B(h, (void *)(((value)ptr + BHDRSIZE + ALIGN - 1) & ~(ALIGN - 1)));
  h->magic = MAGIC_A;
  h->size = osize;
  h->color = mutator;
  h->block = ptr;
  if (alloc_head == NULL && alloc_tail == NULL) {
    alloc_tail = alloc_head = h;
    alloc_head->prev = NULL;
    alloc_head->next = NULL;
  } else {
    assert(alloc_head != NULL && alloc_tail != NULL);
    h->prev = alloc_tail;
    h->next = alloc_tail->next;
    alloc_tail->next = h;
    alloc_tail = h;
  }
  return(B2D(h));
}

static value get_cell(arc *c)
{
  void *cellptr;

  cellptr = alloc(c, sizeof(struct cell));
  if (cellptr == NULL)
    return(CNIL);
  return((value)cellptr);
}

static void mark(arc *c, value v, int reclevel, value marksym)
{
  Bhdr *b;
  int ctx;
  value val, *vptr;
  int i;

  /* If we find a symbol here, find its hash buckets in the symbol
     tables and mark those.  This provides symbol garbage collection,
     leaving only symbols which are actually in active use. */
  if (SYMBOL_P(v) && v != marksym) {
    value symid = INT2FIX(SYM2ID(v));
    value stbucket;

    stbucket = arc_hash_lookup2(c, c->rsymtable, symid);
    D2B(b, (void *)stbucket);  
    mark(c, stbucket, reclevel+1, v);
    val = stbucket;
    stbucket = arc_hash_lookup2(c, c->symtable, REP(val)._hashbucket.val);
    mark(c, stbucket, reclevel+1, v);
    return;
  }

  /* Do not try to mark an immediate value! */
  if (IMMEDIATE_P(v))
    return;

  D2B(b, (void *)v);
  /* do nothing with immutable blocks */
  if (b->magic == MAGIC_I)
    return;
  if (b->magic != MAGIC_A) {
    fprintf(stderr, "FATAL: internal error, pointer points to freed storage\n");
    exit(1);
  }
  SETMARK(b);
  if (--visit >= 0 && reclevel < MAX_MARK_RECURSION) {
    gce--;
    b->color = mutator;
    ++markcount;

    switch (TYPE(v)) {
    case T_NIL:
    case T_TRUE:
    case T_FIXNUM:
    case T_SYMBOL:
      /* never get here as these return true for IMMEDIATE_P above */
      break;
    case T_BIGNUM:
    case T_FLONUM:
    case T_RATIONAL:
    case T_COMPLEX:
    case T_CHAR:
    case T_STRING:
    case T_VMCODE:
      /* contain no internal pointers and are handled as is */
      break;
    case T_CONS:
    case T_CLOS:
    case T_TAGGED:
      mark(c, car(v), reclevel+1, CNIL);
      mark(c, cdr(v), reclevel+1, CNIL);
      break;
    case T_TABLE:
      ctx = 0;
      while ((val = arc_hash_iter(c, v, &ctx)) != CUNBOUND) {
	mark(c, val, reclevel+1, CNIL);
      }
      break;
    case T_CCODE:
      /* mark the function name */
      mark(c, REP(v)._cfunc.name, reclevel+1, CNIL);
      break;
    case T_TBUCKET:
      mark(c, REP(v)._hashbucket.key, reclevel+1, CNIL);
      mark(c, REP(v)._hashbucket.val, reclevel+1, CNIL);
      break;
    case T_THREAD:
      /* mark the registers inside of the thread */
      mark(c, TFUNR(v), reclevel+1, CNIL); /* function register */
      mark(c, TENVR(v), reclevel+1, CNIL); /* environment register */
      mark(c, TVALR(v), reclevel+1, CNIL); /* value register */
      mark(c, TCONR(v), reclevel+1, CNIL); /* continuation register */
      mark(c, TRVCH(v), reclevel+1, CNIL);  /* return value channel */
      mark(c, TCH(v), reclevel+1, CNIL);    /* dynamic-wind here */
      mark(c, TBCH(v), reclevel+1, CNIL);    /* base dynamic-wind here */
      mark(c, TCM(v), reclevel+1, CNIL);    /* continuation marks */
      mark(c, TEXH(v), reclevel+1, CNIL);   /* exception handlers */
      mark(c, TEXC(v), reclevel+1, CNIL);   /* async exception */
      /* Mark the stack *vector* only but not all its contents */
      D2B(b, (void *)TSTACK(v));
      b->color = mutator;
      /* Mark only the used portions of the stack vector */
      for (vptr = TSP(v); vptr == TSTOP(v); vptr++)
	mark(c, *vptr, reclevel+1, CNIL);
      break;
    case T_VECTOR:
    case T_CODE:
    case T_CONT:
    case T_CHAN:
    case T_XCONT:
    case T_EXCEPTION:
    case T_ENV:
      for (i=0; i<REP(v)._vector.length; i++)
	mark(c, REP(v)._vector.data[i], reclevel+1, CNIL);
      break;
    case T_INPUT:
    case T_OUTPUT:
    case T_PORT:
    case T_CUSTOM:
      /* for custom data types (including ports), call the marker
	 function defined for it, and pass ourselves as the next
	 level mark. */
      REP(v)._custom.marker(c, v, reclevel+1, mark);
      break;
      /* XXX fill in with other composite types as they are defined */
    case T_NONE:
      arc_err_cstrfmt(c, "GC: object of undefined type encountered!");
      break;
    }
  }
}

static void free_magic(arc *c, void *v)
{
  Bhdr *h;

  D2B(h, v);
  h->magic = MAGIC_F;
}

static void sweep(arc *c, value v)
{
  sweepcount++;
  /* The only special cases here are for those data types which point to
     immutable memory blocks which are otherwise invisible to the sweeper
     or allocate memory blocks not known to the allocator.  These include
     strings and hash tables (immutable memory) and bignums and rationals
     (use malloc/free directly I believe). */
  switch (TYPE(v)) {
  case T_STRING:
    /* a string's actual data is marked T_IMMUTABLE */
    free_magic(c, REP(v)._str.str);
    break;
#ifdef HAVE_GMP_H
  case T_BIGNUM:
    mpz_clear(REP(v)._bignum);
    break;
  case T_RATIONAL:
    mpq_clear(REP(v)._rational);
    break;
#endif
  case T_TABLE:
    free_magic(c, REP(v)._hash.table);
    break;
  case T_TBUCKET:
    /* Make the cell in the parent hash table undef if this is a symbol
       table's bucket. */
    if (REP(v)._hashbucket.hash == c->symtable
	|| REP(v)._hashbucket.hash == c->rsymtable)
      WB(&REP(REP(v)._hashbucket.hash)._hash.table[REP(v)._hashbucket.index], CUNDEF);
    break;
  case T_PORT:
  case T_CUSTOM:
    REP(v)._custom.sweeper(c, v);
    break;
  default:
    /* this should do for almost everything else */
    break;
  }
  free_magic(c, (void *)v);
}

static void rootset(arc *c)
{
  mutator = gccolor % 3;
  marker = (gccolor-1)%3;
  sweeper = (gccolor-2)%3;

  /* Mark the threads and global environment with propagators so that
     the virtual machine considers them part of the rootset.  Note that
     the symbol tables are *not* part of the rootset.  The symbol tables
     themselves are marked as immutable, so they are not subject to
     garbage collection, but the symbols inside them must be referenced
     elsewhere to prevent their being GCed. */
  MARKPROP(c->vmthreads);
  MARKPROP(c->genv);
  MARKPROP(c->builtin);
  MARKPROP(c->splforms);
  MARKPROP(c->inlfuncs);
  MARKPROP(c->iowaittbl);
  MARKPROP(c->achan);
}

static int rungc(arc *c)
{
  value h;
  unsigned long long gcst, gcet;
  int retval = 0;

  gcst = __arc_milliseconds();
  gcnruns++;

  if (gcptr == NULL) {
    immutable = usedmem = scannedmem = 0;
    gcptr = alloc_head;
  }

  for (visit = c->gcquantum; visit > 0;) {

    if (gcptr == NULL)
	break; 			/* stop if we finished the last heap block */
    scannedmem += gcptr->size + BHDRSIZE + ALIGN - 1;
    if (gcptr->magic == MAGIC_A) {
      visit--;
      gct++;
      h = (value)B2D(gcptr);
      if (gcptr->color == mutator) {
	usedmem += gcptr->size + BHDRSIZE + ALIGN - 1;
      } else if (gcptr->color == propagator) {
	gce--;
	mark(c, h, 0, CNIL);
      } else if (gcptr->color == sweeper) {
	gce++;
	sweep(c, h);
      }
      if (gcptr->color != sweeper)
	gcptr = B2NB(gcptr);
    } else if (gcptr->magic == MAGIC_I) {
      usedmem += gcptr->size + BHDRSIZE + ALIGN - 1;
      immutable += gcptr->size + BHDRSIZE + ALIGN - 1;
      gcptr = B2NB(gcptr);
    } else if (gcptr->magic == MAGIC_F) {
      Bhdr *next = B2NB(gcptr);
      free_block(c, (void *)B2D(gcptr));
      gcptr = next;
    } else {
      fprintf(stderr, "FATAL: Invalid magic number %lx", gcptr->magic);
      exit(1);
    }
  }

  if (gcptr != NULL)		/* completed this iteration? */
    goto endgc;

  if (nprop == 0) { /* completed the epoch? */
    gcepochs++;
    gccolor++;
    rootset(c);
    /*    printf("Epoch %lld ended:\n%lld marked, %lld swept, %lld bytes allocated, %lld bytes freed\n", gcepochs, markcount, sweepcount, allocated, freed);
	  printf("%lld bytes scanned, %lld bytes used, %lld bytes immutable\n", scannedmem, usedmem, immutable); */
    gce = 0;
    gct = 1;
    retval = allocated == freed; /* steady state condition */
    markcount = sweepcount = 0;
    freed = allocated = 0;
    malloc_trim(0);
  }
  nprop = 0;
 endgc:
  gcet = __arc_milliseconds();
  gc_milliseconds += (gcet - gcst);
  return(retval);
}

void arc_set_memmgr(arc *c)
{
  int i;

  c->get_cell = get_cell;
  c->get_block = alloc;
  c->free_block = free_block;
  c->mem_alloc = malloc;
  c->mem_free = free;
  c->rungc = rungc;

  gcepochs = 0;
  gccolor = 3;
  mutator = 0;
  marker = 1;
  sweeper = 2;

  gce = 0;
  gct = 1;

  for (i=0; i<=MAX_BIBOP; i++)
    bibop_fl[i] = NULL;

  /* Set default parameters for heap expansion policy */
  /*  c->minexp = DFL_MIN_EXP;
      c->over_percent = DFL_OVER_PERCENT; */
}

static value ull2val(arc *c, unsigned long long ms)
{
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

value arc_current_gc_milliseconds(arc *c)
{
  return(ull2val(c, gc_milliseconds));
}

value arc_memory(arc *c)
{
  return(ull2val(c, usedmem));
}
