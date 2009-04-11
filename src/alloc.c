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
/* Get the size of a tree, handling the case where the value may be
   null, in which case we set the size to zero. */
#define FTREE_SIZE(n) (((n) == NULL) ? 0 : (n)->size)
  
static void ftree_insert(struct Bhdr *parent, struct Bhdr *new)
{
}

void _carc_ftree_demote(struct Bhdr **parentptr, struct Bhdr *child)
{
  struct Bhdr *rt, *lt;

  rt = FTREE_RIGHT(child);
  lt = FTREE_LEFT(child);
  while (FTREE_SIZE(rt) > FTREE_SIZE(child) ||
	 FTREE_SIZE(lt) > FTREE_SIZE(child)) {
    /* Compare the sizes of the left and right subtrees.  We look for
       the place to put the demoted node in the larger of the left or
       right subtrees. */
    if (FTREE_SIZE(rt) > FTREE_SIZE(lt)) {
      /* The new parent becomes the right subtree, and we traverse
	 down its left branch, assigning as we go.  To preserve the
	 Cartesian tree invariant of having ascending addresses on
	 inorder traversal, we have to go down the left branch. */
      *parentptr = rt;
      parentptr = &FTREE_LEFT(rt);
      rt = FTREE_LEFT(rt);
    } else {
      /* If the left subtree is bigger (or the right subtree is NULL),
	 we go down the left subtree's right branch instead. */
      *parentptr = lt;
      parentptr = &FTREE_RIGHT(lt);
      lt = FTREE_RIGHT(lt);
    }
  }
  /* At this point, the child to demote is larger than both the left
     and right subtrees, so we can now stop. */
  *parentptr = child;
  FTREE_LEFT(child) = lt;
  FTREE_RIGHT(child) = rt;
}

/* Delete a child node of \a parent.  This merges the rightmost path
   of the left subtree and the leftmost path of the right subtree. */
void _carc_ftree_delete(struct Bhdr **parentptr, struct Bhdr *child)
{
  struct Bhdr *rt, *lt, *temp;
  size_t cs;

  rt = FTREE_RIGHT(child);
  lt = FTREE_LEFT(child);

  while (rt != lt) {		/* until both are nil */
    if (FTREE_SIZE(lt) > FTREE_SIZE(rt)) {
      /* We should put a lock here */
      if (NEIGHBOR_P(lt, child)) {
	temp = FTREE_LEFT(lt);	/* neighbor => no right subtree */
	/* Combine the neighbors */
	cs = FTREE_SIZE(child) + BHDRSIZE;
	child = lt;
	child->size += cs;
	lt = temp;
      } else {
	*parentptr = lt;
	parentptr = &FTREE_RIGHT(lt);
	lt = FTREE_RIGHT(lt);
      }
    } else {
      if (NEIGHBOR_P(child, rt)) {
	temp = FTREE_RIGHT(rt);	/* neighbor => no left subtree */
	/* Combine the neighbors */
	cs = FTREE_SIZE(rt) + BHDRSIZE;
	child->size += cs;
	rt = temp;
      } else {
	*parentptr = rt;
	parentptr = &FTREE_LEFT(rt);
	rt = FTREE_LEFT(rt);
      }
    }
  }
  *parentptr = NULL;
}

/*! \fn static struct Bhdr *ftree_alloc(size_t size)
  \brief Allocate */
static struct Bhdr *ftree_alloc(size_t size)
{
  struct Bhdr *node, *parent, *block, **parentptr;

  node = parent = free_root;
  parentptr = &free_root;
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
      parentptr = &FTREE_RIGHT(parent);
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
      parentptr = &FTREE_LEFT(parent);
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
      parentptr = &FTREE_LEFT(parent);
      node = FTREE_LEFT(node);
      continue;
    }

    if (FTREE_LEFT(node)->size < size && FTREE_RIGHT(node)->size >= size) {
      parent = node;
      parentptr = &FTREE_RIGHT(parent);
      node = FTREE_RIGHT(node);
      continue;
    }

    /* If we get here, both left and right nodes are at least as large
       as the requested allocation.  Choose the node that is closer in
       size to our allocation request, so our allocation uses the
       better-fit algorithm.  TODO: when we make this concurrent, use
       random better fit instead. */
    parent = node;
    if (FTREE_LEFT(node)->size > FTREE_RIGHT(node)->size) {
      parentptr = &FTREE_RIGHT(parent);
      node = FTREE_RIGHT(node);
    } else {
      parentptr = &FTREE_LEFT(parent);
      node = FTREE_LEFT(node);
    }
  }

  /* The traversal should have left us with the node which is the
     closest fit to the present node.  The first case is if the size
     of the new node is so close that splitting it would result in a
     node smaller than a Bhdr.  In this case, we simply delete the
     chosen node from the ftree. */
  if (node->size - size < sizeof(struct Bhdr)) {
    _carc_ftree_delete(parent, node);
    node->magic = MAGIC_A;
    return(node);
  }
  /* If the node is larger than the request, we need to split the node
     into two nodes, one of which is the exact size of our request.
     This means that the remaining free node from which the node was
     split off from should probably be demoted in the tree. */
  node->size -= size;		/* Resize the original node */
  block = B2NB(node);		/* Given the new size, get the
				   address of the new block */
  /* Set all of the parameters of the block */
  block->magic = MAGIC_A;
  block->size = size;

  /* Demote the block that was carved out */
  _carc_ftree_demote(parentptr, node);
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
  struct Bhdr *block, *parent;

  D2B(block, ptr);

  parent = free_root;
}
