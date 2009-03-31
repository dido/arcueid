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

#include <inttypes.h>
#include <assert.h>

/*! This enum gives a list of the types of block that the system uses */
enum
{
  MAGIC_A = 0xa110c,	     /*!< Allocated block */
  MAGIC_F = 0xbadc0c0a,	     /*!< Free block */
  MAGIC_E = 0xdeadbabe,	     /*!< End of arena */
  MAGIC_I = 0xabba	     /*!< Block is immutable (hidden from gc) */
};

/*! \struct Bhdr
  \brief Block header used for memory management

  The memory management and garbage collector used by CArc manages the heap
  by means of blocks prefixed with the following header.
  
 */
struct Bhdr {
  uint64_t magic;		/*!< magic number code of the block */
  uint64_t size;		/*!< size of the block */
  /*! \union u
    This union has the data member if the magic number is MAGIC_A or
    MAGIC_I (allocated or immutable), and has the s member if the
    magic number is MAGIC_F (free block).
   */
  union {
    uint8_t data[1];		/*!< data in the block (if allocated) */
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
#define D2B(b, dp) (b) = ((Bhdr *)(((uint8_t *)(dp)) - (((Bhdr *)0)->u.data))); \
		       assert((b)->magic == MAGIC_A || (b)->magic == MAGIC_I)
