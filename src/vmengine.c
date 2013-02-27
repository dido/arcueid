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

value __arc_return(arc *c, value thr)
{
  value cont;

  if (NIL_P(TCONR(thr)))
    return(CNIL);
  cont = car(TCONR(thr));
  TCONR(thr) = cdr(TCONR(thr));
  /*  if (TYPE(CONT_FUN(cont) != T_CCODE))
      arc_restorecont(c, thr, cont); */
  return(cont);
}

/* Perform a function application of the function in thr's value register. 
   Should work for any type that has an applicator. */
void __arc_apply(arc *c, value thr)
{
  typefn_t *tfn;
  enum avals_t result;
  value cont;

  for (;;) {
    tfn = __arc_typefn(c, TVALR(thr));
    if (tfn == NULL) {
      arc_err_cstrfmt(c, "cannot apply value");
      return;
    }
    result = tfn->apply(c, thr, TVALR(thr));
    switch (result) {
    case APP_FNAPP:
      /* If an Arcueid Foreign Function returns APP_FNAPP, it will have
	 created a continuation to allow us to restore it, pushed arguments
	 on the stack, and set the value register to the function to be
	 called.  Looping back will thus take care of what we need to do
	 nicely! */
      break;
    case APP_YIELD:
      /* Yield interpreter */
      TQUANTA(thr) = 0;
      TSTATE(thr) = Tyield;
      return;
    case APP_RET:
      /* Just return to the virtual machine core */
      return;
    case APP_RC:
    default:
      /* Restore the last continuation on the continuation register.  Just
	 return to the virtual machine if the last continuation was a normal
	 continuation. */
      cont = __arc_return(c, thr);
      if (NIL_P(cont)) {
	/* There was no available continuation on the continuation
	   register.  If this happens, the current thread should
	   terminate. */
	TQUANTA(thr) = 0;
	TSTATE(thr) = Trelease;
	return;
      }
      /* If the continuation is a normal one, just return.  It was already
	 restored by the call to __arc_return */
      if (TYPE(CONT_FUN(cont) != T_CCODE))
	return;
      /* If we have an AFF's continuation, we apply it to itself, returning
	 to where it left off before it made a call. */
      result = (int)__arc_apply_aff(c, thr, CONT_FUN(cont), cont);
      break;
    }
  }
}
