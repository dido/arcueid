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
#include <stdarg.h>
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

enum arc_trstate apply(arc *c, value thr, value v)
{
  arc_ffunc *ff = (arc_ffunc *)v;
  int argc, i;
  value *argv;
  arc_thread *t = (arc_thread *)thr;

  argc = t->argc;
  if (ff->argc >= 0 && ff->argc != argc) {
    arc_err_cstr(c, "wrong number of arguments (%d for %d)", argc, ff->argc);
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
    arc_err_cstr(c, "too many arguments");
    break;
  }
  /* Restore continuation for non-AFF. The trampoline this was called
     from will simply return back to wherever it was called from */
  return(TR_RC);
}

arctype __arc_ffunc_t = { NULL, NULL, NULL, NULL, NULL, NULL, apply };

value arc_ff_new(arc *c, int argc, value (*func)())
{
  value vff = arc_new(c, &__arc_ffunc_t, sizeof(arc_ffunc));
  arc_ffunc *ff = (arc_ffunc *)vff;
  ff->argc = argc;
  ff->ff.ff = func;
  return(vff);
}

value arc_aff_new(arc *c, enum arc_trstate (*func)(arc *, value))
{
  value vff = arc_new(c, &__arc_ffunc_t, sizeof(arc_ffunc));
  arc_ffunc *ff = (arc_ffunc *)vff;
  ff->argc = -2;
  ff->ff.aff = func;
  return(vff);
}

enum arc_trstate __arc_resume_aff(arc *c, value thr, value aff)
{
  return(((arc_ffunc *)aff)->ff.aff(c, thr));
}

/* Sets up environment for functions with rest arguments */
static void affenvr(arc *c, value thr, int minenv, int optenv, int dsenv)
{
  arc_thread *t = (arc_thread *)thr;
  int i;
  value rest;

  if (t->argc < minenv) {
    arc_err_cstr(c, "too few arguments, at least %d required, %d passed", minenv, t->argc);
    return;
  }
  rest = CNIL;

  /* Swallow as many extra arguments into the rest parameter,
     up to the minimum + optional number of arguments */
  for (i=t->argc; i>(minenv + optenv); i--)
    rest = cons(c, CPOP(thr), rest);
  /* Create a new environment with the number of additional
     arguments thus obtained. Unbound arguments include an
     extra one for storing the rest parameter, whatever it
     might have */
  __arc_env_new(c, thr, i, minenv + optenv - i + dsenv + 1);
  /* Store the rest parameter */
  __arc_putenv0(c, thr, minenv + optenv + dsenv, rest);
}

void __arc_affenv(arc *c, value thr, int nargs, int optargs,
			 int localvars, int restarg)
{
  arc_thread *t = (arc_thread *)thr;

  /* We only need to set up the environment if this is the first call
     into the function. If this gets called again after the state
     ("instruction pointer") is something other than zero, it does
     nothing because the environment will already have been set up by
     a prior call. */
  if (t->ip != 0)
    return;

  if (restarg) {
    affenvr(c, thr, nargs, optargs, localvars);
    return;
  }

  if (t->argc < nargs) {
    arc_err_cstr(c, "too few arguments, at least %d required, %d passed", nargs, t->argc);
    return;
  }

  if (t->argc > nargs + optargs) {
    arc_err_cstr(c, "too many arguments, at most %d allowed, %d passed", nargs + optargs, t->argc);
    return;
  }
  __arc_env_new(c, thr, t->argc, nargs + optargs - t->argc + localvars);
}

int __arc_affip(arc *c, value thr)
{
  return(((arc_thread *)thr)->ip);
}

enum arc_trstate __arc_affapply(arc *c, value thr, value cont, value func, ...)
{
  int argc;
  va_list ap;
  value arg;
  arc_thread *t = (arc_thread *)thr;

  /* Add the continuation to the continuation register if a
     continuation was passed. If it is nil, then the current
     continuation is used, as it is a tail call. */
  if (!NILP(cont)) {
    arc_wb(c, t->cont, cont);
    t->cont = cont;
  }
  va_start(ap, func);
  /* Push arguments onto the stack. Look for CLASTARG sentinel */
  va_start(ap, func);
  while ((arg = va_arg(ap, value)) != CLASTARG) {
    argc++;
    CPUSH(thr, arg);
  }
  va_end(ap);
  /* Set the argument count */
  t->argc = argc;
  /* Set accumulator to function to be called */
  arc_thr_setacc(c, thr, func);
  /* If this is a tail call, overwrite the current environment */
  if (NILP(cont))
    __arc_menv(c, thr, argc);
  return(TR_FNAPP);
}
