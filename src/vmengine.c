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

static value apply_return(arc *c, value thr)
{
  value cont;

  if (NIL_P(TCONR(thr)))
    return(CNIL);
  cont = car(TCONR(thr));
  TCONR(thr) = cdr(TCONR(thr));
  arc_restorecont(c, thr, cont);
  return(cont);
}

/* This function is used to perform function application.  Before entering,
   the following must have been performed:

   1. A continuation must have been created and linked to the continuation
      register so that the state of the system may be restored when the
      called function returns.
   2. The arguments to the function must have been pushed onto the stack
      in reverse order.
   3. The function to be called must be in the thread's value register.

   This function is meant to be used from the thread dispatcher.
 */
void __arc_thr_trampoline(arc *c, value thr)
{
  typefn_t *tfn;
  enum avals_t result;
  value cont;

  TFUNR(thr) = TVALR(thr);
  for (;;) {
    tfn = __arc_typefn(c, TVALR(thr));
    if (tfn == NULL) {
      arc_err_cstrfmt(c, "cannot apply value");
      return;
    }
    result = tfn->apply(c, thr, TVALR(thr));
    for (;;) {
      /* This is a sort of trampoline that handles the possible
	 re-invocation of an AFF. */
      switch (result) {
      case APP_RET:
	/* This case is used on the application of a closure, which contains
	   virtual machine code.  Running the apply function would have set
	   up the thread registers to the proper values for it to begin
	   execution when we return. */
	return;
      case APP_FNAPP:
	/* If an Arcueid Foreign Function returns APP_FNAPP, it will have
	   created and saved a continuation to allow us to restore it at the
	   point after it made the call, pushed the arguments to the function
	   on the stack, and set the value register to the function to be
	   called.  Looping back will thus take care of what we need to do
	   nicely! */
	break;
      case APP_YIELD:
	/* Yield interpreter.  By reducing the quantum to 0, we end the
	   thread's time slice. */
	TQUANTA(thr) = 0;
	return;
      case APP_RC:
      default:
	/* Restore the last continuation on the continuation register.  Just
	   return to the virtual machine if the last continuation was a normal
	   continuation. */
	cont = apply_return(c, thr);
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
	if (TYPE(CONT_FUN(cont)) != T_CCODE)
	  return;
	/* If we get an AFF's continuation, the thread state should have been
	   restored to a state suitable for resuming the function. */
	result = __arc_resume_aff(c, thr);
	continue;
      }
      break;
    }
  }
}
