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
#include "arcueid.h"
#include "alloc.h"
#include "io.h"
#include "utf.h"

/* Delegate to type-specific I/O free function if any */
static void iofree(arc *c, value v)
{
  struct io_t *io = (struct io_t *)v;

  if (io->iot->free != NULL)
    io->iot->free(c, v);
}

/* Delegate to type-specific I/O marker if any, after marking ioops vector */
static void iomark(arc *c, value v,
		   void (*marker)(struct arc *, value, int),
		   int depth)
{
  struct io_t *io = (struct io_t *)v;

  marker(c, io->io_ops, depth);
  if (io->iot->mark != NULL)
    io->iot->mark(c, v, marker, depth);
}

static enum arc_trstate ioapply(arc *c, value t, value v)
{
  struct io_t *io = (struct io_t *)v;

  /* Delegate to a type-specific apply if any */
  if (io->iot->apply != NULL)
    return(io->iot->apply(c, t, v));
  arc_err_cstr(c, CNIL, "invalid argument for apply");
  return(TR_RC);
}


arctype __arc_io_t = { iofree, iomark, NULL, NULL, NULL, NULL, ioapply };

value __arc_allocio(arc *c, size_t xdsize, arctype *t, value ioops,
		    unsigned int flags)
{
  struct io_t *io;

  io = (struct io_t *)arc_new(c, &__arc_io_t, IOALIGNSIZE + ALIGN_SIZE(xdsize));
  io->ungetrune = -1;
  io->io_ops = ioops;
  io->iot = t;
  io->flags = flags;
  return((value)io);
}

void *__arc_iodata(value io)
{
  return(IODATA(io, void *));
}

/* Get one of the standard fd's */
static value get_stdfd(arc *c, value thr, value fdsym)
{
  value fd;

  fd = arc_cmark(c, thr, fdsym);
  if (NILP(fd))
    fd = arc_gbind(c, fdsym);
  return(fd);
}

AFFDEF(arc_readb)
{
  AOARG(fd);
  Rune ch;
  AFBEGIN;
  /* if fd is not passed, try to get stdin-fd */
  if (!BOUNDP(AV(fd)))
    WV(fd, get_stdfd(c, thr, arc_intern_cstr(c, "stdin-fd")));
  if (arc_type(AV(fd)) != &__arc_io_t) {
    arc_err_cstr(c, "argument is not an I/O port");
    ARETURN(CNIL);
  }
  if ((IO_FLAGS(AV(fd)) & IO_FLAG_READ) == 0) {
    arc_err_cstr(c, "argument is not an input port");
    ARETURN(CNIL);
  }
  AFCALL(IO_OP(AV(fd), IO_closed_p), AV(fd));
  if (!NILP(AFCRV)) {
    arc_err_cstr(c, "port is closed");
    ARETURN(CNIL);
  }
  /* Note that if there is an unget value available, it will return
     the whole *CHARACTER*, not a possible byte within the character!
     As before, one isn't really supposed to mix the 'c' functions
     with the 'b' functions.  Do so at your own risk! */
  if ((ch = IO_UNGETRUNE(AV(fd))) >= 0) {
    ((struct io_t *)AV(fd))->ungetrune = -1;
    ARETURN(INT2FIX(ch));
  }
  AFCALL(IO_OP(AV(fd), IO_ready), AV(fd));
  if (NILP(AFCRV)) {
    arc_err_cstr(c, "port is not ready for reading");
    ARETURN(CNIL);
  }
  AFTCALL(IO_OP(AV(fd), IO_getb), AV(fd));
  AFEND;
}
AFFEND

AFFDEF(arc_readc)
{
  AOARG(fd);
  AVAR(chr, buf, i);
  /* the following entries are always destroyed */
  char cbuf[UTFmax];
  Rune ch;
  int j;
  AFBEGIN;
  if (!BOUNDP(AV(fd)))
    WV(fd, get_stdfd(c, thr, arc_intern_cstr(c, "stdin-fd")));
  if (arc_type(AV(fd)) != &__arc_io_t) {
    arc_err_cstr(c, "argument is not an I/O port");
    ARETURN(CNIL);
  }
  if (!(IO_FLAGS(AV(fd)) & IO_FLAG_READ)) {
    arc_err_cstr(c, "argument is not an input port");
    ARETURN(CNIL);
  }
  AFCALL(IO_OP(AV(fd), IO_closed_p), AV(fd));
  if (!NILP(AFCRV)) {
    arc_err_cstr(c, "port is closed");
    ARETURN(CNIL);
  }
  if ((ch = IO_UNGETRUNE(AV(fd))) >= 0) {
    ((struct io_t *)AV(fd))->ungetrune = -1;
    ARETURN(arc_rune_new(c, ch));
  }
  AFCALL(IO_OP(AV(fd), IO_ready), AV(fd));
  if (NILP(AFCRV)) {
    arc_err_cstr(c, "port is not ready for reading");
    ARETURN(CNIL);
  }

  /* If getb is getc, we can use getb itself to read */
  if ((IO_FLAGS(AV(fd)) & IO_FLAG_GETB_IS_GETC)) {
    AFCALL(IO_OP(AV(fd), IO_getb), AV(fd));
    if (NILP(AFCRV))
      ARETURN(CNIL);
    ARETURN(arc_rune_new(c, FIX2INT(AFCRV)));
  }
  WV(buf, arc_vector_new(c, UTFmax));
  for (WV(i, INT2FIX(0)); FIX2INT(AV(i)) < UTFmax; WV(i, INT2FIX(FIX2INT(AV(i)) + 1))) {
    /* arc_aff_new will memoize an aff for arc_readb as required */
    AFCALL(arc_aff_new(c, arc_readb), AV(fd));
    WV(chr, AFCRV);
    if (NILP(AV(chr)))
      ARETURN(CNIL);
    SVIDX(c, AV(buf), FIX2INT(AV(i)), AV(chr));
    /* Arcueid fixnum vector to C array of chars */
    for (j=0; j<=FIX2INT(AV(i)); j++)
      cbuf[j] = FIX2INT(VIDX(AV(buf), j));
    if (fullrune(cbuf, FIX2INT(AV(i))+1)) {
      chartorune(&ch, cbuf);
      ARETURN(arc_rune_new(c, ch));
    }
  }
  ARETURN(CNIL);
  AFEND;
}
AFFEND
