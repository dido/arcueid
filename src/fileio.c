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

  File I/O support functions.
*/

#include "../config.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "arcueid.h"
#include "builtins.h"
#include "io.h"
#include "arith.h"

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

static typefn_t fileio_tfn;
static typefn_t procio_tfn;

struct fileio_t {
  FILE *fp;
  int closed;
};

#define FIODATA(fio) (IODATA(fio, struct fileio_t *))

static void fio_marker(arc *c, value v, int depth,
		       void (*markfn)(arc *, value, int))
{
}

static void fio_sweeper(arc *c, value v)
{
  /* DO NOT TRY TO CLOSE STDIN, STDOUT, OR STDERR! */
  if (FIODATA(v)->fp == stdin || FIODATA(v)->fp == stdout
      || FIODATA(v)->fp == stderr) {
    return;
  }
  if (!FIODATA(v)->closed) {
    fclose(FIODATA(v)->fp);
    FIODATA(v)->closed = 1;
  }
}

static AFFDEF(fio_closed_p)
{
  AARG(fio);
  AFBEGIN;
  ARETURN((FIODATA(AV(fio))->closed) ? CTRUE : CNIL);
  AFEND;
}
AFFEND

static void pio_sweeper(arc *c, value v)
{
  /* DO NOT TRY TO CLOSE STDIN, STDOUT, OR STDERR! */
  if (FIODATA(v)->fp == stdin || FIODATA(v)->fp == stdout
      || FIODATA(v)->fp == stderr) {
    return;
  }
  if (!FIODATA(v)->closed) {
    pclose(FIODATA(v)->fp);
    FIODATA(v)->closed = 1;
  }
}

static AFFDEF(fio_ready)
{
  AARG(fio);
  FILE *fp;
  fd_set rfds;
  int retval, check;
  struct timeval tv;
  AFBEGIN;

  if (TYPE(AV(fio)) == T_OUTPORT)
    ARETURN(CNIL);

  for (;;) {
    fp = FIODATA(AV(fio))->fp;
    /* XXX - NOT PORTABLE! */
#ifdef _IO_fpos_t
    check = (fp->_IO_read_ptr != fp->_IO_read_end);
#else
    check = (fp->_gptr < (fp)->_egptr);
#endif
    if (check)
      ARETURN(CTRUE);
    /* No buffered data available. See if the underlying file descriptor
       is readable. */
    FD_ZERO(&rfds);
    FD_SET(fileno(fp), &rfds);
    tv.tv_usec = tv.tv_sec = 0;
    retval = select(fileno(fp)+1, &rfds, NULL, NULL, &tv);
    if (retval == -1) {
      int en = errno;

      arc_err_cstrfmt(c, "error checking file descriptor (%s; errno=%d)",
		      strerror(en), en);
      ARETURN(CNIL);
    }

    if (FD_ISSET(fileno(fp), &rfds))
      ARETURN(CTRUE);

    /* We have to wait */
    AIOWAITR(fileno(fp));
  }
  ARETURN(CNIL);
  AFEND;
}
AFFEND

static AFFDEF(fio_wready)
{
  AARG(fio);
  AFBEGIN;
  (void)fio;
  ARETURN(CTRUE);   /* XXX - make this do something more reasonable */
  AFEND;
}
AFFEND

static AFFDEF(fio_getb)
{
  AARG(fio);
  int byte;
  AFBEGIN;
  byte = fgetc(FIODATA(AV(fio))->fp);
  if (byte < 0)
    ARETURN(CNIL);
  ARETURN(INT2FIX(byte));
  AFEND;
}
AFFEND

static AFFDEF(fio_putb)
{
  AARG(fio, byte);
  int rv;

  AFBEGIN;
  rv = fputc(FIX2INT(AV(byte)), FIODATA(AV(fio))->fp);
  if (rv < 0)
    ARETURN(CNIL);
  ARETURN(AV(byte));
  AFEND;
}
AFFEND

static AFFDEF(fio_seek)
{
  AARG(fio, offset, whence);
  AFBEGIN;
  if (!(FIX2INT(AV(whence)) == SEEK_SET || FIX2INT(AV(whence)) == SEEK_CUR ||
	FIX2INT(AV(whence)) == SEEK_END)) {
    arc_err_cstrfmt(c, "invalid whence");
    ARETURN(CNIL);
  }

#ifdef HAVE_FSEEKO
  {
    long long noff;

    if (__arc_val2ll(c, AV(offset), &noff)) {
      ARETURN(INT2FIX(fseeko(FIODATA(AV(fio))->fp, (off_t)noff, FIX2INT(AV(whence)))));
    } else {
      arc_err_cstrfmt(c, "cannot convert specified offset");
    }
  }
#else
  {
    long noff;

    if (__arc_val2long(c, AV(offset), &noff)) {
      ARETURN(INT2FIX(fseek(FIODATA(AV(fio))->fp, noff, FIX2INT(AV(whence)))));
    } else {
      arc_err_cstrfmt(c, "cannot convert specified offset");
    }
  }
#endif
  AFEND;
}
AFFEND

static AFFDEF(fio_tell)
{
  AARG(fio);
  AFBEGIN;
#ifdef HAVE_FSEEKO
  off_t offset;

  offset = ftello(FIODATA(AV(fio))->fp);
  ARETURN(__arc_ll2val(c, (long long)offset));
#else
  long offset;

  offset = ftell(FIODATA(AV(fio))->fp);
  ARETURN(__arc_long2val(c, offset));
#endif
  AFEND;
}
AFFEND

static AFFDEF(pio_close)
{
  AARG(fio);
  AFBEGIN;
  if (FIODATA(AV(fio))->closed == 0) {
    pclose(FIODATA(AV(fio))->fp);
    FIODATA(AV(fio))->closed = 1;
  }
  ARETURN(CNIL);
  AFEND;
}
AFFEND

static AFFDEF(fio_close)
{
  AARG(fio);
  AFBEGIN;
  if (FIODATA(AV(fio))->closed == 0) {
    fclose(FIODATA(AV(fio))->fp);
    FIODATA(AV(fio))->closed = 1;
  }
  ARETURN(CNIL);
  AFEND;
}
AFFEND

static value mkfio(arc *c, int type, FILE *fd, value name)
{
  value fio;

  fio = __arc_allocio(c, type, &fileio_tfn, sizeof(struct fileio_t));
  IO(fio)->flags = 0;
  IO(fio)->io_ops = VINDEX(VINDEX(c->builtins, BI_io), BI_io_fp);
  IO(fio)->name = name;
  FIODATA(fio)->closed = 0;
  FIODATA(fio)->fp = fd;
  return(fio);
}

static value openfio(arc *c, int type, value filename, const char *mode)
{
  int len;
  char *cfn;
  FILE *fp;

  len = arc_strutflen(c, filename);
  cfn = alloca(sizeof(char)*(len+2));
  arc_str2cstr(c, filename, cfn);
  fp = fopen(cfn, mode);
  if (fp == NULL) {
    int en = errno;
    arc_err_cstrfmt(c, "error opening file %s (%s; errno=%d)",
		    cfn, strerror(en), en);
    return(CNIL);
  }
  return(mkfio(c, type, fp, filename));
}

AFFDEF(arc_infile)
{
  AARG(filename);
  AOARG(mode);
  char *cmode;
  AFBEGIN;

  if (AV(mode) == ARC_BUILTIN(c, S_BINARY))
    cmode = "rb";
  else if (AV(mode) == ARC_BUILTIN(c, S_TEXT) || !BOUND_P(AV(mode)))
    cmode = "r";
  else {
    arc_err_cstrfmt(c, "infile: invalid mode");
    ARETURN(CNIL);
  }
  ARETURN(openfio(c, T_INPORT, AV(filename), cmode));
  AFEND;
}
AFFEND

AFFDEF(arc_outfile)
{
  AARG(filename);
  AOARG(mode);
  char *cmode;
  AFBEGIN;

  if (AV(mode) == ARC_BUILTIN(c, S_APPEND))
    cmode = "a";
  else if (!BOUND_P(AV(mode)))
    cmode = "w";
  else {
    arc_err_cstrfmt(c, "outfile: invalid mode");
    ARETURN(CNIL);
  }
  ARETURN(openfio(c, T_OUTPORT, AV(filename), cmode));
  AFEND;
}
AFFEND

/* XXX - when a file handle opened by pipe-from is up for gc it uses
   fclose instead of pclose! */
value arc_pipe_from(arc *c, value cmd)
{
  FILE *fp;
  int len;
  char *cmdstr;
  value ffp;

  TYPECHECK(cmd, T_STRING);
  len = FIX2INT(arc_strutflen(c, cmd));
  cmdstr = (char *)alloca(sizeof(char)*(len+1));
  arc_str2cstr(c, cmd, cmdstr);

  fp = popen(cmdstr, "r");
  if (fp == NULL) {
    int en = errno;
    arc_err_cstrfmt(c, "pipe-from: error executing command \"%s\", (%s; errno=%d)", cmdstr, strerror(en), en);
  }
  ffp = mkfio(c, T_INPORT, fp, cmd);
  IO(ffp)->io_ops = VINDEX(VINDEX(c->builtins, BI_io), BI_io_pfp);
  IO(ffp)->io_tfn = &procio_tfn;
  return(ffp);
}

void __arc_init_fio(arc *c)
{
  value io_ops;

  io_ops = arc_mkvector(c, IO_last+1);
  SVINDEX(io_ops, IO_closed_p, arc_mkaff(c, fio_closed_p, CNIL));
  SVINDEX(io_ops, IO_ready, arc_mkaff(c, fio_ready, CNIL));
  SVINDEX(io_ops, IO_wready, arc_mkaff(c, fio_wready, CNIL));
  SVINDEX(io_ops, IO_getb, arc_mkaff(c, fio_getb, CNIL));
  SVINDEX(io_ops, IO_putb, arc_mkaff(c, fio_putb, CNIL));
  SVINDEX(io_ops, IO_seek, arc_mkaff(c, fio_seek, CNIL));
  SVINDEX(io_ops, IO_tell, arc_mkaff(c, fio_tell, CNIL));
  SVINDEX(io_ops, IO_close, arc_mkaff(c, fio_close, CNIL));
  SVINDEX(VINDEX(c->builtins, BI_io), BI_io_fp, io_ops);

  io_ops = arc_mkvector(c, IO_last+1);
  SVINDEX(io_ops, IO_closed_p, arc_mkaff(c, fio_closed_p, CNIL));
  SVINDEX(io_ops, IO_ready, arc_mkaff(c, fio_ready, CNIL));
  SVINDEX(io_ops, IO_wready, arc_mkaff(c, fio_wready, CNIL));
  SVINDEX(io_ops, IO_getb, arc_mkaff(c, fio_getb, CNIL));
  SVINDEX(io_ops, IO_putb, arc_mkaff(c, fio_putb, CNIL));
  SVINDEX(io_ops, IO_seek, arc_mkaff(c, fio_seek, CNIL));
  SVINDEX(io_ops, IO_tell, arc_mkaff(c, fio_tell, CNIL));
  SVINDEX(io_ops, IO_close, arc_mkaff(c, pio_close, CNIL));
  SVINDEX(VINDEX(c->builtins, BI_io), BI_io_pfp, io_ops);

  arc_bindsym(c, ARC_BUILTIN(c, S_STDIN_FD),
	      mkfio(c, T_INPORT, stdin, arc_mkstringc(c, "(stdin)")));
  arc_bindsym(c, ARC_BUILTIN(c, S_STDOUT_FD),
	      mkfio(c, T_OUTPORT, stdout, arc_mkstringc(c, "(stdout)")));
  arc_bindsym(c, ARC_BUILTIN(c, S_STDERR_FD),
	      mkfio(c, T_OUTPORT, stderr, arc_mkstringc(c, "(stderr)")));
}

static typefn_t fileio_tfn = {
  fio_marker,
  fio_sweeper,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

static typefn_t procio_tfn = {
  fio_marker,
  pio_sweeper,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
