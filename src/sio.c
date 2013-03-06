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

  String I/O support functions.

*/
#include <stdio.h>
#include "arcueid.h"
#include "builtins.h"
#include "io.h"

struct stringio_t {
  int closed;
  value str;
  int idx;
};

#define SIODATA(sio) (IODATA(sio, struct stringio_t *))

static void sio_marker(arc *c, value v, int depth,
		       void (*markfn)(arc *, value, int))
{
  markfn(c, SIODATA(v)->str, depth);
}

static value sio_pprint(arc *c, value v, value *ppstr, value visithash)
{
  __arc_append_cstring(c, "string", ppstr);
  return(*ppstr);
}

static unsigned long sio_hash(arc *c, value v, arc_hs *s, value vh)
{
  unsigned long len;

  len = arc_hash(c, SIODATA(v)->str, vh);
  len += arc_hash(c, INT2FIX(SIODATA(v)->idx), vh);
  return(len);
}

static typefn_t stringio_tfn = {
  sio_marker,
  __arc_null_sweeper,
  sio_pprint,
  sio_hash,
  NULL,
  NULL
};

static AFFDEF(sio_closed_p, sio)
{
  AFBEGIN;
  ARETURN((SIODATA(AV(sio))->closed) ? CTRUE : CNIL);
  AFEND;
}
AFFEND

/* This essentially always returns true */
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
  ARETURN((TYPE(AV(sio)) == T_OUTPORT) ? CTRUE : CNIL);
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
  if (SIODATA(AV(sio))->str == CNIL)
    SIODATA(AV(sio))->str = arc_mkstringlen(c, 0);
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

static value mkstringio(arc *c, int type, value string, value name)
{
  value sio;

  sio = __arc_allocio(c, type, &stringio_tfn, sizeof(struct stringio_t));
  IO(sio)->flags = IO_FLAG_GETB_IS_GETC;
  IO(sio)->io_ops = VINDEX(VINDEX(c->builtins, BI_io), BI_io_strio);
  IO(sio)->name = name;
  SIODATA(sio)->closed = 0;
  SIODATA(sio)->idx = 0;
  SIODATA(sio)->str = string;
  return(sio);
}

value arc_instring(arc *c, value string, value name)
{
  value sio = mkstringio(c, T_INPORT, string, name);
  return(sio);
}

value arc_outstring(arc *c, value name)
{
  return(mkstringio(c, T_OUTPORT, CNIL, name));
}

value arc_inside(arc *c, value sio)
{
  /* XXX type checks */
  return(SIODATA(sio)->str);
}

void __arc_init_sio(arc *c)
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
