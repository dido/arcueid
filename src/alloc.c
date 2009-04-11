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
#include <assert.h>
#include "carc.h"
#include "alloc.h"

static size_t heap_incr;	/* Minimum heap increment on alloc */
struct Shdr *carc_heap;		/* The actual heap. */
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
  carc_heap = NULL;
  free_root = NULL;
}

static struct Bhdr *expand_heap(size)
{
  struct Shdr *new_seg;

  size = (size < heap_incr) ? heap_incr : size;
  new_seg = _carc_new_segment(size);
  /* Link it into the list of segments that make up the heap */
  new_seg->next = carc_heap;
  carc_heap = new_seg;
  return(new_seg->fblk);
}

#define FTREE_LEFT(n) ((n)->u.s.left)
#define FTREE_RIGHT(n) ((n)->u.s.right)
/* Determine whether two nodes are neighbors, i.e. b2 starts where b1 ends */
#define NEIGHBOR_P(b1, b2) ((value)(B2D(b1)) + b1->size == (value)b2)

static void ftree_insert(struct Bhdr *parent, struct Bhdr *new)
{
}

/* Delete a child node of \a parent */
static void ftree_delete(struct Bhdr *parent, struct Bhdr *child)
{
  value c, p;
  int rl;
  struct Bhdr *lt, *rt;

  rl = ((value)parent > (value)child);
  lt = FTREE_LEFT(child);
  rt = FTREE_RIGHT(child);
  while (lt != rt) {		/* until both are NULL */
    if (lt->size > rt->size) {	/* The lt block is bigger */
      /* We should insert a lock here */
    } else {
    }
  }
}

static struct Bhdr *ftree_node_split(struct Bhdr *node, size_t size)
{
}

static void ftree_demote(struct Bhdr *parent, struct Bhdr *child)
{
  value p, c;
  int rl;
  struct Bhdr *rt, *lt;

  p = (value)parent;
  c = (value)child;
}

static struct Bhdr *ftree_alloc(size_t size)
{
  struct Bhdr *node, *parent, *block;

  node = parent = free_root;
  if (node == NULL || node->size < size) {
    /* root is empty or too small to allocate from */
    return(NULL);
  }

  for (;;) {
    if ((FTREE_LEFT(node) == NULL && FTREE_RIGHT(node) == NULL)) {
      /* We have reached a leaf node that has no children. Stop. */
      break;
    }

    if (FTREE_LEFT(node) == NULL && FTREE_RIGHT(node)->size >= size) {
      parent = node;
      node = FTREE_RIGHT(node);
      continue;
    }

    if (FTREE_LEFT(node) == NULL && FTREE_RIGHT(node)->size < size) {
      /* Both subtrees are ineligible, we're already at a node that
	 will work */
      break;
    }

    if (FTREE_RIGHT(node) == NULL && FTREE_LEFT(node)->size >= size) {
      parent = node;
      node = FTREE_LEFT(node);
      continue;
    }

    if (FTREE_RIGHT(node) == NULL && FTREE_LEFT(node)->size < size) {
      /* Both subtrees are ineligible, we're already at a node that 
	 will work */
      break;
    }

    if (FTREE_RIGHT(node)->size < size && FTREE_LEFT(node)->size >= size) {
      parent = node;
      node = FTREE_LEFT(node);
      continue;
    }

    if (FTREE_LEFT(node)->size < size && FTREE_RIGHT(node)->size >= size) {
      parent = node;
      node = FTREE_RIGHT(node);
      continue;
    }

    /* If we get here, both left and right nodes are at least as large
       as the requested allocation.  Choose the node that is closer in
       size to our allocation request. */
    parent = node;
    node = (FTREE_LEFT(node)->size > FTREE_RIGHT(node)->size) ? FTREE_RIGHT(node) : FTREE_LEFT(node);
  }

  /* The traversal should have left us with the node which is the
     closest fit to the present node.  The first case is if the size
     of the new node is so close that splitting it would result in a
     node smaller than a Bhdr.  In this case, we simply delete the
     chosen node from the ftree. */
  if (node->size - size < sizeof(struct Bhdr)) {
    ftree_delete(parent, node);
    return(node);
  }
  /* If the node is larger than the request, we need to split the node
     into two nodes, one of which is the exact size of our request.
     This means that the remaining free node from which the node was
     split off from should probably be demoted in the tree. */
  block = ftree_node_split(node, size);
  ftree_demote(parent, node);
  return(block);
}

void *carc_heap_alloc(size_t size)
{
  struct Bhdr *hp, *new_block;

  hp = ftree_alloc(size);
  if (hp == NULL) {
    new_block = expand_heap(size);
    ftree_insert(free_root, new_block);
    hp = ftree_alloc(size);
  }
  return(B2D(hp));
}

void carc_heap_free(void *ptr)
{
  struct Bhdr *block;

  D2B(block, ptr);

}
