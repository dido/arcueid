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
#include <stdio.h>
#include "arcueid.h"
#include "io.h"

struct stringio_t {
  int closed;
  value str;
  int idx;
};

#define SIODATA(sio) (IODATA(sio, struct stringio_t *))

static void sio_marker(arc *c, value v,
		       void (*marker)(struct arc *, value, int),
		       int depth)
{
  marker(c, SIODATA(v)->str, depth);
}

/* Only sio_marker adds onto the default io type functions */
arctype __arc_sio_t = { NULL, sio_marker, NULL, NULL, NULL, NULL, NULL };

static AFFDEF(sio_closed_p)
{
  AARG(sio);
  AFBEGIN;
  ARETURN((SIODATA(AV(sio))->closed) ? CTRUE : CNIL);
  AFEND;
}
AFFEND

/* A sio is _always_ ready for reading*/
static AFFDEF(sio_ready)
{
  AARG(sio);
  AFBEGIN;
  AV(sio);
  ARETURN(CTRUE);
  AFEND;
}
AFFEND

static AFFDEF(sio_wready)
{
  AARG(sio);
  AFBEGIN;
  ARETURN(((IO_FLAGS(AV(sio)) & IO_FLAG_WRITE) == 0) ? CTRUE : CNIL);
  AFEND;  
}
AFFEND

static AFFDEF(sio_getb)
{
  AARG(sio);
  int len;
  Rune r;
  AFBEGIN;

  if ((IO_FLAGS(AV(sio)) & IO_FLAG_READ) == 0)
    return(CNIL);

  len = arc_strlen(c, SIODATA(AV(sio))->str);
  if (SIODATA(AV(sio))->idx >= len)
    ARETURN(CNIL);
  r = arc_strindex(c, SIODATA(AV(sio))->str, SIODATA(AV(sio))->idx++);
  ARETURN(INT2FIX(r));
  AFEND;
}
AFFEND

static AFFDEF(sio_putb)
{
  AARG(sio, byte);
  int len;
  AFBEGIN;
  if ((IO_FLAGS(AV(sio)) & IO_FLAG_WRITE) == 0)
    return(CNIL);
  if (NILP(SIODATA(AV(sio))->str))
    SIODATA(AV(sio))->str = arc_string_new(c, 0, 0);
  len = arc_strlen(c, SIODATA(AV(sio))->str);
  if (SIODATA(AV(sio))->idx >= len) {
    SIODATA(AV(sio))->idx = len+1;
    SIODATA(AV(sio))->str = arc_strcatrune(c, SIODATA(AV(sio))->str,
					   (Rune)FIX2INT(AV(byte)));
  } else {
    arc_strsetindex(c, SIODATA(AV(sio))->str, SIODATA(AV(sio))->idx++,
		    (Rune)FIX2INT(AV(byte)));
  }
  ARETURN(AV(byte));
  AFEND;
}
AFFEND

static AFFDEF(sio_seek)
{
  AARG(sio, offset, whence);
  int len, noffset;
  AFBEGIN;
  len = arc_strlen(c, SIODATA(AV(sio))->str);
  switch (FIX2INT(AV(whence))) {
  case SEEK_SET:
    noffset = FIX2INT(AV(offset));
    break;
  case SEEK_CUR:
    noffset = FIX2INT(AV(offset)) + SIODATA(AV(sio))->idx;
    break;
  case SEEK_END:
    noffset = len - FIX2INT(AV(offset));
    break;
  default:
    /* We should never get here because the arc_seek function does all
       argument checking. */
    ARETURN(CNIL);
  }
  if (noffset >= len || noffset < 0)
    ARETURN(CNIL);
  SIODATA(AV(sio))->idx = noffset;
  ARETURN(CTRUE);
  AFEND;
}
AFFEND

static AFFDEF(sio_tell)
{
  AARG(sio);
  AFBEGIN;
  ARETURN(INT2FIX(SIODATA(AV(sio))->idx));
  AFEND;
}
AFFEND

static AFFDEF(sio_close)
{
  AARG(sio);
  AFBEGIN;
  SIODATA(AV(sio))->closed = 1;
  ARETURN(CNIL);
  AFEND;
}
AFFEND

static value mkstringio(arc *c, unsigned int rwflags, value string)
{
  value sio;
  value ioops;

  ioops = __arc_tbl_lookup(c, c->builtins, arc_intern_cstr(c, "sio"));
  if (!BOUNDP(ioops)) {
    ioops = arc_vector_new(c, IO_last+1);
    SVIDX(c, ioops, IO_closed_p, arc_aff_new(c, sio_closed_p));
    SVIDX(c, ioops, IO_ready, arc_aff_new(c, sio_ready));
    SVIDX(c, ioops, IO_wready, arc_aff_new(c, sio_wready));
    SVIDX(c, ioops, IO_getb, arc_aff_new(c, sio_getb));
    SVIDX(c, ioops, IO_putb, arc_aff_new(c, sio_putb));
    SVIDX(c, ioops, IO_seek, arc_aff_new(c, sio_seek));
    SVIDX(c, ioops, IO_tell, arc_aff_new(c, sio_tell));
    SVIDX(c, ioops, IO_close, arc_aff_new(c, sio_close));
    __arc_tbl_insert(c, c->builtins, arc_intern_cstr(c, "sio"), ioops);
  }
  sio = __arc_allocio(c, sizeof(struct stringio_t), &__arc_sio_t, ioops,
		      IO_FLAG_GETB_IS_GETC | rwflags);

  SIODATA(sio)->closed = 0;
  SIODATA(sio)->idx = 0;
  SIODATA(sio)->str = string;
  return(sio);
}

value arc_instring(arc *c, value string)
{
  return(mkstringio(c, IO_FLAG_READ, string));
}

value arc_outstring(arc *c, value string)
{
  return(mkstringio(c, IO_FLAG_READ|IO_FLAG_WRITE, string));
}

value arc_inside(arc *c, value sio)
{
  /* XXX type checks */
  if (NILP(SIODATA(sio)->str))
    return(arc_string_new_cstr(c, ""));
  return(SIODATA(sio)->str);
}
