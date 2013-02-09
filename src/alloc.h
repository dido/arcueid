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

typedef struct Bhdr_t {
  unsigned long _size;		/* low 2 bits are mark/propagator */
  struct Bhdr_t *_next;
  void *_block;
  char _data[1];
} Bhdr;

#define B2D(bp) ((void *)bp->_data)
#define D2B(b, dp) (b) = (Bhdr *)(((char *)dp) - (char *)(((Bhdr *)0)->_data))
#define B2NB(b) ((Bhdr *)(b)->next)
#define BHDRSIZE ((long)(((Bhdr *)0)->data))

/* alignment */
#define ALIGN_BITS 4
#define ALIGN (1 << ALIGN_BITS)
#define ALIGN_SIZE(size) ((size + ALIGN - 1) & ~(ALIGN - 1))
#define BHDR_ALIGN_SIZE ALIGN_SIZE(BHDRSIZE)
#define ALIGN_PTR(ptr) ((void *)(((value)ptr + ALIGN - 1) & ~(ALIGN - 1)))

#endif
