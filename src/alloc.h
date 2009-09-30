/* 
  Copyright (C) 2009 Rafael R. Sevilla

  This file is part of CArc

  CArc is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA
  02110-1301 USA.
*/

#ifndef _ALLOC_H_
#define _ALLOC_H_

#include <inttypes.h>
#include "carc.h"

/* These magic numbers are essentially the same as that used by
   Inferno in its memory allocator. */
#define MAGIC_A 0xa110c		      /* Allocated block */
#define MAGIC_F	0xbadc0c0a	      /* Free block */
#define MAGIC_E 0xdeadbabe	      /* End of arena */
#define MAGIC_I	0xabba		      /* Block is immutable (non-GC) */

typedef struct Bhdr_t {
  uint64_t magic;
  uint64_t size;
  uint64_t color;
  uint64_t pad;	    /* so that the block header is exactly 32 bytes */
  union {
    char data[1];
    struct Bhdr_t *next;
  } u;
} Bhdr;

typedef struct {
  void *block;	   /* address of malloced block this chunk lives in */
  size_t size;	   /* size in bytes */
  void *next;	   /* next pointer */
  long pad;	   /* padding so the size is exactly four words, 16 bytes
		      on 32-bit platforms or 32 bytes on 64-bit platforms. */
} Hhdr;

#define HHDR_SIZE(h) ((((Hhdr *)(h))[-1]).size)
#define HHDR_BLOCK(h) ((((Hhdr *)(h))[-1]).block)
#define HHDR_NEXT(h) ((((Hhdr *)(h))[-1]).next)

/* Page size is 4096 bytes */
#define PAGE_LOG 12
#define PAGE_SIZE (1 << PAGE_LOG)

#define B2D(bp) ((void *)bp->u.data)
#define D2B(b, dp) (b) = (Bhdr *)(((char *)dp) - (char *)(((Bhdr *)0)->u.data))
#define B2NB(b) ((Bhdr *)((char *)(b) + (b)->size + BHDRSIZE))
#define FBNEXT(b) ((b)->u.next)
#define BHDRSIZE ((long)(((Bhdr *)0)->u.data))
/* round to a multiple of 16 bytes to ensure alignment is maintained */
#define ROUNDSIZE(ns, s) { (ns) = ((s) & ~0x0f); (ns) = ((ns) < (s)) ? ((ns) + 0x10) : (ns); }
/* round a heap request size to a page size */
#define ROUNDHEAP(ns, s) { (ns) = ((s) & ~0x0fff); (ns) = ((ns) < (s)) ? ((ns) + 0x1000) : (ns); }
#define BLOCK_IMM(dp) { Bhdr *b; D2B(b, dp); b->magic = MAGIC_I; }

/* default to 30% minimum extra space on heap expansion */
#define DFL_OVER_PERCENT 30
/* default to 1 megabyte minimum heap expansion at any given time */
#define DFL_MIN_EXP 1048576

#define PROPAGATOR_COLOR 3	/* propagator color */
extern int nprop;		/* propagator flag */

static inline void write_barrier(value *loc, value nval)
{
  Bhdr *p;

  if (!(IMMEDIATE_P(*loc) || *loc == CNIL || *loc == CTRUE)) {
    D2B(p, (void *)*loc);
    p->color = PROPAGATOR_COLOR;
    nprop = 1;
  }
  *loc = nval;
}

extern void *__carc_aligned_mmap(size_t osize, int modulo, void **block);
extern void __carc_aligned_munmap(void *addr, size_t size);
extern void *__carc_aligned_malloc(size_t osize, int modulo, void **block);
extern void __carc_aligned_free(void *addr, size_t size);


#endif
