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
/* This default memory allocator and garbage collector uses a simple
   free-list to manage memory blocks, and the VCGC garbage collector
   by Huelsbergen and Winterbottom (ISMM 1998).  Someone's been reading
   the sources for Inferno emu and the OCaml bytecode interpreter!
   We'll try using a more advanced memory allocator later on. */
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include "arcueid.h"
#include "alloc.h"
#include "arith.h"
#include "../config.h"

#define GC_QUANTA 16777216
#define MAX_GC_QUANTA GC_QUANTA

static int quanta = MAX_GC_QUANTA;
static int visit;
static Bhdr *gcptr = NULL;
static Bhdr *alloc_head = NULL;
static int gce = 0, gct = 1;
unsigned long long gcepochs = 0;
static unsigned long long gccolor = 3;
static unsigned long long gcnruns = 0;
static unsigned long long markcount = 0;
static unsigned long long sweepcount = 0;
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

#if 0

static void free_block(struct arc *c, void *blk)
{
  Bhdr *h;

  D2B(h, blk);
  if (h->prev == NULL)
    alloc_head = h->next;
  else
    h->prev->next = h->next;
  if (h->next != NULL)
    h->next->prev = h->prev;
  c->mem_free(h->block);
}

#else

static void free_block(struct arc *c, void *blk)
{
  Bhdr *h;
  int nsize;
  void *blk2;

  D2B(h, blk);
  if (h->prev == NULL)
    alloc_head = h->next;
  else
    h->prev->next = h->next;
  if (h->next != NULL)
    h->next->prev = h->prev;
  /* clear the entire block */
  nsize = h->size + BHDRSIZE + ALIGN - 1;
  blk2 = h->block;
  memset(blk2, 0xff, nsize);
  c->mem_free(blk2);
}

#endif

static void *alloc(arc *c, size_t osize)
{
  void *ptr;
  Bhdr *h;

  ptr = c->mem_alloc(osize + BHDRSIZE + ALIGN - 1);
  if (ptr == NULL) {
    fprintf(stderr, "Failed to allocate memory\n");
    exit(1);
  }
  /* We have to position Bhdr inside the allocated memory
     such that Bhdr->data is at an aligned address. */
  D2B(h, (void *)(((value)ptr + BHDRSIZE + ALIGN - 1) & ~(ALIGN - 1)));
  h->magic = MAGIC_A;
  h->size = osize;
  h->color = mutator;
  h->block = ptr;
  if (alloc_head == NULL) {
    alloc_head = h;
    alloc_head->prev = NULL;
    alloc_head->next = NULL;
  } else {
    h->prev = NULL;
    h->next = alloc_head;
    alloc_head->prev = h;
    alloc_head = h;
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

static void mark(arc *c, value v, int reclevel)
{
  Bhdr *b;
  int ctx;
  value val, *vptr;
  int i;

  /* If we find a symbol here, find its hash buckets in the symbol
     tables and mark those.  This provides symbol garbage collection,
     leaving only symbols which are actually in active use. */
  if (SYMBOL_P(v)) {
    value symid = INT2FIX(SYM2ID(v));

    val = arc_hash_lookup2(c, c->rsymtable, symid);
    mark(c, val, reclevel+1);
    val = arc_hash_lookup2(c, c->symtable, REP(val)._hashbucket.val);
    mark(c, val, reclevel+1);
    return;
  }

  /* Do not try to mark an immediate value! */
  if (IMMEDIATE_P(v))
    return;

  D2B(b, (void *)v);
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
    case T_ENV:
      mark(c, car(v), reclevel+1);
      mark(c, cdr(v), reclevel+1);
      break;
    case T_TABLE:
      ctx = 0;
      while ((val = arc_hash_iter(c, v, &ctx)) != CUNBOUND) {
	mark(c, val, reclevel+1);
      }
      break;
    case T_CCODE:
      /* mark the function name */
      mark(c, REP(v)._cfunc.name, reclevel+1);
      break;
    case T_TBUCKET:
      mark(c, REP(v)._hashbucket.key, reclevel+1);
      mark(c, REP(v)._hashbucket.val, reclevel+1);
      break;
    case T_THREAD:
      /* mark the registers inside of the thread */
      mark(c, TFUNR(v), reclevel+1); /* function register */
      mark(c, TENVR(v), reclevel+1); /* environment register */
      mark(c, TVALR(v), reclevel+1); /* value register */
      mark(c, TCONR(v), reclevel+1); /* continuation register */
      mark(c, TECONT(v), reclevel+1); /* error continuation */
      mark(c, TEXC(v), reclevel+1);   /* current exception */
      mark(c, TSTDH(v), reclevel+1);  /* standard handles */
      mark(c, TRVCH(v), reclevel+1);  /* return value channel */
      /* Mark the stack of this thread (used portions only) */
      for (vptr = TSP(v); vptr == TSTOP(v); vptr++)
	mark(c, *vptr, reclevel+1);
      break;
    case T_VECTOR:
    case T_CODE:
    case T_CONT:
    case T_CHAN:
    case T_XCONT:
    case T_EXCEPTION:
      for (i=0; i<REP(v)._vector.length; i++)
	mark(c, REP(v)._vector.data[i], reclevel+1);
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
    c->free_block(c, REP(v)._str.str); /* a string's actual data is marked T_IMMUTABLE */
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
    c->free_block(c, REP(v)._hash.table); /* free the immutable memory of the hash table */
    break;
  case T_TBUCKET:
    /* Make the cell in the parent hash table unbound as well. */
    REP(REP(v)._hashbucket.hash)._hash.table[REP(v)._hashbucket.index] = CUNBOUND;
    break;
  case T_PORT:
  case T_CUSTOM:
    REP(v)._custom.sweeper(c, v);
    break;
  default:
    /* this should do for almost everything else */
    break;
  }
  c->free_block(c, (void *)v);
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

static void rungc(arc *c)
{
  value h;
  unsigned long long gcst, gcet;
  Bhdr *cur;

  gcst = __arc_milliseconds();
  gcnruns++;

  /* We have to keep pressing */
  if (gcptr != NULL && nprop != 0)
    gcptr = NULL;

  if (gcptr == NULL)
    gcptr = alloc_head;

  for (visit = quanta; visit > 0;) {

    if (gcptr == NULL)
	break; 			/* stop if we finished the last heap block */
    /* The following operations may delete cur before we advance,
       so we advance now. */
    cur = gcptr;
    gcptr = B2NB(gcptr);
    if (cur->magic == MAGIC_A) {
      visit--;
      gct++;
      h = (value)B2D(cur);
      if (cur->color == propagator) {
	gce--;
	cur->color = mutator;
	mark(c, h, 0);
      } else if (cur->color == sweeper) {
	gce++;
	sweep(c, h);
      }
    }
  }

  quanta = MAX_GC_QUANTA;

  if (gcptr != NULL)		/* completed this iteration? */
    goto endgc;

  if (nprop == 0) {		/* completed the epoch? */
    printf("Epoch %lld ended:\n%lld marked, %lld swept\n", gcepochs,
	   markcount, sweepcount);
    markcount = sweepcount = 0;
    gcepochs++;
    gccolor++;
    rootset(c);
    gce = 0;
    gct = 1;
    goto endgc;
  }
  nprop = 0;
 endgc:
  gcet = __arc_milliseconds();
  gc_milliseconds += (gcet - gcst);
}

void arc_set_memmgr(arc *c)
{
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

  /* Set default parameters for heap expansion policy */
  /*  c->minexp = DFL_MIN_EXP;
      c->over_percent = DFL_OVER_PERCENT; */
}

value arc_current_gc_milliseconds(arc *c)
{
  unsigned long long ms;

  ms = gc_milliseconds;
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
