/* Copyright (C) 2017, 2018 Rafael R. Sevilla

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

#ifndef __IO_H__

#define __IO_H__

#include "arcueid.h"
#include "utf.h"
#include "alloc.h"

/*! \enum io_ops
    \brief I/O operations

    Every I/O-type object is associated with a vector with the
    following functions (AFFs):

   0. IO_closedp - true or false based on whether the I/O object is
      closed or not.
   1. IO_ready - should always return true.  If an I/O source is not
      ready, for reading, it should use the AIOWAIT mechanism to wait,
      make the trampoline wait until it is, and then return when it is
      ready.  If it returns CNIL, that results in an error.
   2. IO_wready - Same as IO_ready, but for write operations.
   3. IO_getb - get a byte from the I/O source. Return CNIL on end of
      file 
   4. IO_putb - put a byte to the I/O source
   5. IO_seek - seek in the I/O source
   6. IO_tell - get the offset in the I/O source
   7. IO_close - close the I/O source

   The seek and tell functions may not be applicable for certain I/O
   sources, and they may throw an error in such cases.
 */
enum {
  IO_closed_p=0,
  IO_ready=1,
  IO_wready=2,
  IO_getb=3,
  IO_putb=4,
  IO_seek=5,
  IO_tell=6,
  IO_close=7,
  IO_last=7
};

/*! \struct io_t
    \brief Base I/O structure
    All I/O objects have the following header. Additional fields may
    be added for specific I/O objects.
*/
struct io_t {
  unsigned int flags;		/*!< flags  */
  Rune ungetrune;		/*!< Buffered unget rune  */
  value io_ops;			/*!< I/O operations functions  */
  arc_type *iot;		/*!< Specific I/O type (called after)  */
  char _data[1];		/*!< pointer to specific data of the I/O  */
};

/*! \def IOHDRSIZE
    \brief Actual size of an I/O header.

    Excluding the dummy _data piece, the size of a io_t structure.
 */
#define IOHDRSIZE ((long)(((struct io_t *)0)->_data))

/*! \def GCHDR_ALIGN_SIZE
    \brief Alignment size for an io_t
 */
#define IOALIGNSIZE (ALIGN_SIZE(IOHDRSIZE))

/*! \def IOPAD
    \brief I/O header padding
 */
#define IOPAD (IOALIGNSIZE - IOHDRSIZE)

/*! \def IO(v)
    \brief Value to io_t
 */
#define IO(v) ((struct io_t *)v)

/*! \def IODATA(v, t)
    \brief Specific I/O data
 */
#define IODATA(v, t) ((t)(((char *)(((struct io_t *)(v))->_data)) + GCHPAD))

/*! \def IO_OP(op)
    \brief Get the I/O op function
 */
#define IO_OP(v, op) (VIDX(IO(v)->io_ops, (op)))

#endif

