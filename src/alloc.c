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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
*/
#include <stdio.h>
#include <stdlib.h>
#include "carc.h"
#include "alloc.h"

static size_t heap_incr;	/* Minimum heap increment on alloc */
Shdr *carc_heap;		/* The actual heap. */
Bhdr *free_root;   /* The root of the Cartesian tree of free blocks */

/*!< \fn static Shdr *new_segment(size_t size, int modulo)
  \brief Allocate a new segment

  This method allocates a new segment which contains a free block of
  size \a size.  This allocates an additional end of segment block in
  addition.

  TODO: memory alignment?
 */
struct Shdr *_carc_new_segment(size_t size)
{
  char *mem;
  size_t fsize;
  struct Shdr *seg;
  struct Bhdr *endseg;

  fsize = size + sizeof(struct Shdr) + sizeof(struct Bhdr);

  mem = (char *) malloc(fsize);
  if (mem == NULL) {
    perror("Failed to allocate segment\n");
    exit(1);
  }

  seg = (struct Shdr *)mem;
  seg->size = fsize;
  seg->next = NULL;
  seg->fblk->magic = MAGIC_F;
  seg->fblk->size = size + BHDRSIZE;
  endseg = B2NB(seg->fblk);
  endseg->magic = MAGIC_E;
  endseg->size = 0;
  return(seg);
}

void carc_alloc_init(size_t set_heap_incr)
{
  heap_incr = set_heap_incr;
  carc_heap = free_root = NULL;
}

void *carc_heap_alloc(size_t size)
{
  Bhdr *hp, *new_block;

  hp = freelist_alloc(size);
  if (hp == NULL) {
    new_block = expand_heap(size);
    freelist_add(new_block);
    hp = freelist_alloc(size);
  }
  return(B2D(hp));
}
