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
#include "arcueid.h"
#include "utf.h"
#include "builtins.h"
#include "io.h"

#define CHECK_CLOSED(fd)				\
  AFCALL(VINDEX(IO(fd)->io_ops, IO_closed_p), fd);	\
  if (AFCRV == CTRUE) {					\
    arc_err_cstrfmt(c, "port is closed");		\
    return(CNIL);					\
  }

#define IO_TYPECHECK(fd)						\
  if (TYPE(fd) != T_INPORT && TYPE(fd) != T_OUTPORT) {			\
    arc_err_cstrfmt(c, "argument is not an input or output port");	\
    return(CNIL);							\
  }

/* Make a bare I/O object.  Note that one must populate the io_ops structure
   to be able to use it. */
value __arc_allocio(arc *c, int type, struct typefn_t *tfn, size_t xdsize)
{
  value io;

  io = arc_mkobject(c, sizeof(struct io_t) - sizeof(char) + xdsize, type);
  IO(io)->name = CNIL;
  IO(io)->flags = 0;
  IO(io)->io_tfn = tfn;
  IO(io)->ungetrune = -1;
  IO(io)->io_ops = CNIL;
  return(io);
}

void arc_init_io(arc *c)
{
  __arc_init_sio(c);
}

static void io_marker(arc *c, value v, int depth,
		      void (*markfn)(arc *, value, int))
{
  IO(v)->io_tfn->marker(c, v, depth, markfn);
}

static void io_sweeper(arc *c, value v)
{
  IO(v)->io_tfn->sweeper(c, v);
}

static value io_pprint(arc *c, value v, value *ppstr, value visithash)
{
  if (TYPE(v) == T_INPORT)
    __arc_append_cstring(c, "#<input-port:", ppstr);
  else if (TYPE(v) == T_OUTPORT)
    __arc_append_cstring(c, "#<output-port:", ppstr);
  else
    __arc_append_cstring(c, "#<unknown-port:", ppstr);
  IO(v)->io_tfn->pprint(c, v, ppstr, visithash);
  __arc_append_cstring(c, ">", ppstr);
  return(*ppstr);
}

static unsigned long io_hash(arc *c, value v, arc_hs *s, value vh)
{
  return(IO(v)->io_tfn->hash(c, v, s, vh));
}

AFFDEF(arc_readb, fd)
{
  Rune ch;
  AFBEGIN;
  IO_TYPECHECK(fd);
  /* Note that if there is an unget value available, it will return
     the whole *CHARACTER*, not a possible byte within the character!
     As before, one isn't really supposed to mix the 'c' functions
     with the 'b' functions.  Do so at your own risk! */
  if (IO(AV(fd))->ungetrune >= 0) {
    ch = IO(AV(fd))->ungetrune;
    IO(AV(fd))->ungetrune = -1;
    return(INT2FIX(ch));
  }

  CHECK_CLOSED(AV(fd));
  AFCALL(VINDEX(IO(AV(fd))->io_ops, IO_ready), fd);
  if (AFCRV == CNIL) {
    arc_err_cstrfmt(c, "port is not ready");
    return(CNIL);
  }
  AFCALL(VINDEX(IO(AV(fd))->io_ops, IO_getb), fd);
  ARETURN(AFCRV);
  AFEND;
}
AFFEND

AFFDEF(arc_readc, fd)
{
  AVAR(chr, buf, i, readb);
  char cbuf[UTFmax];    /* this is always destroyed after every read */
  Rune ch;
  int j;
  AFBEGIN;
  AV(buf) = arc_mkvector(c, UTFmax);
  IO_TYPECHECK(fd);
  if (IO(AV(fd))->ungetrune >= 0) {
    ch = IO(AV(fd))->ungetrune;
    IO(AV(fd))->ungetrune = -1;
    ARETURN(arc_mkchar(c, ch));
  }
  CHECK_CLOSED(AV(fd));
  if (IO(AV(fd))->flags & IO_FLAG_GETB_IS_GETC) {
    AFCALL(VINDEX(IO(AV(fd))->io_ops, IO_getb), fd);
    ARETURN(AFCRV);
  }
  /* XXX - should put this in builtins */
  AV(readb) = arc_mkaff(c, arc_readb, CNIL);
  for (AV(i) = INT2FIX(0); FIX2INT(AV(i)) < UTFmax; AV(i) = INT2FIX(FIX2INT(i) + 1)) {
    AFCALL(readb, fd);
    AV(chr) = AFCRV;
    if (AV(chr) == CNIL)
      return(CNIL);
    VINDEX(AV(buf), FIX2INT(AV(i))) = AV(chr);
    /* Fixnum to C array of ints */
    for (j=0; j<=FIX2INT(AV(i)); j++)
      cbuf[j] = FIX2INT(VINDEX(AV(buf), j));
    if (fullrune(cbuf, FIX2INT(AV(i)) + 1)) {
      chartorune(&ch, cbuf);
      ARETURN(arc_mkchar(c, ch));
    }
  }
  ARETURN(CNIL);
  AFEND;
}
AFFEND

typefn_t __arc_io_typefn__ = {
  io_marker,
  io_sweeper,
  io_pprint,
  io_hash,
  /* XXX - this means that applying is or iso to any I/O port values
     will only return true if and only if they are the same object.
     This seems to be the case for string port I/O objects.  With
     reference Arc 3.1:

     arc> (= ins (instring "abc"))
     #<input-port:string>
     arc> (= ins2 (instring "abc"))
     #<input-port:string>
     arc> (is ins ins2)
     nil
     arc> (iso ins ins2)
     nil

     Verify if this is true in general, but it seems to be so.
  */

  NULL,
  NULL
};
