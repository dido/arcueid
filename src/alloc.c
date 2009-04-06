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

/*!< \fn static Shdr *new_segment(size_t size, int modulo)
  \brief Allocate a new segment

  This method allocates a new segment with size \a size
 */
static Shdr *new_segment(size_t size)
{
  char *mem, *segbase;
  uint64_t aligned_mem;
  static void *last_addr = NULL;
  Bhdr *last_block;

  mem = (char *)mmap(last_addr, size + PAGE_SIZE, PROT_READ | PROT_WRITE,
		     MAP_PRIVATE | MAP_ANON, -1, 0);
  if (mem == MAP_FAILED) {
    perror("Failed to allocate segment\n");
    exit(1);
  }
  last_addr = mem + size + 2*PAGE_SIZE;
  segbase = mem + sizeof(Shdr);
  aligned_mem = (((uint64_t)segbase / PAGE_SIZE + 1) * PAGE_SIZE);
  segbase = (char *)(aligned_mem - sizeof(Shdr));
  segbase->block = (void *)mem;
  segbase->size = size;
  segbase->next = NULL;
  segbase->firstblock->magic = MAGIC_F;
  last_block = (last_addr - sizeof(struct Bhdr));
  segbase->firstblock->size = (size_t)((char *)last_block - (char *)segbase->firstblock);
  last_block->magic = MAGIC_E;
  last_block->size = 0;
  return(segbase);
}

static void demote(Bhdr *parent, Bhdr *child)
{
  Bhdr *left, *right;
  int r1;

  r1 = (parent->s.left == child);
  left = child->s.left;
  right = child->s.right;
  while (left->size > child->size || right->size > child->size) {
    if (right->size > child->size) {
    } else {
    }
  }
}
