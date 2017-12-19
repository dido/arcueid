/* 
  Copyright (C) 2017 Rafael R. Sevilla

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

/*! \file alloc.h
    \brief Definitions for Arcueid's memory allocator.    
 */

#ifndef _ALLOC_H_
#define _ALLOC_H_

#include "arcueid.h"

/*! \struct Bhdr
    \brief Memory block header

    Arcueid's memory allocator prefixes every allocated block with the
    Bhdr structure, which permits it to keep track of allocated memory.    
 */
struct Bhdr {
  unsigned long _size;		/*!< The size of the memory block. The
			          low order bit determines whether
			          the block is allocated or free. */
  union {
    struct Bhdr *_next;		/*!< The next memory block if free. */
    char _data[1];		/*!< A pointer to the data of the
				  block if allocated */
  } u;
};

/*! \def BHDRSIZE
    \brief Actual size of a block header.

    Excluding the dummy _data piece, the size of a Bhdr structure.
 */
#define BHDRSIZE ((long)(((struct Bhdr *)0)->u._data))

/* Alignment */

/*! \def ALIGN_BITS
    \brief Alignment bits

    The number of bits to align all structures and data against. This
    is set by default as 4 bits (16-byte alignment). Do not change it
    unless there is a good reason to do so.
 */
#define ALIGN_BITS 4

/*! \def ALIGN
    \brief Alignment mask
 */
#define ALIGN (1 << ALIGN_BITS)
/*! \def ALIGN_SIZE(size)
    \brief Macro to compute an aligned size for a structure
    Compute the closest value to _size_ that is greater than or equal to
    some multiple of the alignment bits.
*/
#define ALIGN_SIZE(size) ((size + ALIGN - 1) & ~(ALIGN - 1))
/*! \def BHDR_ALIGN_SIZE
    \brief Alignment size for a Bhdr
 */
#define BHDR_ALIGN_SIZE (ALIGN_SIZE(BHDRSIZE))
/*! \def ALIGN_PTR(ptr)
    \brief Align a pointer
    Compute the next pointer address that is aligned as above.
 */
#define ALIGN_PTR(ptr) ((void *)(((value)ptr + ALIGN - 1) & ~(ALIGN - 1)))

/*! \def BPAD
    \brief Block header padding.

    If ALIGN_SIZE is not the same as BHDRSIZE, actual data should
    begin at _data[BPAD] so that the data is properly aligned.
    Generally true on 32-bit architectures, but not on 64-bit
    architectures.
 */
#define BPAD (BHDR_ALIGN_SIZE - BHDRSIZE)

/*! \def B2D(bp)
    \brief Bhdr to data
    Given a Bhdr bp, get a pointer to the data it points to.  Only
    valid if the block is allocated.
 */
#define B2D(bp) ((void *)((bp)->u._data + BPAD))
/*! \def D2B(bp)
    \brief Data to Bhdr
    Given a pointer to data dp, assign the corresponding Bhdr to b.
 */
#define D2B(b, dp) (b) = (struct Bhdr *)(((char *)(dp)) - (char *)(((Bhdr *)0)->u._data + BPAD))
/*! \def B2NB(b)
    \brief Next Bhdr
    Get the next Bhdr in the linked list of free blocks. Only valid if
    the block is free.
 */
#define B2NB(b) ((struct Bhdr *)(b)->u._next)

/*! \def BSSIZE(bp, size)
    \brief Set Bhdr size
    Sets the size of a _Bhdr_ _bp_ to _size_. This will not affect the
    allocated/free status of the block.
 */
#define BSSIZE(bp, size) (bp)->_size = ((((bp)->_size) & 0x01) | ((size) << 1))
/*! \def BSIZE(bp)
    \brief Get Bhdr size
    Get the size of the _Bhdr_ _bp_. Masks out the allocated bit.
 */
#define BSIZE(bp) ((bp)->_size >> 1)

/*! \def BALLOC(bp)
    \brief Set a Bhdr to allocated
 */
#define BALLOC(bp) ((bp)->_size |= (0x1))
/*! \def BFREE(bp)
    \brief Set a Bhdr to free
 */
#define BFREE(bp) ((bp)->_size &= ~(0x1))
/*! \def BALLOCP(bp)
    \brief Predicate for whether a Bhdr is allocated.
 */
#define BALLOCP(bp) ((bp->_size & 0x1) == 0x1)

/*! \def MAX_BIBOP
    \brief Maximum size of objects subject to BiBOP allocation

    Memory blocks up to this defined size will be allocated using the
    BiBOP allocator. Anything larger will be allocated and freed using
    the standard OS allocator.
 */
#define MAX_BIBOP 512


/* \def BIBOP_PAGE_SIZE
   \brief Number of objects in each BiBOP page

   Each BiBOP page created will have at most this number of objects
   inside it.
 */
#define BIBOP_PAGE_SIZE 64

/*! \struct BBPhdr
    \brief BiBOP page header

    Metadata information about BiBOP pages. BiBOP pages are maintained
    as a linked list.
 */
struct BBPhdr {
  struct BBPhdr *next;		/*!< The next BiBOP header */
  char _dummy[1];		/*!< Dummy pointer */
};

/*! \def BBPHDRSIZE
    \brief Actual size of a BiBOP page header

    Excluding the _dummy piece, the size of a BBPhdr structure.
 */
#define BBPHDRSIZE ((long)(((struct BBPhdr *)0)->_dummy))

/*! \def BBPHDR_ALIGN_SIZE
    \brief Alignment size for a BBPhdr
 */
#define BBPHDR_ALIGN_SIZE (ALIGN_SIZE(BBPHDRSIZE))

/*! \def BBHPAD
    \brief BiBOP header padding
 */
#define BBHPAD (BBPHDR_ALIGN_SIZE - BBPHDRSIZE)


/*! \struct mm_ctx
    \brief Memory management context

    All memory manager related data are collected into this struct,
    which is passed to all memory allocation-related functions.
 */
struct mm_ctx {
  struct Bhdr *bibop_fl[MAX_BIBOP+1]; /*!< The BiBOP free lists */
  struct BBPhdr *bibop_pages[MAX_BIBOP+1]; /*!< The actual BiBOP pages */
  unsigned long long allocmem;		   /*!< memory allocated from
                                              the OS */
  unsigned long long usedmem;		   /*!< used memory */
};

/*! \fn extern void *__arc_init_mm_ctx(mm_ctx *c)
    \brief Initialise memory allocator context
    \param c The memory allocator context
 */
extern void __arc_init_mm_ctx(struct mm_ctx *c);

/*! \fn extern void *__arc_alloc(mm_ctx *c, size_t size)
    \brief Allocate memory
    \param c The memory allocator context
    \param size The size of the desired memory block

    Allocate memory.
 */
extern void *__arc_alloc(struct mm_ctx *c, size_t size);

/*! \fn extern void *__arc_free(mm_ctx *c, void *ptr)
    \brief Free memory
    \param c The memory allocator context
    \param size The memory block to be freed

    Free memory.
 */
extern void __arc_free(struct mm_ctx *c, void *ptr);

#endif
