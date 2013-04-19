/* 
  Copyright (C) 2013 Rafael R. Sevilla

  This file is part of Arcueid

  Arcueid is free software: you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library. If not, see <http://www.gnu.org/licenses/>
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

struct cfunc_t {
  value name;
  union {
    value (*sff)();
    struct {
      int (*aff)(arc *, value);
      value env;
    } aff_t;
  } cfunc;
  int argc;
};

static AFFDEF(cfunc_pprint)
{
  AARG(sexpr, disp, fp);
  AOARG(visithash);
  AVAR(dw, wc);
  struct cfunc_t *rep;
  AFBEGIN;

  (void)visithash;
  (void)disp;
  WV(dw, arc_mkaff(c, __arc_disp_write, CNIL));
  WV(wc, arc_mkaff(c, arc_writec, CNIL));
  AFCALL(AV(dw), arc_mkstringc(c, "#<procedure"), CTRUE, AV(fp), AV(visithash));
  rep = (struct cfunc_t *)REP(AV(sexpr));
  if (!NIL_P(rep->name)) {
    AFCALL(AV(wc), arc_mkchar(c, ':'), AV(fp));
    AFCALL(AV(wc), arc_mkchar(c, ' '), AV(fp));
    rep = (struct cfunc_t *)REP(AV(sexpr));
    AFCALL(AV(dw), rep->name, CTRUE, AV(fp), AV(visithash));
  }
  AFCALL(AV(wc), arc_mkchar(c, '>'), AV(fp));
  ARETURN(CNIL);
  AFEND;
}
AFFEND

static void cfunc_marker(arc *c, value v, int depth,
			  void (*mark)(struct arc *, value, int))
{
  mark(c, ((struct cfunc_t *)REP(v))->name, depth);
}

static unsigned long cfunc_hash(arc *c, value v, arc_hs *s)
{
  return(arc_hash(c, ((struct cfunc_t *)REP(v))->name));
}

value arc_mkccode(arc *c, int argc, value (*cfunc)(), value name)
{
  value cfn;
  struct cfunc_t *rcfn;

  cfn = arc_mkobject(c, sizeof(struct cfunc_t), T_CCODE);
  rcfn = (struct cfunc_t *)REP(cfn);
  rcfn->name = name;
  rcfn->cfunc.sff = cfunc;
  rcfn->argc = argc;
  return(cfn);
}

value arc_mkaff2(arc *c, int (*xaff)(arc *, value), value name, value env)
{
  value aff = arc_mkccode(c, -2, NULL, name);
  struct cfunc_t *rcfn;

  rcfn = (struct cfunc_t *)REP(aff);
  rcfn->cfunc.aff_t.aff = xaff;
  rcfn->cfunc.aff_t.env = env;
  return(aff);
}

value arc_mkaff(arc *c, int (*xaff)(arc *, value), value name)
{
  return(arc_mkaff2(c, xaff, name, CNIL));
}

/* same as below, but with rest arguments */
static void affenvr(arc *c, value thr, int minenv, int optenv, int dsenv)
{
  int i;
  value rest;

  if (TARGC(thr) < minenv) {
    arc_err_cstrfmt(c, "too few arguments, at least %d required, %d passed", minenv, TARGC(thr));
  } else {
    rest = CNIL;

    /* Swallow as many extra arguments into the rest parameter,
       up to the minimum + optional number of arguments */
    for (i=TARGC(thr); i>(minenv + optenv); i--)
      rest = cons(c, CPOP(thr), rest);
    /* Create a new environment with the number of additional
       arguments thus obtained.  Unbound arguments include an
       extra one for storing the rest parameter, whatever it
       might have. */
    __arc_mkenv(c, thr, i, minenv + optenv - i + dsenv + 1);
    /* Store the rest parameter */
    __arc_putenv(c, thr, 0, minenv + optenv + dsenv, rest);
  }
}

/* Initialize the environment of an Arcueid foreign function.  This
   essentially creates the AFF's local environment if it is not yet
   there (i.e. the the TENVR of the thread is nil.  It will also take
   care of copying the parameters to the function from the thread's
   stack into any parameters that were defined by the AARG and AOARG
   macros. */
void __arc_affenv(arc *c, value thr, int nargs, int optargs, int localvars,
		  int restarg)
{
  /* If the aff_line is zero this is the first call, and we need to
     set up the environment */
  if (TIP(thr).aff_line != 0)
    return;

  if (restarg) {
    affenvr(c, thr, nargs, optargs, localvars);
    return;
  }

  if (TARGC(thr) < nargs) {
    arc_err_cstrfmt(c, "too few arguments, at least %d required, %d passed", nargs, TARGC(thr));
    return;
  } else if (TARGC(thr) > nargs + optargs) {
    arc_err_cstrfmt(c, "too many arguments, at most %d allowed, %d passed", nargs + optargs, TARGC(thr));
    return;
  }
  __arc_mkenv(c, thr, TARGC(thr), nargs + optargs - TARGC(thr) + localvars);
}

int __arc_affip(arc *c, value thr)
{
  return(TIP(thr).aff_line);
}

/* Call a function.  This sets up the thread so that when the AFF returns,
   the dispatcher will invoke the function that has been set up.  DO NOT
   USE THIS FUNCTION DIRECTLY.  It should only be used from the AFCALL
   macro.

   If a null continuation is passed, the function application is processed
   as a tail call.
 */
int __arc_affapply(arc *c, value thr, value cont, value func, ...)
{
  va_list ap;
  value arg;
  int argc=0;

  /* Add the continuation to the continuation register if a continuation
     was passed.  If it is null, then the current continuation is used,
     as it is a tail call. */
  if (!NIL_P(cont))
    SCONR(thr, cont);
  va_start(ap, func);
  /* Push the arguments onto the stack. Look for the CLASTARG sentinel
     value. */
  while ((arg = va_arg(ap, value)) != CLASTARG) {
    argc++;
    CPUSH(thr, arg);
  }
  va_end(ap);
  /* set the argument count */
  TARGC(thr) = argc;
  /* set the value register to the function to be called */
  SVALR(thr, func);
  /* If this is a tail call, overwrite the current environment */
  if (NIL_P(cont))
    __arc_menv(c, thr, argc);
  return(TR_FNAPP);
}

int __arc_affapply2(arc *c, value thr, value cont, value func, value argv)
{
  int argc=0;

  /* Add the continuation to the continuation register if a continuation
     was passed.  If it is null, then the current continuation is used,
     as it is a tail call. */
  if (!NIL_P(cont))
    SCONR(thr, cont);
  /* Push the arguments onto the stack. */
  while (!NIL_P(argv)) {
    argc++;
    CPUSH(thr, car(argv));
    argv = cdr(argv);
  }

  /* set the argument count */
  TARGC(thr) = argc;
  /* set the value register to the function to be called */
  SVALR(thr, func);
  /* If this is a tail call, overwrite the current environment */
  if (NIL_P(cont))
    __arc_menv(c, thr, argc);
  return(TR_FNAPP);
}


int __arc_affyield(arc *c, value thr, int line)
{
  TIP(thr).aff_line = line;
  return(TR_SUSPEND);
}

int __arc_affiowait(arc *c, value thr, int line, int fd, int rw)
{
  TWAITFD(thr) = fd;
  TWAITRW(thr) = rw;
  TSTATE(thr) = Tiowait;
  return(__arc_affyield(c, thr, line));
}

int __arc_resume_aff(arc *c, value thr)
{
  struct cfunc_t *rcfn;

  rcfn = (struct cfunc_t *)REP(TFUNR(thr));
  return(rcfn->cfunc.aff_t.aff(c, thr));
}

static int cfunc_apply(arc *c, value thr, value cfn)
{
  int argc, i;
  struct cfunc_t *rcfn;
  value *argv;

  rcfn = (struct cfunc_t *)REP(cfn);
  argc = TARGC(thr);
  if (rcfn->argc >= 0 && rcfn->argc != argc) {
    /* XXX - error handling */
    arc_err_cstrfmt(c, "wrong number of arguments (%d for %d)", argc,
		    rcfn->argc);
    return(TR_RC);
  }

  if (rcfn->argc == -2) {
    /* Set up the thread with the initial information for AFFs */
    TIP(thr).aff_line = 0;	/* start at line 0 (start of function body) */
    SENVR(thr, rcfn->cfunc.aff_t.env); /* parent env */
    SFUNR(thr, cfn);
    /* return to the trampoline and make it resume from the beginning
       of the function now that everything is ready */
    return(TR_RESUME);
  }

  /* Simple Foreign Functions */
  argv = alloca(sizeof(value)*argc);
  for (i=argc-1; i>=0; i--)
    argv[i] = CPOP(thr);
  switch (rcfn->argc) {
  case -1:
    SVALR(thr, rcfn->cfunc.sff(c, argc, argv));
    break;
  case 0:
    SVALR(thr, rcfn->cfunc.sff(c));
    break;
  case 1:
    SVALR(thr, rcfn->cfunc.sff(c, argv[0]));
    break;
  case 2:
    SVALR(thr, rcfn->cfunc.sff(c, argv[0], argv[1]));
    break;
  case 3:
    SVALR(thr, rcfn->cfunc.sff(c, argv[0], argv[1], argv[2]));
    break;
  case 4:
    SVALR(thr, rcfn->cfunc.sff(c, argv[0], argv[1], argv[2], argv[3]));
    break;
  case 5:
    SVALR(thr, rcfn->cfunc.sff(c, argv[0], argv[1], argv[2], argv[3],
			       argv[4]));
    break;
  case 6:
    SVALR(thr, rcfn->cfunc.sff(c, argv[0], argv[1], argv[2], argv[3],
			       argv[4], argv[5]));
    break;
  case 7:
    SVALR(thr, rcfn->cfunc.sff(c, argv[0], argv[1], argv[2], argv[3],
			       argv[4], argv[5], argv[6]));
    break;
  case 8:
    SVALR(thr, rcfn->cfunc.sff(c, argv[0], argv[1], argv[2], argv[3],
			      argv[4], argv[5], argv[6], argv[7]));
    break;
    /* XXX - extend this to perhaps 17 arguments just like Ruby */
  default:
    arc_err_cstrfmt(c, "too many arguments");
    break;
  }
  /* Restore continuation for non-AFF.  This will just return to
     wherever we were called from. */
  return(TR_RC);
}

typefn_t __arc_cfunc_typefn__ = {
  cfunc_marker,
  __arc_null_sweeper,
  cfunc_pprint,
  cfunc_hash,
  NULL,
  NULL,
  cfunc_apply,
  NULL
};
