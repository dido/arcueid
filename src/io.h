/* 
  Copyright (C) 2013 Rafael R. Sevilla

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

/* These are the basic functions that all I/O derived objects should
   make available.  All of these should be AFFs.

   * IO_closed_p - true or false based on whether the I/O object is closed
     or not.
   * IO_ready - should always return true.  If an I/O source is not ready,
     for reading, it should use the AIOWAIT mechanism to wait, make the
     trampoline wait until it is, and then return when it is ready.  If it
     returns CNIL, that results in an error.
   * IO_wready - Same as IO_ready, but for write operations.
   * IO_getb - get a byte from the I/O source. Return CNIL on end of file
   * IO_putb - put a byte to the I/O source
   * IO_seek - seek in the I/O source
   * IO_tell - get the offset in the I/O source
   * IO_close - close the I/O source
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

/* getb actually returns a Unicode character rather than a byte */
#define IO_FLAG_GETB_IS_GETC 1

/* A basic I/O structure. */
struct io_t {
  unsigned int flags;
  value name;
  Rune ungetrune;
  struct typefn_t *io_tfn;
  value io_ops;
  char data[1];
};

#define IO(v) (((struct io_t *)REP(v)))
#define IODATA(v,t) ((t)((IO(v))->data))
#define IO_OP(op) (IO(v)->io_ops)

extern value __arc_allocio(arc *c, int type, struct typefn_t *tfn,
			   size_t xdsize);

extern void __arc_init_sio(arc *c);
extern void arc_init_io(arc *c);

enum {
  BI_io_strio=0,
  BI_io_fp=1,
  BI_io_last=1
};

#endif
