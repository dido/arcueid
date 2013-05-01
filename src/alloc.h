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

#ifndef _ALLOC_H_
#define _ALLOC_H_

#include "arcueid.h"

/* Memory block header */
typedef struct Bhdr_t {
  unsigned long _size;		/* low bit of _size is alloc/free flag */
  struct Bhdr_t *_next;
  char _data[1];
} Bhdr;

/* Actual size of a block header */
#define BHDRSIZE ((long)(((Bhdr *)0)->_data))

/* alignment */
#define ALIGN_BITS 4
#define ALIGN (1 << ALIGN_BITS)
#define ALIGN_SIZE(size) ((size + ALIGN - 1) & ~(ALIGN - 1))
#define BHDR_ALIGN_SIZE (ALIGN_SIZE(BHDRSIZE))
#define BIBOP_ALIGN_SIZE (ALIGN_SIZE(BIBOPPSIZE))
#define ALIGN_PTR(ptr) ((void *)(((value)ptr + ALIGN - 1) & ~(ALIGN - 1)))

/* Block header padding.  If ALIGN_SIZE is not the same as BHDRSIZE, actual
   data should begin at _data[BPAD] so that the data is properly
   aligned.  Generally true on 32-bit architectures, but not on 64-bit
   architectures. */
#define BPAD (BHDR_ALIGN_SIZE - BHDRSIZE)

#define B2D(bp) ((void *)((bp)->_data + BPAD))
#define D2B(b, dp) (b) = (Bhdr *)(((char *)(dp)) - (char *)(((Bhdr *)0)->_data + BPAD))
#define B2NB(b) ((Bhdr *)(b)->_next)

/* The size of a block includes not just the size but the following
   information in the low-order bits

   0 - Allocated or not flag (used only for BiBOP objects)
   1-2 - The object's GC colour.  A colour of 3 is the propagator.
   3+ - The object's actual size
 */
#define BSSIZE(bp, size) (bp)->_size = ((((bp)->_size) & 0x03) | ((size) << 3))
#define BSIZE(bp) ((bp)->_size >> 3)

/* Allocated flag */
#define BALLOC(bp) ((bp)->_size |= (0x1))
#define BFREE(bp) ((bp)->_size &= ~(0x1))
#define BALLOCP(bp) ((bp->_size & 0x1) == 0x1)

/* Colour */
#define BSCOLOUR(bp, colour) (bp)->_size = ((((bp)->_size) & ~0x06) | ((colour) << 1))
#define BCOLOUR(bp) ((((bp)->_size) >> 1) & 0x03)

/* Maximum size of objects subject to BiBOP allocation */
#define MAX_BIBOP 512

/* Number of objects in each BiBOP page */
#define BIBOP_PAGE_SIZE 64

struct mm_ctx {
  /* The BiBOP free lists */
  Bhdr *bibop_fl[MAX_BIBOP+1];
  /* The actual BiBOP pages */
  Bhdr *bibop_pages[MAX_BIBOP+1];

  /* The allocated list */
  Bhdr *alloc_head;

  /* GC statistics */
  unsigned long long gc_milliseconds;
  unsigned long long usedmem;

  /* variables used by VCGC */
  int gcquantum;		/* garbage collector visit max */
  unsigned long long gcepochs;	/* number of GC epochs */
  unsigned long long gccolour;	/* current GC colour */
  unsigned long long gcnruns;	/* number of GC runs */
  Bhdr *gcptr;			/* running pointer used by collector */
  void *gcpptr;			/* previous pointer */
  int visit;			/* visited node count for gc */
};

#define MMVAR(c, var) (((struct mm_ctx *)c->alloc_ctx)->var)
#define BIBOPFL(c) (MMVAR(c, bibop_fl))
#define BIBOPPG(c) (MMVAR(c, bibop_pages))
#define ALLOCHEAD(c) (MMVAR(c, alloc_head))
#define GCMS(c) (MMVAR(c, gc_milliseconds))
#define USEDMEM(c) (MMVAR(c, usedmem))
#define VISIT(c) (MMVAR(c, visit))
#define GCPTR(c) (MMVAR(c, gcptr))
#define GCPPTR(c) (MMVAR(c, gcpptr))

extern void __arc_markprop(arc *c, value p);
extern value arc_current_gc_milliseconds(arc *c);
extern value arc_memory(arc *c);
extern void arc_init_memmgr(arc *c);

#endif
