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
#include <sys/mman.h>
#include "carc.h"
#include "alloc.h"

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
  static void *last_addr = NULL;
  size_t fsize;
  struct Shdr *seg;
  struct Bhdr *endseg;

  fsize = size + sizeof(struct Shdr) + 2*sizeof(struct Bhdr);

  mem = (char *)mmap(NULL, fsize,
		     PROT_READ | PROT_WRITE,
		     MAP_PRIVATE | MAP_ANON, -1, 0);

  if (mem == MAP_FAILED) {
    perror("Failed to allocate segment\n");
    exit(1);
  }

  seg = (struct Shdr *)mem;
  seg->fblk->magic = MAGIC_F;
  seg->fblk->size = size;
  endseg = B2NB(seg->fblk);
  endseg->magic = MAGIC_E;
  endseg->size = 0;
  return(seg);
}

