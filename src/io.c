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

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
#ifndef alloca
# define alloca __builtin_alloca
#endif
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# include <stddef.h>
void *alloca (size_t);
#endif

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

#define IOW_TYPECHECK(fd)						\
  if (TYPE(fd) != T_OUTPORT) {			\
    arc_err_cstrfmt(c, "argument is not an output port");		\
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

static void io_marker(arc *c, value v, int depth,
		      void (*markfn)(arc *, value, int))
{
  IO(v)->io_tfn->marker(c, v, depth, markfn);
}

static void io_sweeper(arc *c, value v)
{
  IO(v)->io_tfn->sweeper(c, v);
}

#if 0
static value io_pprint(arc *c, value v, value *ppstr, value visithash)
{
  if (TYPE(v) == T_INPORT)
    __arc_append_cstring(c, "#<input-port:", ppstr);
  else if (TYPE(v) == T_OUTPORT)
    __arc_append_cstring(c, "#<output-port:", ppstr);
  else
    __arc_append_cstring(c, "#<unknown-port:", ppstr);
  if (!NIL_P(IO(v)->name))
    arc_prettyprint(c, IO(v)->name, ppstr, visithash);
  IO(v)->io_tfn->pprint(c, v, ppstr, visithash);
  __arc_append_cstring(c, ">", ppstr);
  return(*ppstr);
}
#endif

static unsigned long io_hash(arc *c, value v, arc_hs *s)
{
  return(IO(v)->io_tfn->hash(c, v, s));
}

AFFDEF0(arc_readb)
{
  AVAR(fd);

  Rune ch;
  AFBEGIN;

  if (arc_thr_argc(c, thr) == 0) {
    STDIN(AV(fd));
  } else if (arc_thr_argc(c, thr) == 1) {
    AV(fd) = arc_thr_pop(c, thr);
  } else {
    arc_err_cstrfmt(c, "readb: too many arguments");
    return(CNIL);
  }
  IO_TYPECHECK(AV(fd));
  CHECK_CLOSED(AV(fd));
  /* Note that if there is an unget value available, it will return
     the whole *CHARACTER*, not a possible byte within the character!
     As before, one isn't really supposed to mix the 'c' functions
     with the 'b' functions.  Do so at your own risk! */
  if (IO(AV(fd))->ungetrune >= 0) {
    ch = IO(AV(fd))->ungetrune;
    IO(AV(fd))->ungetrune = -1;
    return(INT2FIX(ch));
  }

  AFCALL(VINDEX(IO(AV(fd))->io_ops, IO_ready), AV(fd));
  if (AFCRV == CNIL) {
    arc_err_cstrfmt(c, "port is not ready for reading");
    return(CNIL);
  }
  AFCALL(VINDEX(IO(AV(fd))->io_ops, IO_getb), AV(fd));
  ARETURN(AFCRV);
  AFEND;
}
AFFEND

AFFDEF0(arc_readc)
{
  AVAR(fd, chr, buf, i, readb);
  char cbuf[UTFmax];    /* this is always destroyed */
  Rune ch;
  int j;
  AFBEGIN;

  if (arc_thr_argc(c, thr) == 0) {
    STDIN(AV(fd));
  } else if (arc_thr_argc(c, thr) == 1) {
    AV(fd) = arc_thr_pop(c, thr);
  } else {
    arc_err_cstrfmt(c, "readc: too many arguments");
    return(CNIL);
  }

  IO_TYPECHECK(AV(fd));
  CHECK_CLOSED(AV(fd));
  if (IO(AV(fd))->ungetrune >= 0) {
    ch = IO(AV(fd))->ungetrune;
    IO(AV(fd))->ungetrune = -1;
    ARETURN(arc_mkchar(c, ch));
  }
  if (IO(AV(fd))->flags & IO_FLAG_GETB_IS_GETC) {
    AFCALL(VINDEX(IO(AV(fd))->io_ops, IO_getb), AV(fd));
    if (NIL_P(AFCRV))
      ARETURN(CNIL);
    ARETURN(arc_mkchar(c, FIX2INT(AFCRV)));
  }
  AV(buf) = arc_mkvector(c, UTFmax);
  /* XXX - should put this in builtins */
  AV(readb) = arc_mkaff(c, arc_readb, CNIL);
  for (AV(i) = INT2FIX(0); FIX2INT(AV(i)) < UTFmax; AV(i) = INT2FIX(FIX2INT(i) + 1)) {
    AFCALL(AV(readb), AV(fd));
    AV(chr) = AFCRV;
    if (AV(chr) == CNIL)
      return(CNIL);
    VINDEX(AV(buf), FIX2INT(AV(i))) = AV(chr);
    /* Arcueid fixnum vector to C array of chars */
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

AFFDEF0(arc_writeb)
{
  AVAR(byte, fd);
  AFBEGIN;

  if (arc_thr_argc(c, thr) == 0) {
    arc_err_cstrfmt(c, "writeb: too few arguments");
    return(CNIL);
  }

  if (arc_thr_argc(c, thr) == 1) {
    STDOUT(AV(fd));
  } else if (arc_thr_argc(c, thr) == 1) {
    AV(fd) = arc_thr_pop(c, thr);
  } else {
    arc_err_cstrfmt(c, "writeb: too many arguments");
    return(CNIL);
  }
  AV(byte) = arc_thr_pop(c, thr);
  IOW_TYPECHECK(AV(fd));
  CHECK_CLOSED(AV(fd));
  AFCALL(VINDEX(IO(AV(fd))->io_ops, IO_wready), AV(fd));
  if (AFCRV == CNIL) {
    arc_err_cstrfmt(c, "port is not ready for writing");
    return(CNIL);
  }
  AFCALL(VINDEX(IO(AV(fd))->io_ops, IO_putb), AV(fd), AV(byte));
  ARETURN(AFCRV);
  AFEND;
}
AFFEND

AFFDEF0(arc_writec)
{
  AVAR(fd, chr, buf, i, writeb, nbytes);
  char cbuf[UTFmax];
  Rune ch;
  int j;
  AFBEGIN;

  if (arc_thr_argc(c, thr) == 0) {
    arc_err_cstrfmt(c, "writec: too few arguments");
    return(CNIL);
  }

  if (arc_thr_argc(c, thr) == 1) {
    STDOUT(AV(fd));
  } else if (arc_thr_argc(c, thr) == 2) {
    AV(fd) = arc_thr_pop(c, thr);
  } else {
    arc_err_cstrfmt(c, "writec: too many arguments");
    return(CNIL);
  }

  AV(chr) = arc_thr_pop(c, thr);
  IOW_TYPECHECK(AV(fd));
  CHECK_CLOSED(AV(fd));
  if (IO(AV(fd))->flags & IO_FLAG_GETB_IS_GETC) {
    AFCALL(VINDEX(IO(AV(fd))->io_ops, IO_putb), fd,
	   INT2FIX(arc_char2rune(c, AV(chr))));
    ARETURN(AFCRV);
  }
  /* XXX - should put this in builtins */
  AV(writeb) = arc_mkaff(c, arc_writeb, CNIL);
  ch = arc_char2rune(c, AV(chr));
  AV(nbytes) = INT2FIX(runetochar(cbuf, &ch));
  /* Convert C char array into Arcueid vector of fixnums */
  AV(buf) = arc_mkvector(c, FIX2INT(AV(nbytes)));
  for (j=0; j<FIX2INT(AV(nbytes)); j++)
    VINDEX(AV(buf), j) = INT2FIX(cbuf[j]);
  for (AV(i) = INT2FIX(0); FIX2INT(AV(i)) < FIX2INT(AV(nbytes)); AV(i) = INT2FIX(FIX2INT(i) + 1)) {
    AFCALL(AV(writeb), AV(fd), VINDEX(AV(buf), AV(i)));
  }
  ARETURN(AV(chr));
  AFEND;
}
AFFEND

AFFDEF0(arc_close)
{
  AVAR(fd, nargs, i);
  AFBEGIN;
  if (arc_thr_argc(c, thr) == 0) {
    arc_err_cstrfmt(c, "close: too few arguments");
    return(CNIL);
  }
  AV(nargs) = INT2FIX(arc_thr_argc(c, thr));
  for (AV(i) = INT2FIX(0); FIX2INT(AV(i)) < FIX2INT(AV(nargs)); AV(i) = INT2FIX(FIX2INT(i) + 1)) {
    AV(fd) = arc_thr_pop(c, thr);
    AFCALL(VINDEX(IO(AV(fd))->io_ops, IO_close), AV(fd));
  }
  ARETURN(CNIL);
  AFEND;
}
AFFEND

AFFDEF(arc_tell, fp)
{
  AFBEGIN;
  AFCALL(VINDEX(IO(AV(fp))->io_ops, IO_tell), AV(fp));
  ARETURN(AFCRV);
  AFEND;
}
AFFEND

AFFDEF0(arc_stdin)
{
  AVAR(fd);
  AFBEGIN;
  STDIN(AV(fd));
  ARETURN(AV(fd));
  AFEND;
}
AFFEND

AFFDEF0(arc_stdout)
{
  AVAR(fd);
  AFBEGIN;
  STDOUT(AV(fd));
  ARETURN(AV(fd));
  AFEND;
}
AFFEND

AFFDEF0(arc_stderr)
{
  AVAR(fd);
  AFBEGIN;
  STDERR(AV(fd));
  ARETURN(AV(fd));
  AFEND;
}
AFFEND

void arc_init_io(arc *c)
{
  __arc_init_sio(c); 
}

Rune arc_ungetc_rune(arc *c, Rune r, value fd)
{
  IO(fd)->ungetrune = r;
  return(r);
}

AFFDEF(arc_swrite, sexpr, visithash)
{
  typefn_t *tfn;
  AFBEGIN;

  if (NIL_P(AV(sexpr)))
    ARETURN(arc_mkstringc(c, "nil"));

  if (TYPE(AV(sexpr)) == T_TRUE)
    ARETURN(arc_mkstringc(c, "t"));
  if (TYPE(AV(sexpr)) == T_FIXNUM) {
    long val = FIX2INT(AV(sexpr));
    int len;
    char *outstr;

    len = snprintf(NULL, 0, "%ld", val) + 1;
    outstr = (char *)alloca(sizeof(char)*(len+2));
    snprintf(outstr, len+1, "%ld", val);
    ARETURN(arc_mkstringc(c, outstr));
  }
  if (TYPE(AV(sexpr)) == T_SYMBOL) {
    ARETURN(arc_sym2name(c, AV(sexpr)));
  }

  if (TYPE(AV(sexpr)) == T_NONE) {
    arc_err_cstrfmt(c, "invalid type for prettyprint");
    ARETURN(arc_mkstringc(c, ""));
  }

  /* Non-immediate type */
  tfn = __arc_typefn(c, AV(sexpr));
  if (tfn == NULL || tfn->pprint == NULL)
    ARETURN(arc_mkstringc(c, ""));
  AFCALL(arc_mkaff(c, tfn->pprint, CNIL), AV(sexpr), AV(visithash));
  AFEND;
}
AFFEND

typefn_t __arc_io_typefn__ = {
  io_marker,
  io_sweeper,
  NULL,
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
