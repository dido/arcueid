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
#include "vmengine.h"

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

typedef struct {
  int argc;
  union {
    value (*ff)();
    enum arc_trstate (*aff)(arc *, value);
  } ff;
} ffunc;

enum arc_trstate apply(arc *c, value thr, value v)
{
  ffunc *ff = (ffunc *)v;
  int argc, i;
  value *argv;
  arc_thread *t = (arc_thread *)thr;

  argc = t->argc;
  if (ff->argc >= 0 && ff->argc != argc) {
    arc_err_cstr(c, t->line, "wrong number of arguments (%d for %d)", argc, ff->argc);
    return(TR_RC);
  }

  if (ff->argc == -2) {
    /* Set up the thread with initial information for AFFs. */
    arc_thr_setip(c, thr, 0);
    arc_thr_setfunc(c, thr, v);
    /* return to trampoline and make it resume from the beginning of the
       function now that everything is ready */
    return(TR_RESUME);
  }

  /* Simple foreign functions */
  argv = alloca(sizeof(value)*argc);
  for (i=argc-1; i>=0; i--)
    argv[i] = CPOP(thr);
  switch (ff->argc) {
  case -1:
    arc_thr_setacc(c, thr, ff->ff.ff(c, argc, argv));
    break;
  case 0:
    arc_thr_setacc(c, thr, ff->ff.ff(c));
    break;
  case 1:
    arc_thr_setacc(c, thr, ff->ff.ff(c, argv[0]));
    break;
  case 2:
    arc_thr_setacc(c, thr, ff->ff.ff(c, argv[0], argv[1]));
    break;
  case 3:
    arc_thr_setacc(c, thr, ff->ff.ff(c, argv[0], argv[1], argv[2]));
    break;
  case 4:
    arc_thr_setacc(c, thr, ff->ff.ff(c, argv[0], argv[1], argv[2], argv[3]));
    break;
  case 5:
    arc_thr_setacc(c, thr, ff->ff.ff(c, argv[0], argv[1], argv[2], argv[3],
				     argv[4]));
    break;
  case 6:
    arc_thr_setacc(c, thr, ff->ff.ff(c, argv[0], argv[1], argv[2], argv[3],
				     argv[4], argv[5]));
    break;
  case 7:
    arc_thr_setacc(c, thr, ff->ff.ff(c, argv[0], argv[1], argv[2], argv[3],
				     argv[4], argv[5], argv[6]));
    break;
  case 8:
    arc_thr_setacc(c, thr, ff->ff.ff(c, argv[0], argv[1], argv[2], argv[3],
				     argv[4], argv[5], argv[6], argv[7]));
    break;
  default:
    arc_err_cstr(c, t->line, "too many arguments");
    break;
  }
  /* Restore continuation for non-AFF. The trampoline this was called
     from will simply return back to wherever it was called from */
  return(TR_RC);
}

arctype __arc_ffunc_t = { NULL, NULL, NULL, NULL, NULL, NULL, apply };

value arc_ff_new(arc *c, int argc, value (*func)())
{
  value vff = arc_new(c, &__arc_ffunc_t, sizeof(ffunc));
  ffunc *ff = (ffunc *)vff;
  ff->argc = argc;
  ff->ff.ff = func;
  return(vff);
}

value arc_aff_new(arc *c, enum arc_trstate (*func)(arc *, value))
{
  value vff = arc_new(c, &__arc_ffunc_t, sizeof(ffunc));
  ffunc *ff = (ffunc *)vff;
  ff->argc = -2;
  ff->ff.aff = func;
  return(vff);
}

enum arc_trstate __arc_resume_aff(arc *c, value thr, value aff)
{
  return(((ffunc *)aff)->ff.aff(c, thr));
}
