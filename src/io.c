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

  Note: It is generally unsafe to mix the arc_readb/arc_writeb functions
  (which read and write individual bytes) with the arc_readc/arc_writec
  functions (which return Unicode runes).

  There are some idiosyncrasies in the behavior of string streams:
  the byte functions (readb and writeb) behave in exactly the same
  way as the character functions (readc and writec).  This is by
  design, although it might not be the way the PG-Arc reference
  implementation behaves.
*/
#include <stdio.h>
#include "arcueid.h"
#include "utf.h"
#include "vmengine.h"
#include "builtins.h"
#include "io.h"

/* Make a bare I/O object.  Note that one must populate the io_ops structure
   to be able to use it. */
value __arc_allocio(arc *c, value type, struct typefn_t *tfn, size_t xdsize,
		    int itype)
{
  value io;

  io = arc_mkobject(c, sizeof(struct io_t) - sizeof(char) + xdsize, itype);
  IO(io)->type = type;
  IO(io)->io_tfn = tfn;
  return(io);
}

/* String I/O functions */

#define SIO_READONLY 0
#define SIO_RDWR 1

struct stringio_t {
  int type;
  int closed;
  value str;
  int idx;
};

#define SIODATA(sio) (IODATA(sio, struct stringio_t *))

static AFFDEF(sio_closed_p, sio)
{
  AFBEGIN;
  return((SIODATA(AV(sio))->closed) ? CTRUE : CNIL);
  AFEND;
}
AFFEND

static AFFDEF(sio_ready, sio)
{
  AFBEGIN;
  ARETURN((SIODATA(AV(sio))->idx >= 0) ? CTRUE : CTRUE);
  AFEND;
}
AFFEND

static AFFDEF(sio_wready, sio)
{
  AFBEGIN;
  ARETURN((SIODATA(AV(sio))->type == SIO_RDWR) ? CTRUE : CNIL);
  AFEND;
}
AFFEND

static AFFDEF(sio_getb, sio)
{
  int len;
  Rune r;

  AFBEGIN;
  len = arc_strlen(c, SIODATA(AV(sio))->str);
  if (SIODATA(AV(sio))->idx >= len)
    ARETURN(CNIL);
  r = arc_strindex(c, SIODATA(AV(sio))->str, SIODATA(AV(sio))->idx++);
  ARETURN(INT2FIX(r));
  AFEND;
}
AFFEND

static AFFDEF(sio_putb, sio, byte)
{
  int len;

  AFBEGIN;
  len = arc_strlen(c, SIODATA(AV(sio))->str);
  if (SIODATA(AV(sio))->idx >= len) {
    SIODATA(AV(sio))->idx = len+1;
    SIODATA(AV(sio))->str = arc_strcatc(c, SIODATA(AV(sio))->str,
					(Rune)FIX2INT(AV(byte)));
  } else {
    arc_strsetindex(c, SIODATA(AV(sio))->str, SIODATA(AV(sio))->idx,
		    (Rune)FIX2INT(AV(byte)));
  }
  ARETURN(AV(byte));
  AFEND;
}
AFFEND

static AFFDEF(sio_seek, sio, offset, whence)
{
  int len, noffset;

  AFBEGIN;
  len = arc_strlen(c, SIODATA(AV(sio))->str);
  switch (FIX2INT(AV(whence))) {
  case SEEK_SET:
    noffset = FIX2INT(AV(offset));
    break;
  case SEEK_CUR:
    noffset = FIX2INT(AV(offset)) + len;
    break;
  case SEEK_END:
    noffset = len - FIX2INT(AV(offset));
    break;
  default:
    ARETURN(CNIL);
  }
  if (noffset >= len || noffset < 0)
    ARETURN(CNIL);
  SIODATA(AV(sio))->idx = noffset;
  ARETURN(CTRUE);
  AFEND;
}
AFFEND

static AFFDEF(sio_tell, sio)
{
  AFBEGIN;
  ARETURN(INT2FIX(SIODATA(AV(sio))->idx));
  AFEND;
}
AFFEND

static AFFDEF(sio_close, sio)
{
  AFBEGIN;
  SIODATA(AV(sio))->closed = 1;
  ARETURN(CNIL);
  AFEND;
}
AFFEND

void arc_init_io(arc *c)
{
  value io_ops;

  VINDEX(c->builtins, BI_io) = arc_mkvector(c, BI_io_last);
  io_ops = arc_mkvector(c, IO_last+1);
  VINDEX(io_ops, IO_closed_p) = arc_mkaff(c, sio_closed_p, CNIL);
  VINDEX(io_ops, IO_ready) = arc_mkaff(c, sio_ready, CNIL);
  VINDEX(io_ops, IO_wready) = arc_mkaff(c, sio_wready, CNIL);
  VINDEX(io_ops, IO_getb) = arc_mkaff(c, sio_getb, CNIL);
  VINDEX(io_ops, IO_putb) = arc_mkaff(c, sio_putb, CNIL);
  VINDEX(io_ops, IO_seek) = arc_mkaff(c, sio_seek, CNIL);
  VINDEX(io_ops, IO_tell) = arc_mkaff(c, sio_tell, CNIL);
  VINDEX(io_ops, IO_close) = arc_mkaff(c, sio_close, CNIL);
  VINDEX(VINDEX(c->builtins, BI_io), BI_io_strio) = io_ops;
}
