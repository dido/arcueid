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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "carc.h"
#include "alloc.h"

static size_t heap_incr;	/* Minimum heap increment on alloc */
struct Shdr *carc_heap_head;	/* The actual heap. */
struct Bhdr *free_root;	/* The root of the Cartesian tree of free blocks */

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
  seg->fblk->size = size;
  seg->fblk->u.s.left = NULL;
  seg->fblk->u.s.right = NULL;
  endseg = B2NB(seg->fblk);
  endseg->magic = MAGIC_E;
  endseg->size = 0;
  return(seg);
}

void carc_alloc_init(size_t set_heap_incr)
{
  heap_incr = set_heap_incr;
  carc_heap_head = NULL;
  free_root = NULL;
}

static struct Bhdr *expand_heap(size)
{
  struct Shdr *new_seg;

  size = (size < heap_incr) ? heap_incr : size;
  new_seg = _carc_new_segment(size);
  /* Link it into the list of segments that make up the heap */
  new_seg->next = carc_heap_head;
  carc_heap_head = new_seg;
  return(new_seg->fblk);
}

#define FTREE_LEFT(n) ((n)->u.s.left)
#define FTREE_RIGHT(n) ((n)->u.s.right)
/* Determine whether two nodes are neighbors, i.e. b2 starts where b1 ends */
#define NEIGHBOR_P(b1, b2) ((value)(B2D(b1)) + b1->size == (value)b2)
/* Get the size of a tree, handling the case where the value may be
   null, in which case we set the size to zero. */
#define FTREE_SIZE(n) (((n) == NULL) ? 0 : (n)->size)
  
/*! \fn static struct Bhdr *ftree_alloc(size_t size)
  \brief Allocate */
static struct Bhdr *ftree_alloc(size_t size)
{
  struct Bhdr *node, *parent, *block, **parentptr;

  node = free_root;
  parentptr = &free_root;

  for (;;) {
    if (node == NULL)
      return(NULL);
    if (FTREE_SIZE(node) > size)
      break;
    parentptr = &FTREE_RIGHT(node);
    node = FTREE_RIGHT(node);
  }

  /* The traversal should have left us with some node which will fit
     the present node.  The first case is if the size of the new node is so
     close that splitting it would result in a block smaller than a Bhdr.
     In this case, we simply unlink the chosen node from the free
     list and provide it to the user. */
  if (FTREE_SIZE(node) - size < sizeof(struct Bhdr)) {
    *parentptr = FTREE_RIGHT(node); /* unlink */
    node->magic = MAGIC_A;
    return(node);
  }
  /* If the node is larger enough than the request, we should split
     the node into two nodes, one of which is the exact size of our
     request. */
  node->size -= size;		/* Resize the original node */
  block = B2NB(node);		/* Given the new size, get the
				   address of the new block */
  /* Set all of the parameters of the block */
  block->magic = MAGIC_A;
  block->size = size;
  return(block);
}

void *carc_heap_alloc(size_t size)
{
  struct Bhdr *hp, *new_block;

  hp = ftree_alloc(size);
  if (hp == NULL) {
    new_block = expand_heap(size);
    _carc_block_insert(&free_root, new_block);
    hp = ftree_alloc(size);
  }
  return(B2D(hp));
}

void carc_heap_free(void *ptr)
{
  struct Bhdr *block, *parent, *child, **parentptr, *releaselist = NULL;

  D2B(block, ptr);


}
