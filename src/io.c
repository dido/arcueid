/* 
  Copyright (C) 2012 Rafael R. Sevilla

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
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "arcueid.h"
#include "utf.h"
#include "io.h"
#include "vmengine.h"
#include "symbols.h"
#include "builtin.h"
#include "../config.h"

#define CHECK_CLOSED(fd)				\
  if (PORT(fd)->closed) {				\
    arc_err_cstrfmt(c, "port is closed");		\
    return(CNIL);					\
  }

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

  
  nstr = (PORT(v)->name == CNIL) ? arc_strcat(c, nstr, arc_mkstringc(c, "???")) : arc_strcat(c, nstr, PORT(v)->name);
  nstr = arc_strcat(c, nstr, arc_mkstringc(c, ">"));
  return(nstr);
}

static void file_marker(arc *c, value v, int level,
			void (*markfn)(arc *, value, int, value))
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

static int file_ready(arc *c, struct arc_port *p)
{
  FILE *fp;

  fp = p->u.file.fp;
  /* XXX - NOT PORTABLE! */
#ifdef _IO_fpos_t
  return(fp->_IO_read_ptr != fp->_IO_read_end);
#else
  return(fp->_gptr < (fp)->_egptr);
#endif
}

static int file_fd(arc *c, struct arc_port *p)
{
  return(fileno(p->u.file.fp));
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
  PORT(fd)->name = filename;
  PORTF(fd).fp = fp;
  PORTF(fd).open = 1;
  PORT(fd)->getb = file_getb;
  PORT(fd)->putb = file_putb;
  PORT(fd)->seek = file_seek;
  PORT(fd)->tell = file_tell;
  PORT(fd)->close = file_close;
  PORT(fd)->ready = file_ready;
  PORT(fd)->fd = file_fd;
  PORT(fd)->closed = 0;
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
    return(CNIL);
  }
  return(arc_filefp(c, fp, filename));
}

/* XXX - If somebody wants to someday make Arcueid support Windows they
   should do something about the text/binary distinction that files on
   that platform have. */
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

SDEF2(outfile, CNIL);

static value fstr_pp(arc *c, value v)
{
  return(arc_mkstringc(c, "#<port:string>"));
}

static void fstr_marker(arc *c, value v, int level,
			void (*mark)(arc *, value, int, value))
{
  mark(c, PORTS(v).str, level+1, CNIL);
}

static void fstr_sweeper(arc *c, value v)
{
  /* no need to do anything */
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

static int fstr_ready(arc *c, struct arc_port *p)
{
  return(1);			/* fstr's are always ready */
}

static int fstr_fd(arc *c, struct arc_port *p)
{
  return(-1);			/* no such thing */
}

value arc_instring(arc *c, value str, value name)
{
  void *cellptr;
  value fd;

  TYPECHECK(str, T_STRING, 1);
  if (!NIL_P(name)) {
    TYPECHECK(name, T_STRING, 2);
  }
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
  PORT(fd)->name = name;
  PORT(fd)->getb = fstr_getb;
  PORT(fd)->putb = fstr_putb;
  PORT(fd)->seek = fstr_seek;
  PORT(fd)->tell = fstr_tell;
  PORT(fd)->close = fstr_close;
  PORT(fd)->ready = fstr_ready;
  PORT(fd)->fd = fstr_fd;
  PORT(fd)->closed = 0;
  PORT(fd)->ungetrune = -1;	/* no rune available */
  return(fd);
}

SDEF2(instring, CNIL);

value arc_outstring(arc *c, value name)
{
  if (!NIL_P(name)) {
    TYPECHECK(name, T_STRING, 1);
  }
  return(arc_instring(c, arc_mkstringc(c, ""), name));
}

SDEF(outstring, CNIL);

value arc_fstr_inside(arc *c, value fstr)
{
  TYPECHECK(fstr, T_PORT, 1);
  if (PORT(fstr)->type != FT_STRING) {
    arc_err_cstrfmt(c, "port is not a string port");
    return(CNIL);
  }
  return(PORTS(fstr).str);
}

/* Read a byte.  Returns -1 on failure. */
value arc_readb(arc *c, value fd)
{
  int ch;

  TYPECHECK(fd, T_PORT, 1);
  /* Note that if there is an unget value available, it will return
     the whole *CHARACTER*, not a possible byte within the character!
     As before, one isn't really supposed to mix the 'c' functions
     with the 'b' functions.  Do so at your own risk! */
  if (PORT(fd)->ungetrune >= 0) {
    ch = PORT(fd)->ungetrune;
    PORT(fd)->ungetrune = -1;
    return(INT2FIX(ch));
  }

  READ_CHECK(c, fd);
  CHECK_CLOSED(fd);
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

  TYPECHECK(fd, T_PORT, 2);
  CHECK_CLOSED(fd);
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
    READ_CHECK(c, fd);
    CHECK_CLOSED(fd);
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

  TYPECHECK(fd, T_PORT, 1);
  CHECK_CLOSED(fd);
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

  TYPECHECK(fd, T_PORT, 1);
  CHECK_CLOSED(fd);
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
  TYPECHECK(r, T_CHAR, 1);
  TYPECHECK(fd, T_PORT, 2);
  CHECK_CLOSED(fd);
  arc_writec_rune(c, REP(r)._char, fd);
  return(r);
}

SDEF2(writec, arc_stdout(c));

Rune arc_ungetc_rune(arc *c, Rune r, value fd)
{
  CHECK_CLOSED(fd);
  PORT(fd)->ungetrune = r;
  return(r);
}

/* Note that ungetc is a rather simplistic function. */
value arc_ungetc(arc *c, value r, value fd)
{
  TYPECHECK(fd, T_CHAR, 1);
  TYPECHECK(fd, T_PORT, 2);
  CHECK_CLOSED(fd);
  arc_ungetc_rune(c, REP(r)._char, fd);
  return(r);
}

SDEF2(ungetc, arc_stdin(c));

Rune arc_peekc_rune(arc *c, value fd)
{
  Rune r;

  CHECK_CLOSED(fd);
  r = arc_readc_rune(c, fd);
  arc_ungetc_rune(c, r, fd);
  return(r);
}

SDEF(peekc, arc_stdin(c));

value arc_peekc(arc *c, value fd)
{
  Rune r;

  TYPECHECK(fd, T_PORT, 1);
  CHECK_CLOSED(fd);
  r = arc_peekc_rune(c, fd);
  return(arc_mkchar(c, r));
}

/* XXX - probably need to define some constants for whence
   XXX - probably need to check FIX2INT values as well.  Some 32/64-bit
   compatibility issues may arise (e.g. no large files on 32-bit
   systems) */
value arc_seek(arc *c, value fd, value ofs, value whence)
{
  TYPECHECK(fd, T_PORT, 1);
  TYPECHECK(ofs, T_FIXNUM, 2);
  TYPECHECK(whence, T_FIXNUM, 3);
  CHECK_CLOSED(fd);
  if (PORT(fd)->seek(c, PORT(fd), FIX2INT(ofs), FIX2INT(whence)) < 0)
    return(CNIL);
  return(CTRUE);
}

value arc_tell(arc *c, value fd)
{
  int64_t pos;

  TYPECHECK(fd, T_PORT, 1);
  CHECK_CLOSED(fd);
  pos = PORT(fd)->tell(c, PORT(fd));
  if (pos < 0)
    return(CNIL);
  return(INT2FIX(pos));
}

value arc_close(arc *c, value fd)
{
  TYPECHECK(fd, T_PORT, 1);
  if (PORT(fd)->close(c, PORT(fd)) != 0) {
    int en = errno;

    arc_err_cstrfmt(c, "close: cannot close %v \"%s\", (%s; errno=%d)", fd, strerror(en), en);
  }
  PORT(fd)->closed = 1;
  return(CNIL);
}

/* to obtain the current stdin, we have to go through the continuation reg.
   and find some continuation register that may have a STDIN defined. */
value arc_stdin(arc *c)
{
  value thr = c->curthread, cont, fps, h=CNIL;

  if (thr != CNIL) {
    cont = TCONR(thr);
    while (cont != CNIL) {
      fps = CONT_FPS(car(cont));
      if (!(NIL_P(fps) || NIL_P(VINDEX(fps, 0))))
	return(VINDEX(fps, 0));
      cont = cdr(cont);
    }
    /* fallback to the thread's default stdin if none */
    h = VINDEX(TSTDH(thr), 0);
  }

  if (h == CNIL) {
    return(arc_hash_lookup(c, c->genv,
			   ARC_BUILTIN(c, S_STDIN_FD)));
  }
  return(h);
}

value arc_stdout(arc *c)
{
  value thr = c->curthread, cont, fps, h=CNIL;

  if (thr != CNIL) {
    cont = TCONR(thr);
    while (cont != CNIL) {
      fps = CONT_FPS(car(cont));
      if (!(NIL_P(fps) || NIL_P(VINDEX(fps, 1))))
	return(VINDEX(fps, 1));
      cont = cdr(cont);
    }
    /* fallback to the thread default stdout if none */
    h = VINDEX(TSTDH(thr), 1);
  }
  if (h == CNIL) {
    return(arc_hash_lookup(c, c->genv,
			   ARC_BUILTIN(c, S_STDOUT_FD)));
  }
  return(h);
}

value arc_stderr(arc *c)
{
  value thr = c->curthread, cont, fps, h=CNIL;

  if (thr != CNIL) {
    cont = TCONR(thr);
    while (cont != CNIL) {
      fps = CONT_FPS(car(cont));
      if (!(NIL_P(fps) || NIL_P(VINDEX(fps, 2))))
	return(VINDEX(fps, 2));
      cont = cdr(cont);
    }
    /* fallback to the thread default stderr if none */
    h = VINDEX(TSTDH(thr), 2);
  }
  if (h == CNIL) {
    return(arc_hash_lookup(c, c->genv,
			   ARC_BUILTIN(c, S_STDERR_FD)));
  }
  return(h);
}

/* Unlike the reference Arc implementation, this actually creates
   a piped process using popen.  The reference implementation cheats
   because MzScheme/Racket doesn't wrap popen(3). */
value arc_pipe_from(arc *c, value cmd)
{
  FILE *fp;
  int len;
  char *cmdstr;

  TYPECHECK(cmd, T_STRING, 1);
  len = FIX2INT(arc_strutflen(c, cmd));
  cmdstr = (char *)alloca(sizeof(char)*len+1);
  arc_str2cstr(c, cmd, cmdstr);

  fp = popen(cmdstr, "r");
  if (fp == NULL) {
    int en = errno;
    arc_err_cstrfmt(c, "pipe-from: error executing command \"%s\", (%s; errno=%d)", cmdstr, strerror(en), en);
  }
  return(arc_filefp(c, fp, cmd));
}

value arc_call_w_stdin(arc *c, value argv, value rv, CC4CTX)
{
  CC4VDEFBEGIN;
  CC4VARDEF(thr);
  CC4VARDEF(cont);
  CC4VARDEF(port);
  CC4VARDEF(thunk);
  CC4VDEFEND;
  CC4BEGIN(c);
  if (VECLEN(argv) != 2) {
    arc_err_cstrfmt(c, "call-w/stdin wrong number of arguments (%d for 2)",
		    VECLEN(argv));
    return(CNIL);
  }
  CC4V(thr) = c->curthread;
  CC4V(cont) = car(TCONR(CC4V(thr)));
  CC4V(port) = VINDEX(argv, 0);
  CC4V(thunk) = VINDEX(argv, 1);
  TYPECHECK(CC4V(port), T_PORT, 1);
  TYPECHECK(CC4V(thunk), T_CLOS, 2);
  /* modify the continuation created to call this to have its stdin
     set to whatever was specified. */
  CONT_FPS(CC4V(cont)) = arc_mkvector(c, 3);
  VINDEX(CONT_FPS(CC4V(cont)), 0) = CC4V(port);
  VINDEX(CONT_FPS(CC4V(cont)), 1) = CNIL;
  VINDEX(CONT_FPS(CC4V(cont)), 2) = CNIL;
  /* call the thunk -- any calls to arc_stdin will use the port specified */
  CC4CALL(c, argv, CC4V(thunk), 0, CNIL);
  CC4END;
  return(rv);
}

value arc_call_w_stdout(arc *c, value argv, value rv, CC4CTX)
{
  CC4VDEFBEGIN;
  CC4VARDEF(thr);
  CC4VARDEF(cont);
  CC4VARDEF(port);
  CC4VARDEF(thunk);
  CC4VDEFEND;
  CC4BEGIN(c);
  if (VECLEN(argv) != 2) {
    arc_err_cstrfmt(c, "call-w/stdout wrong number of arguments (%d for 2)",
		    VECLEN(argv));
    return(CNIL);
  }
  CC4V(thr) = c->curthread;
  CC4V(cont) = car(TCONR(CC4V(thr)));
  CC4V(port) = VINDEX(argv, 0);
  CC4V(thunk) = VINDEX(argv, 1);
  TYPECHECK(CC4V(port), T_PORT, 1);
  TYPECHECK(CC4V(thunk), T_CLOS, 2);
  /* modify the continuation created to call this to have its stdout
     set to whatever was specified. */
  CONT_FPS(CC4V(cont)) = arc_mkvector(c, 3);
  VINDEX(CONT_FPS(CC4V(cont)), 0) = CNIL;
  VINDEX(CONT_FPS(CC4V(cont)), 1) = CC4V(port);
  VINDEX(CONT_FPS(CC4V(cont)), 2) = CNIL;
  /* call the thunk -- any calls to arc_stdin will use the port specified */
  CC4CALL(c, argv, CC4V(thunk), 0, CNIL);
  CC4END;
  return(rv);
}

value arc_dir(arc *c, value dirname)
{
  char *utf_filename;
  DIR *dirp;
  int en;
  value dirlist;
  struct dirent *entry, *result;
  int delen;

  TYPECHECK(dirname, T_STRING, 1);
  utf_filename = alloca(FIX2INT(arc_strutflen(c, dirname)) + 1);
  arc_str2cstr(c, dirname, utf_filename);
  dirp = opendir(utf_filename);
  if (dirp == NULL) {
    en = errno;
    arc_err_cstrfmt(c, "dir: cannot open directory \"%s\", (%s; errno=%d)", utf_filename, strerror(en), en);
    return(CNIL);
  }
  dirlist = CNIL;
  delen = offsetof(struct dirent, d_name)
    + pathconf(utf_filename, _PC_NAME_MAX) + 1;
  entry = (struct dirent *)alloca(delen);
  for (;;) {
    if (readdir_r(dirp, entry, &result) != 0) {
      /* error */
      en = errno;
      arc_err_cstrfmt(c, "dir: error reading directory \"%s\", (%s; errno=%d)", utf_filename, strerror(en), en);
      return(CNIL);
    }
    /* end of list */
    if (result == NULL)
      break;
    /* ignore the . and .. directories */
    if (strcmp(entry->d_name == ".") == 0 || strcmp(entry->d_name == "..") == 0)
      continue;
    dirlist = cons(c, arc_mkstringc(c, entry->d_name), dirlist);
  }
  return(dirlist);
}

value arc_dir_exists(arc *c, value dirname)
{
  char *utf_filename;
  struct stat st;

  TYPECHECK(dirname, T_STRING, 1);
  utf_filename = alloca(FIX2INT(arc_strutflen(c, dirname)) + 1);
  arc_str2cstr(c, dirname, utf_filename);
  if (stat(utf_filename, &st) == -1) {
    return(CNIL);
  }
  if (S_ISDIR(st.st_mode))
    return(dirname);
  return(CNIL);
}

value arc_file_exists(arc *c, value filename)
{
  char *utf_filename;
  struct stat st;

  TYPECHECK(filename, T_STRING, 1);
  utf_filename = alloca(FIX2INT(arc_strutflen(c, filename)) + 1);
  arc_str2cstr(c, filename, utf_filename);
  if (stat(utf_filename, &st) == -1) {
    return(CNIL);
  }
  if (!S_ISDIR(st.st_mode))
    return(filename);
  return(CNIL);
}

value arc_rmfile(arc *c, value filename)
{
  char *utf_filename;
  int en;

  TYPECHECK(filename, T_STRING, 1);
  utf_filename = alloca(FIX2INT(arc_strutflen(c, filename)) + 1);
  arc_str2cstr(c, filename, utf_filename);
  if (unlink(utf_filename) < 0) {
    en = errno;
    arc_err_cstrfmt(c, "rmfile: cannot delete file \"%s\", (%s; errno=%d)", utf_filename, strerror(en), en);
    return(CNIL);
  }
  return(CNIL);
}
