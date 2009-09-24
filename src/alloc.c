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
/* This default memory allocator and garbage collector uses a simple
   free-list to manage memory blocks, and the VCGC garbage collector
   by Huelsbergen and Winterbottom (ISMM 1998).  Someone's been reading
   the sources for Inferno emu and the OCaml bytecode interpreter! */
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include "carc.h"
#include "alloc.h"
#include "../config.h"

static Bhdr *fl_head = NULL;
static Bhdr *fl_prev = NULL;
static Hhdr *heaps = NULL;
static uint64_t epoch = 3;
static int mutator = 0;
static int marker = 1;
static int sweeper = 2;
#define propagator 3		/* propagator color */

/* This function takes an eligible block and carves out of it a portion
   of at least [size].  There are three cases:

   1. The free block is exactly the size requested.  Detach it from
      the free list and return it. 
   2. The additional space after carving out the free block is less than
      or equal to the size of a header.  The remaining space becomes
      slack (internal fragmentation), and the block is returned as is.
   3. The block is big enough.  Split it in two, and return the
      right-side block.
   In all cases the new allocated block is right-justified in the free
   block, in the higher address portions.  This way, the linking of the
   free list does not change in case 3.
 */
static void *fl_get_block(size_t size, Bhdr *prev, Bhdr *cur)
{
  Bhdr *h;

  if (cur->size <= (uint64_t)(size + BHDRSIZE)) {
    /* Cases 1 and 2, unlink the block and use it as is. */
    h = cur;
    FBNEXT(prev) = FBNEXT(cur);
  } else {
    /* Case 3, the block is bigger than what we need.  Shrink the
       current block by the size plus the size of the block header.
       Since we are carving away the right-hand side, the linkage
       of the free list does not change. */
    cur->size -= size + BHDRSIZE;
    h = B2NB(cur);
  }
  fl_prev = prev;
  h->size = size;
  h->magic = MAGIC_A;
  h->color = mutator;
  return(B2D(h));
}

/* Allocate memory for the heap.  This uses the low level memory allocator
   function specified in the carc structure.  Takes care of filling in the
   heap header information and adding the heap to the list of heaps. */
static void *alloc_for_heap(carc *c, size_t req)
{
  void *mem;
  void *block;

  mem = c->mem_alloc(req + sizeof(Hhdr), sizeof(Hhdr), &block);
  if (mem == NULL)
    return(NULL);
  mem += sizeof(Hhdr);
  HHDR_SIZE(mem) = req;
  HHDR_BLOCK(mem) = block;
  HHDR_NEXT(mem) = heaps;
  heaps = mem;
  return(mem);
}

/* Add a block to the free list.  The block's header fields are filled
   in by this function.  This also works to add a newly created block
   from expand_heap to the free list. */
static void fl_free_block(Bhdr *blk)
{
  Bhdr *prev, *cur;
  int inserted;

  /* The boundary of the freed block exactly coincides with the
     address of the head.  Coalesce the new block with the head
     and let it become the new head. */
  if (B2NB(blk) == fl_head) {
    blk->size += fl_head->size + BHDRSIZE;
    FBNEXT(blk) = fl_head;
    fl_head = blk;
    return;
  }

  /* If the free list head is less than the address of the head,
     insert the new block at the head.  This also covers the case
     where fl_head is NULL at the initial case, since any new
     block we get will not be a null pointer. */
  if (fl_head < blk) {
    FBNEXT(blk) = fl_head;
    fl_head = blk;
    return;
  }

  /* Neither of these initial cases is true, so now we have to
     search for the insertion point somewhere in the free list. */
  prev = fl_head;
  cur = FBNEXT(prev);
  inserted = 0;
  while (cur != NULL) {
    if (B2NB(prev) == blk) {
      /* end of previous block coincides with this block.  Coalesce
	 the freed block to it. */
      prev->size += blk->size + BHDRSIZE;
      blk = prev;
      inserted = 1;
    }
    if (B2NB(blk) == cur) {
      /* end of the block itself coincides with the start of the current
	 block.  Coalesce with the current block. */
      blk->size += cur->size + BHDRSIZE;
      FBNEXT(blk) = FBNEXT(cur);
      inserted = 1;
      cur = blk;
    }

    if (!inserted && prev < blk && cur > blk) {
      /* Cannot coalesce, just plain insert */
      FBNEXT(prev) = blk;
      FBNEXT(blk) = cur;
      inserted = 1;
    }
    if (inserted)
      return;
  }
  /* If we get here, we have reached the end of the free list.  The block
     must have a higher address than any other block already present in
     the free list.  Tack it onto the end. */
  assert(prev < blk);
  FBNEXT(blk) = FBNEXT(prev);
  FBNEXT(prev) = blk;
}

static void *fl_alloc(size_t size)
{
  Bhdr *prev, *cur;

  /* Search from [fl_prev] to the end of the list */
  prev = fl_prev;
  cur = FBNEXT(prev);
  while (cur != NULL) {
    if (cur->size >= size) {
      return(fl_get_block(size, prev, cur));
    }
    prev = cur;
    cur = FBNEXT(prev);
  }
  /* Search from the start of the list to fl_prev */
  prev = fl_head;
  cur = FBNEXT(prev);
  while (cur != fl_prev) {
    if (cur->size >= size) {
      return(fl_get_block(size, prev, cur));
    }
    prev = cur;
    cur = FBNEXT(prev);
  }
  /* No block found */
  return(NULL);
}

/* Allocate more memory using malloc/mmap.  This creates a new heap chunk,
   links it into the heap chunk list, and creates two block headers, one
   marking the new free block that fills the entire heap chunk and a second
   marking the end of the heap chunk.

   The heap expansion algorithm basically works by allocating
   [c->over_percent] more space than requested, plus the size of two
   block headers.  If the heap is smaller than the minimum expansion
   size, clamp it to the minimum expansion size.  It then rounds
   up the size of the chunk to the next larger multiple of the page
   size.
 */
static Bhdr *expand_heap(carc *c, size_t request)
{
  Bhdr *mem, *tail;
  size_t over_request, rounded_request;

  /* Allocate c->over_percent more beyond the requested amount plus
     space for the two headers, one for the header of the new free block
     and another for the tail block marking the end of the arena. */
  over_request = request + ((request / 100) * c->over_percent) + 2*BHDRSIZE;
  /* If less than minimum, expand to the minimum */
  if (over_request < c->minexp)
    over_request = c->minexp;
  ROUNDHEAP(rounded_request, over_request);
  mem = (Bhdr *)alloc_for_heap(c, rounded_request);
  if (mem == NULL) {
    c->signal_error(c, "No room for growing heap");
    return(NULL);
  }
  mem->magic = MAGIC_F;
  mem->size = rounded_request - BHDRSIZE;
  mem->color = mutator;
  /* Add a tail block to this new heap chunk so that the sweeper knows
     that it has reached the end of the heap chunk and should begin
     sweeping the next one, if any. */
  tail = B2NB(mem);
  tail->magic = MAGIC_E;
  tail->size = 0;
  return(mem);
}

static void *alloc(carc *c, size_t osize)
{
  void *blk;
  size_t size;
  Bhdr *nblk;

  /* Adjust the size of the allocated block so that we maintain
     alignment of at least 16 bytes. */
  ROUNDSIZE(size, osize);
  blk = fl_alloc(size);
  if (blk == NULL) {
    nblk = expand_heap(c, size);
    if (nblk == NULL) {
      c->signal_error(c, "Fatal error: out of memory!");
      return(NULL);
    }
    fl_free_block(nblk);
    blk = fl_alloc(size);
  }
  return(blk);
}

static value get_cell(carc *c)
{
  void *cellptr;

  cellptr = alloc(c, sizeof(struct cell));
  if (cellptr == NULL)
    return(CNIL);
  return((value)cellptr);
}

void carc_set_memmgr(carc *c)
{
  c->get_cell = get_cell;
  c->get_block = alloc;
#ifdef HAVE_MMAP
  c->mem_alloc = __carc_aligned_mmap;
  c->mem_free = __carc_aligned_munmap;
#else
  c->mem_alloc = __carc_aligned_malloc;
  c->mem_free = __carc_aligned_free;
#endif
  epoch = 3;
  mutator = 0;
  marker = 1;
  sweeper = 2;

  /* Set default parameters for heap expansion policy */
  c->minexp = DFL_MIN_EXP;
  c->over_percent = DFL_OVER_PERCENT;
}
