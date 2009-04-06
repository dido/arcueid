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

/*! \file alloc.h
  \brief Memory allocation data structures and functions

  The basic data structures used by the CArc memory manager are all
  here.  Similarities between this code and that of the Inferno
  memory manager should probably be unmissable.

  The memory allocator used herein is an adaptation of the Stephenson
  fast fits memory allocator using Cartesian trees.

 */

#include <inttypes.h>
#include <assert.h>

/*! This enum gives a list of the types of block that the system uses */
enum
{
  MAGIC_A = 0xa110c,	     /*!< Allocated block */
  MAGIC_F = 0xbadc0c0a,	     /*!< Free block */
  MAGIC_E = 0xdeadbabe,	     /*!< End of heap */
  MAGIC_I = 0xabba	     /*!< Block is immutable (hidden from gc) */
};

/*! \struct Bhdr
  \brief Block header used for memory management

  The memory manager and garbage collector used by CArc manages the heap
  by means of blocks prefixed with the following header.
  
 */
struct Bhdr {
  uint64_t magic;		/*!< magic number code of the block */
  size_t size;			/*!< size of the block */
  /*! \union u
    This union has the data member if the magic number is MAGIC_A or
    MAGIC_I (allocated or immutable), and has the s member if the
    magic number is MAGIC_F (free block).
   */
  union {
    /*! \struct d
      This member is used when the block is allocated (MAGIC_A or MAGIC_F). */
    uint8_t data[1];	      /*!< data in the block (if allocated) */
    /*! \struct s
      This is the tree structure by which the memory allocator organizes
      free blocks.  These free blocks are organized by means of a
      Cartesian tree.
     */
    struct {
      struct Bhdr *left;	/*!< left child of this node */
      struct Bhdr *right;	/*!< right child of this node */
    } s;
  } u;
};

/*! \struct Shdr
 \brief Segment header

 The heap of the CArc memory manager is structured as a linked list of
 memory segments. */
struct Shdr {
  void *block;		     /*!< address of the memory block this chunk lives in (alignment may make this different)  */
  size_t size;		     /*!< size of the segment in bytes */
  struct Shdr *next;	     /*!< next segment */
  struct Bhdr firstblock[1]; /*!< first block header in the segment */
};

/*! \var struct Shdr carc_heap_head
  \brief The start of the linked list of heap segments. */
extern struct Shdr *carc_heap_head;

/*! \def PAGE_SIZE
  \brief Size of a memory page
*/
#define PAGE_SIZE (1 << (PAGE_LOG))

/*! \def PAGE_LOG
  \brief The number of bits used to represent a page.
*/
#define PAGE_LOG (12)

/*! \def B2D(bp)
  \brief Given a block header, get a pointer to the data

  Given a pointer to a block header \a bp, this macro will produce a
  pointer to the data member of the block.
 */
#define B2D(bp) ((void *)(bp)->u.data)

/*! \def D2B(b, dp)
  \brief Given a data pointer, get the block header

  Given a pointer to a valid data pointer \a dp, put the address of the
  block header in \a b.  This will check to see if the magic numbers are
  valid, and assert whether the data are valid.
 */
#define D2B(b, dp) (b) = ((struct Bhdr *) \
			  (((uint8_t *)(dp)) - \
			   (((struct Bhdr *)0)->u.d.data))); \
		       assert((b)->magic == MAGIC_A || (b)->magic == MAGIC_I)

/*! \def B2NB(b)
  \brief Get the following block in a segment

  Given a Bhdr \a b, this macro will give the block following it in the
  heap.  This macro can be used to visit all the blocks of a segment.
*/
#define B2NB(b) ((struct Bhdr *)(((uint8_t *)(b) + (b)->size)))

/*! \fn void *carc_heap_alloc(size_t size)
  \brief allocate memory from the heap

  Allocate a block of memory from the heap of size \a size.
 */
void *carc_heap_alloc(size_t size);

/*! \fn void *carc_heap_free(void *ptr)
  \brief free a memory block back to the heap.
 */
void carc_heap_free(void *ptr);

