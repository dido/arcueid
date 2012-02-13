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

#ifndef _ALLOC_H_
#define _ALLOC_H_

#include <inttypes.h>
#include "arcueid.h"

#define MAGIC_A 0xa110c		      /* normal allocated block */
#define MAGIC_I	0xabba		      /* Block is immutable (non-GC) */
#define MAGIC_F 0xbadc0c0a	      /* Block is scheduled to be freed */

typedef struct Bhdr_t {
  uint64_t magic;		/* magic number */
  uint64_t size;		/* block size */
  uint64_t color;		/* gc color */
  struct Bhdr_t *next;		/* next block in list */
  struct Bhdr_t *prev;		/* previous block in list */
  void *block;			/* actual block (may be unaligned) */
  char data[1];			/* data */
} Bhdr;

#define B2D(bp) ((void *)bp->data)
#define D2B(b, dp) (b) = (Bhdr *)(((char *)dp) - (char *)(((Bhdr *)0)->data))
#define B2NB(b) ((Bhdr *)(b)->next)
#define BHDRSIZE ((long)(((Bhdr *)0)->data))
#define BLOCK_IMM(dp) { Bhdr *b; D2B(b, dp); b->magic = MAGIC_I; }
/* alignment */
#define ALIGN (1 << 4)

#define PROPAGATOR_COLOR 3	/* propagator color */
extern int nprop;		/* propagator flag */
extern int __arc_mutator;	/* mutator color */
#define SETMARK(h) if ((h)->color != __arc_mutator) { (h)->color = PROPAGATOR_COLOR; nprop=1; }
#define MARKPROP(b) if (!IMMEDIATE_P(b)) { Bhdr *p; D2B(p, (void *)(b)); SETMARK(p); }

static inline void WB(value *loc, value nval)
{
  MARKPROP(*loc);
  *loc = nval;
}

#endif
