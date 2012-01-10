/* 
  Copyright (C) 2010 Rafael R. Sevilla

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
#include <alloca.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include "arcueid.h"
#include "utf.h"
#include "io.h"
#include "symbols.h"
#include "../config.h"

#define SDEF(fn, defaultport)			\
  value arc_s##fn(arc *c, int argc, value *argv) \
  {						\
    value port;					\
    if (argc == 0) {				\
      port = defaultport;			\
    } else if (argc == 1) {			\
      port = argv[0];							\
    } else {								\
      arc_err_cstrfmt(c, #fn ": too many arguments (%d for 0 or 1)", argc); \
      return(CNIL);							\
    }									\
    return(arc_##fn(c, port));						\
  }

#define SDEF2(fn, defaultport)						\
  value arc_s##fn(arc *c, int argc, value *argv)			\
  {									\
    value port, ch;							\
    if (argc == 0) {							\
      arc_err_cstrfmt(c, #fn ": not enough arguments (%d for 1 or 2)", argc); \
      return(CNIL);							\
    } else if (argc == 1) {						\
      port = defaultport;						\
    } else if (argc == 2) {						\
      port = argv[1];							\
    } else {								\
      arc_err_cstrfmt(c, #fn ": too many arguments (%d for 1 or 2)", argc); \
      return(CNIL);							\
    }									\
    ch = argv[0];							\
    return(arc_##fn(c, ch, port));					\
  }

static value file_pp(arc *c, value v)
{
  value nstr = arc_mkstringc(c, "#<port:");

  nstr = arc_strcat(c, nstr, PORTF(v).name);
  nstr = arc_strcat(c, nstr, arc_mkstringc(c, ">"));
  return(nstr);
}

static void file_marker(arc *c, value v, int level,
			void (*markfn)(arc *, value, int))
{
  /* does nothing */
}

static void file_sweeper(arc *c, value v)
{
  if (PORTF(v).open) {
    fclose(PORTF(v).fp);
    /* XXX: error handling here? */
    PORTF(v).open = 0;
  }
  /* release memory */
  c->free_block(c, (void *)v);
}

static int file_getb(arc *c, struct arc_port *p)
{
  return(fgetc(p->u.file.fp));
}

static int file_putb(arc *c, struct arc_port *p, int byte)
{
  return(fputc(byte, p->u.file.fp));
}

static int file_seek(arc *c, struct arc_port *p, int64_t offset, int whence)
{
#ifdef HAVE_FSEEKO
  return(fseeko(p->u.file.fp, offset, whence));
#else
  return(fseek(p->u.file.fp, (long)offset, whence));
#endif
}

static int64_t file_tell(arc *c, struct arc_port *p)
{
#ifdef HAVE_FSEEKO
  return((int64_t)ftello(p->u.file.fp));
#else
  return((int64_t)ftell(p->u.file.fp));
#endif
}

static int file_close(arc *c, struct arc_port *p)
{
  int ret = 0;

  if (p->u.file.open) {
    ret = fclose(p->u.file.fp);
    p->u.file.open = 0;
  }
  return(ret);
}

value arc_filefp(arc *c, FILE *fp, value filename)
{
  void *cellptr;
  value fd;

  cellptr = c->get_block(c, sizeof(struct cell) + sizeof(struct arc_port));
  if (cellptr == NULL)
    arc_err_cstrfmt(c, "openfile: cannot allocate memory");
  fd = (value)cellptr;
  BTYPE(fd) = T_PORT;
  REP(fd)._custom.pprint = file_pp;
  REP(fd)._custom.marker = file_marker;
  REP(fd)._custom.sweeper = file_sweeper;
  PORT(fd)->type = FT_FILE;
  /* XXX make this file name a fully qualified pathname */
  PORTF(fd).name = filename;
  PORTF(fd).fp = fp;
  PORTF(fd).open = 1;
  PORT(fd)->getb = file_getb;
  PORT(fd)->putb = file_putb;
  PORT(fd)->seek = file_seek;
  PORT(fd)->tell = file_tell;
  PORT(fd)->close = file_close;
  PORT(fd)->ungetrune = -1;	/* no rune available */
  return(fd);
}

static value openfile(arc *c, value filename, const char *mode)
{
  char *utf_filename;
  FILE *fp;
  int en;

  utf_filename = alloca(FIX2INT(arc_strutflen(c, filename)) + 1);
  arc_str2cstr(c, filename, utf_filename);
  fp = fopen(utf_filename, mode);
  if (fp == NULL) {
    en = errno;
    arc_err_cstrfmt(c, "openfile: cannot open input file \"%s\", (%s; errno=%d)", utf_filename, strerror(en), en);
  }
  return(arc_filefp(c, fp, filename));
}

value arc_infile(arc *c, value filename)
{
  return(openfile(c, filename, "r"));
}

value arc_outfile(arc *c, value filename, value xmode)
{
  if (xmode == arc_intern_cstr(c, "append"))
    return(openfile(c, filename, "a"));
  return(openfile(c, filename, "w"));
}

static value fstr_pp(arc *c, value v)
{
  return(arc_mkstringc(c, "#<port:string>"));
}

static void fstr_marker(arc *c, value v, int level,
			void (*mark)(arc *, value, int))
{
  mark(c, PORTS(v).str, level+1);
}

static void fstr_sweeper(arc *c, value v)
{
  /* release memory */
  c->free_block(c, (void *)v);
}

/* NOTE: this will actually return a RUNE, not a byte! */
static int fstr_getb(arc *c, struct arc_port *p)
{
  int len;

  len = arc_strlen(c, p->u.strfile.str);
  if (p->u.strfile.idx >= len)
    return(EOF);
  return(arc_strindex(c, p->u.strfile.str, p->u.strfile.idx++));
}

/* NOTE: this will actually write a RUNE, not a byte! */
static int fstr_putb(arc *c, struct arc_port *p, int byte)
{
  int len;

  len = arc_strlen(c, p->u.strfile.str);
  if (p->u.strfile.idx >= len) {
    p->u.strfile.idx = len+1;
    p->u.strfile.str = arc_strcatc(c, p->u.strfile.str, (Rune)byte);
  } else {
    arc_strsetindex(c, p->u.strfile.str, p->u.strfile.idx, (Rune)byte);
  }
  return(byte);
}

static int fstr_seek(arc *c, struct arc_port *p, int64_t offset, int whence)
{
  int64_t len;

  len = (int64_t)arc_strlen(c, p->u.strfile.str);
  switch (whence) {
  case SEEK_SET:
    break;
  case SEEK_CUR:
    offset += p->u.strfile.idx;
    break;
  case SEEK_END:
    offset = len - offset;
    break;
  default:
    return(-1);
  }
  if (offset >= len || offset < 0)
    return(-1);
  p->u.strfile.idx = offset;
  return(0);
}

static int64_t fstr_tell(arc *c, struct arc_port *p)
{
  return(p->u.strfile.idx);
}

static int fstr_close(arc *c, struct arc_port *p)
{
  /* essentially a no-op */
  return(0);
}

value arc_instring(arc *c, value str)
{
  void *cellptr;
  value fd;

  cellptr = c->get_block(c, sizeof(struct cell) + sizeof(struct arc_port));
  if (cellptr == NULL)
    arc_err_cstrfmt(c, "openstring: cannot allocate memory");
  fd = (value)cellptr;
  BTYPE(fd) = T_PORT;
  REP(fd)._custom.pprint = fstr_pp;
  REP(fd)._custom.marker = fstr_marker;
  REP(fd)._custom.sweeper = fstr_sweeper;
  PORT(fd)->type = FT_STRING;
  PORTS(fd).idx = 0;
  PORTS(fd).str = str;
  PORT(fd)->getb = fstr_getb;
  PORT(fd)->putb = fstr_putb;
  PORT(fd)->seek = fstr_seek;
  PORT(fd)->tell = fstr_tell;
  PORT(fd)->close = fstr_close;
  PORT(fd)->ungetrune = -1;	/* no rune available */
  return(fd);
}

value arc_outstring(arc *c)
{
  return(arc_instring(c, arc_mkstringc(c, "")));
}

value arc_fstr_inside(arc *c, value fstr)
{
  return(PORTS(fstr).str);
}

/* Read a byte.  Returns -1 on failure. */
value arc_readb(arc *c, value fd)
{
  int ch;

  /* Note that if there is an unget value available, it will return
     the whole *CHARACTER*, not a possible byte within the character!
     As before, one isn't really supposed to mix the 'c' functions
     with the 'b' functions.  Do so at your own risk! */
  if (PORT(fd)->ungetrune >= 0) {
    ch = PORT(fd)->ungetrune;
    PORT(fd)->ungetrune = -1;
    return(INT2FIX(ch));
  }

  ch = PORT(fd)->getb(c, PORT(fd));
  if (ch == EOF)
    ch = -1;
  return(INT2FIX(ch));
}

SDEF(readb, arc_stdin(c));

/* Write a byte.  Returns -1 on failure. */
value arc_writeb(arc *c, value byte, value fd)
{
  int ch = FIX2INT(byte);

  PORT(fd)->putb(c, PORT(fd), ch);
  if (ch == EOF)
    ch = -1;
  return(INT2FIX(ch));
}

SDEF2(writeb, arc_stdout(c));

/* Read a rune from fd.  On end of file returns -1.  If UTF-8
   cannot be decoded properly, returns Runeerror. */
Rune arc_readc_rune(arc *c, value fd)
{
  int ch;
  char buf[UTFmax];
  int i;
  Rune r;

  if (PORT(fd)->ungetrune >= 0) {
    r = PORT(fd)->ungetrune;
    PORT(fd)->ungetrune = -1;
    return(r);
  }

  if (PORT(fd)->type == FT_STRING)
    return(FIX2INT(arc_readb(c, fd)));

  for (i=0; i<UTFmax; i++) {
    ch = PORT(fd)->getb(c, PORT(fd));
    if (ch == EOF)
      return(-1);
    buf[i] = ch;
    if (fullrune(buf, i+1)) {
      chartorune(&r, buf);
      return(r);
    }
  }
  return(-1);
}

/* Read a character from fd.  Returns CNIL on end of file, or the
   character. */
value arc_readc(arc *c, value fd)
{
  Rune r;

  r = arc_readc_rune(c, fd);
  if (r < 0)
    return(CNIL);
  return(arc_mkchar(c, r));
}

SDEF(readc, arc_stdin(c));

Rune arc_writec_rune(arc *c, Rune r, value fd)
{
  char buf[UTFmax];
  int nbytes, i;

  if (PORT(fd)->type == FT_STRING) {
    PORT(fd)->putb(c, PORT(fd), r);
    return(r);
  }

  nbytes = runetochar(buf, &r);
  for (i=0; i<nbytes; i++)
    PORT(fd)->putb(c, PORT(fd), buf[i]);
  return(r);
}

value arc_writec(arc *c, value r, value fd)
{
  arc_writec_rune(c, REP(r)._char, fd);
  return(r);
}

SDEF2(writec, arc_stdout(c));

Rune arc_ungetc_rune(arc *c, Rune r, value fd)
{
  PORT(fd)->ungetrune = r;
  return(r);
}

/* Note that ungetc is a rather simplistic function. */
value arc_ungetc(arc *c, value r, value fd)
{
  arc_ungetc_rune(c, REP(r)._char, fd);
  return(r);
}

SDEF2(ungetc, arc_stdin(c));

Rune arc_peekc_rune(arc *c, value fd)
{
  Rune r;

  r = arc_readc_rune(c, fd);
  arc_ungetc_rune(c, r, fd);
  return(r);
}

SDEF(peekc, arc_stdin(c));

value arc_peekc(arc *c, value fd)
{
  Rune r;

  r = arc_peekc_rune(c, fd);
  return(arc_mkchar(c, r));
}

/* XXX - probably need to define some constants for whence
   XXX - probably need to check FIX2INT values as well.  Some 32/64-bit
   compatibility issues may arise (e.g. no large files on 32-bit
   systems) */
value arc_seek(arc *c, value fd, value ofs, value whence)
{
  if (PORT(fd)->seek(c, PORT(fd), FIX2INT(ofs), FIX2INT(whence)) < 0)
    return(CNIL);
  return(CTRUE);
}

value arc_tell(arc *c, value fd)
{
  int64_t pos;

  pos = PORT(fd)->tell(c, PORT(fd));
  if (pos < 0)
    return(CNIL);
  return(INT2FIX(pos));
}

value arc_close(arc *c, value fd)
{
  if (PORT(fd)->close(c, PORT(fd)) != 0) {
    int en = errno;

    arc_err_cstrfmt(c, "close: cannot close %v \"%s\", (%s; errno=%d)", fd, strerror(en), en);
  }
  return(CNIL);
}

value arc_stdin(arc *c)
{
  return(arc_hash_lookup(c, c->genv, ARC_BUILTIN(c, S_STDIN_FD)));
}

value arc_stdout(arc *c)
{
  return(arc_hash_lookup(c, c->genv, ARC_BUILTIN(c, S_STDOUT_FD)));
}

value arc_stderr(arc *c)
{
  return(arc_hash_lookup(c, c->genv, ARC_BUILTIN(c, S_STDERR_FD)));
}

