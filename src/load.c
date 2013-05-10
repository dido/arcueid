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
*/

#include "arcueid.h"
#include "builtins.h"
#include "io.h"
#include "compiler.h"

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

value arc_loadpath_add(arc *c, value path)
{
  value loadpath, lp;

  TYPECHECK(path, T_STRING);
  loadpath = arc_gbind(c, ARC_BUILTIN(c, S_LOADPATH));
  if (!BOUND_P(loadpath))
    loadpath = CNIL;
  if (NIL_P(loadpath)) {
    loadpath = cons(c, path, loadpath);
    arc_bindsym(c, ARC_BUILTIN(c, S_LOADPATH), loadpath);
  } else {
    lp = loadpath;
    while (!NIL_P(cdr(lp))) {
      if (arc_strcmp(c, car(lp), path) == 0)
	return(loadpath);	/* already there */
      lp = cdr(lp);
    }
    if (arc_strcmp(c, car(lp), path) == 0)
      return(loadpath);	/* already there */
    scdr(lp, cons(c, path, CNIL));
  }
  return(loadpath);
}

/* Admittedly a dynamic-wind is extremely cumbersome to use in
   C, but it is the safest way. Environment layout is as follows:

   1 0 - loadfile
   1 1 - lpath
   1 2 - lf
   1 3 - fp
 */
static AFFDEF(beforethunk)
{
  AFBEGIN;
  /* Nothing to do here */
  AFEND;
}
AFFEND

#define LOAD_FP __arc_getenv(c, thr, 1, 3)

static AFFDEF(duringthunk)
{
  AVAR(sread, eval, sexpr);
  AFBEGIN;
  WV(sread, arc_mkaff(c, arc_sread, CNIL));
  WV(eval, arc_mkaff(c, arc_eval, CNIL));
  /* This performs the actual load. */
  for (;;) {
    AFCALL(AV(sread), LOAD_FP, CNIL);
    WV(sexpr, AFCRV);
    if (NIL_P(AV(sexpr)))
      ARETURN(CNIL);		/* finished */
    AFCALL(AV(eval), AV(sexpr));
  }
  AFEND;
}
AFFEND

/* close the file whatever happens */
static AFFDEF(afterthunk)
{
  AFBEGIN;
  AFTCALL(arc_mkaff(c, arc_close, CNIL), LOAD_FP);
  AFEND;
}
AFFEND

/* XXX - add the ability to load dynamic shared objects */
AFFDEF(arc_load)
{
  AARG(loadfile);
  AVAR(lpath, ldf, fp);
  AFBEGIN;

  /* Try to load a file specified as an absolute path directly */
  if (__arc_is_absolute_path(c, AV(loadfile))) {
    AFCALL(arc_mkaff(c, arc_infile, CNIL), AV(loadfile));
    WV(fp, AFCRV);
    AFCALL(arc_mkaff(c, arc_dynamic_wind, CNIL),
	   arc_mkaff2(c, beforethunk, CNIL, TENVR(thr)),
	   arc_mkaff2(c, duringthunk, CNIL, TENVR(thr)),
	   arc_mkaff2(c, afterthunk, CNIL, TENVR(thr)));
    ARETURN(CNIL);
  }

  /* Look in the loadpath for files which aren't specified as absolute */
  WV(lpath, arc_gbind(c, ARC_BUILTIN(c, S_LOADPATH)));
  while (!NIL_P(AV(lpath))) {
    WV(ldf, arc_pathjoin2(c, car(AV(lpath)), AV(loadfile)));
    if (NIL_P(arc_file_exists(c, AV(ldf)))) {
      WV(lpath, cdr(AV(lpath)));
      continue;
    }
    /* first open the file. */
    AFCALL(arc_mkaff(c, arc_infile, CNIL), AV(ldf));
    WV(fp, AFCRV);
    /* The actual load takes place in the duringthunk. The
       after thunk will take care of closing the file
       whatever happens. */
    AFCALL(arc_mkaff(c, arc_dynamic_wind, CNIL),
	   arc_mkaff2(c, beforethunk, CNIL, TENVR(thr)),
	   arc_mkaff2(c, duringthunk, CNIL, TENVR(thr)),
	   arc_mkaff2(c, afterthunk, CNIL, TENVR(thr)));
    ARETURN(CNIL);
  }
  {
    char *str;
    str = (char *)alloca(FIX2INT(arc_strutflen(c, AV(loadfile)))*sizeof(char));
    arc_str2cstr(c, AV(loadfile), str);
    arc_err_cstrfmt(c, "file %s not found in loadpath*", str);
    ARETURN(CNIL);
  }
  AFEND;
}
AFFEND
