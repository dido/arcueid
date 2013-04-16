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
#include "vmengine.h"
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

/*
  (def on-err (handler thunk)
    (ccc (fn (cont)
            (let old nil
               (dynamic-wind (fn () (= old *exh)
                                    (= *exh (cons cont handler)))
	                     thunk
                             (fn () (= *exh old))))))

  ;; not counting the cases where *exh is empty
  (def err (exc)
     (let (cont . handler) *exh
        (cont (handler exc))))

 */

/* For the following two functions, the environment layout is as follows:

   __arc_getenv(c, 1, 0) -> cont from ccchandler
   __arc_getenv(c, 1, 1) -> old from ccchandler
   __arc_getenv(c, 2, 0) -> handler from arc_on_err
   __arc_getenv(c, 2, 1) -> thunk from arc_on_err
 */
static AFFDEF(savetexh)
{
  AFBEGIN;
  /* (= old *exh) */
  *__arc_getenv(c, thr, 1, 1) = TEXH(thr);
  /* (= *exh (cons cont handler)) */
  TEXH(thr) = cons(c, *__arc_getenv(c, thr, 1, 0), *__arc_getenv(c, thr, 2, 0));
  AFEND;
}
AFFEND

static AFFDEF(restoretexh)
{
  AFBEGIN;
  /* (= *exh old) */
  TEXH(thr) = *__arc_getenv(c, thr, 1, 1);
  AFEND;
}
AFFEND

/* Environment layout is as follows:

   __arc_getenv(c, 0, 0) -> cont from ccchandler
   __arc_getenv(c, 0, 1) -> old from ccchandler
   __arc_getenv(c, 1, 0) -> handler from arc_on_err
   __arc_getenv(c, 1, 1) -> thunk from arc_on_err
 */
static AFFDEF(ccchandler)
{
  AARG(cont);
  AVAR(old);
  AFBEGIN;
  (void)old;
  (void)cont;
  TENVR(thr) = __arc_env2heap(c, thr, TENVR(thr));
  /* (dynamic-wind ... thunk ...) */
  AFTCALL(arc_mkaff(c, arc_dynamic_wind, CNIL),
	  arc_mkaff2(c, savetexh, CNIL, TENVR(thr)),
	  *__arc_getenv(c, thr, 1, 1), /* thunk from arc_on_err */
	  arc_mkaff2(c, restoretexh, CNIL, TENVR(thr)));
  AFEND;
}
AFFEND

AFFDEF(arc_on_err)
{
  AARG(handler, thunk);
  value ret;
  AFBEGIN;
  /* Silence error messages about them being unused.  They are used
     indirectly. */
  (void)handler;
  (void)thunk;
  TENVR(thr) = __arc_env2heap(c, thr, TENVR(thr));
  AFCALL(arc_mkaff(c, arc_callcc, CNIL), arc_mkaff2(c, ccchandler, CNIL,
						    TENVR(thr)));
  ret = AFCRV;
  if (TYPE(ret) != T_EXCEPTION)
    ARETURN(ret);
  AFTCALL(AV(handler), ret);
  AFEND;
}
AFFEND

/* Exceptions may have other stuff in them too someday */
value arc_mkexception(arc *c, value str)
{
  value exc;
  exc = arc_mkobject(c, 1*sizeof(value), T_EXCEPTION);
  REP(exc)[0] = str;
  return(exc);
}

void exception_marker(arc *c, value v, int depth,
		      void (*markfn)(arc *, value, int))
{
  markfn(c, REP(v)[0], depth);
}

value arc_details(arc *c, value ex)
{
  return(REP(ex)[0]);
}

AFFDEF(arc_err)
{
  AARG(str);
  AVAR(exc, cont, handler);
  AFBEGIN;
  AV(exc) = arc_mkexception(c, AV(str));
  if (NIL_P(TEXH(thr))) {
    /* if no exception handler is available, call the default error
       handler if one is set, and longjmp away */
    if (c->errhandler != NULL)
      c->errhandler(c, arc_details(c, AV(exc)));
    longjmp(TEJMP(thr), 2);
  }

  /* Otherwise, apply the exception to the continuation, so we 
     that an error was raised an the error handler needs to be
     called from arc_on_err. */
  AV(cont) = car(TEXH(thr));
  AV(handler) = cdr(TEXH(thr));
  AFTCALL(AV(cont), AV(exc));
  AFEND;
}
AFFEND

void arc_err_cstrfmt(arc *c, const char *fmt, ...)
{
  va_list ap;
  char cstr[1000];
  value str;

  va_start(ap, fmt);
  vsnprintf(cstr, sizeof(char)*1000, fmt, ap);
  va_end(ap);
  str = arc_mkstringc(c, cstr);
  /* This is how we can invoke arc_err from a non-AFF */
  __arc_mkenv(c, thr, 0, 0);	/* null env required */
  __arc_affapply(c, c->curthread, CNIL, arc_mkaff(c, arc_err, CNIL), str,
		 CLASTARG);
  longjmp(TEJMP(c->curthread), 1);
}

typefn_t __arc_exception_typefn__ = {
  exception_marker,
  __arc_null_sweeper,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
};
