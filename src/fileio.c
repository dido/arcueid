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

#include <stdio.h>
#include "arcueid.h"
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

static typefn_t fileio_tfn;

struct fileio_t {
  FILE *fp;
};

#define FIODATA(fio) (IODATA(fio, struct fileio_t *))

static void fio_marker(arc *c, value v, int depth,
		       void (*markfn)(arc *, value, int))
{
}

static void fio_sweeper(arc *c, value v)
{
  fclose(FIODATA(v));
}

static void mkfio(arc *c, int type, value filename, const char *mode)
{
  value fio;
  int len;
  char *cfn;

  fio = __arc_allocio(c, type, &fileio_tfn, sizeof(struct fileio_t));
  IO(fio)->flags = 0;
  IO(fio)->io_ops = VINDEX(VINDEX(c->builtins, BI_io), BI_io_fp);
  IO(fio)->name = filename;

  len = arc_strutflen(c, filename);
  cfn = alloca(sizeof(char)*(len+2));
  arc_str2cstr(c, filename, cfn);
  FIODATA(fio)->fp = fopen(c, cfn, mode);
  return(fio);
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
  ARETURN(mkfio(c, T_INPORT, AV(filename), cmode));
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
  ARETURN(mkfio(c, T_OUTPORT, AV(filename), cmode));
  AFEND;
}
AFFEND


void __arc_init_fio(arc *c)
{
  value io_ops;

  io_ops = arc_mkvector(c, IO_last+1);
  VINDEX(io_ops, IO_closed_p) = arc_mkaff(c, fio_closed_p, CNIL);
  VINDEX(io_ops, IO_ready) = arc_mkaff(c, fio_ready, CNIL);
  VINDEX(io_ops, IO_wready) = arc_mkaff(c, fio_wready, CNIL);
  VINDEX(io_ops, IO_getb) = arc_mkaff(c, fio_getb, CNIL);
  VINDEX(io_ops, IO_putb) = arc_mkaff(c, fio_putb, CNIL);
  VINDEX(io_ops, IO_seek) = arc_mkaff(c, fio_seek, CNIL);
  VINDEX(io_ops, IO_tell) = arc_mkaff(c, fio_tell, CNIL);
  VINDEX(io_ops, IO_close) = arc_mkaff(c, fio_close, CNIL);
  VINDEX(VINDEX(c->builtins, BI_io), BI_io_fp) = io_ops;
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
}
