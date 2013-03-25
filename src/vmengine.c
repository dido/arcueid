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

static value apply_return(arc *c, value thr)
{
  value cont;

  if (NIL_P(TCONR(thr)))
    return(CNIL);
  cont = TCONR(thr);
  arc_restorecont(c, thr, cont);
  return(cont);
}

static int apply_vr(arc *c, value thr)
{
  typefn_t *tfn;

  TFUNR(thr) = TVALR(thr);
  tfn = __arc_typefn(c, TVALR(thr));
  if (tfn == NULL) {
    arc_err_cstrfmt(c, "cannot apply value");
    return(0);
  }
  return(tfn->apply(c, thr, TVALR(thr)));
}

/* This is the main trampoline.  Its job is basically to keep running
   the thread until some condition occurs where it is no longer able to.

   The trampoline states are as follows:

   TR_RESUME
   Resume execution of the current state of the function being executed
   after it has been suspended for whatever reason.

   TR_SUSPEND
   Return to the thread dispatcher if the thread has been suspended for
   whatever reason.

   TR_FNAPP
   Apply the function that has been set up in the value register.  This
   will set up the thread state so that APP_RESUME will begin executing
   the function in question.

   TR_RC
   Restore the last continuation on the continuation register.
*/
void __arc_thr_trampoline(arc *c, value thr, enum tr_states_t state)
{
  value cont;

  for (;;) {
    switch (state) {
    case TR_RESUME:
      /* Resume execution of the current virtual machine state. */
      state = (TYPE(TFUNR(thr)) == T_CCODE) ? __arc_resume_aff(c, thr) : __arc_vmengine(c, thr);
      break;
    case TR_SUSPEND:
      /* just return to the dispatcher */
      return;
    case TR_FNAPP:
      /* If the state of the trampoline becomes TR_FNAPP, we will attempt
	 to apply the function in the value register, with arguments on
	 the stack, whatever type of function it happens to be. */
      state = apply_vr(c, thr);
      break;
    case TR_RC:
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
      state = TR_RESUME;
      break;
    }
  }
}

/* instruction decoding macros */
#ifdef HAVE_THREADED_INTERPRETER
/* threaded interpreter */
#define INST(name) lbl_##name
#define JTBASE ((void *)&&lbl_inop)
#ifdef HAVE_TRACING
#define NEXT {							\
    if (--TQUANTA(thr) <= 0)					\
      goto endquantum;						\
    if (vmtrace)						\
      trace(c, thr);						\
    goto *(JTBASE + jumptbl[*TIPP(thr)++]); }
#else
#define NEXT {							\
    if (--TQUANTA(thr) <= 0)					\
      goto endquantum;						\
    goto *(JTBASE + jumptbl[*TIPP(thr)++]); }
#endif

#else
/* switch interpreter */
#define INST(name) case name
#define NEXT break
#endif

/* The actual virtual machine engine.  Fits into the trampoline just
   like a normal function. */
int __arc_vmengine(arc *c, value thr)
{
#ifdef HAVE_THREADED_INTERPRETER
  static const int jumptbl[] = {
#include "jumptbl.h"
  };
#else
  value curr_instr;
#endif

#ifdef HAVE_THREADED_INTERPRETER
#ifdef HAVE_TRACING
  if (vmtrace)
    trace(c, thr);
#endif
  goto *(void *)(JTBASE + jumptbl[*TIPP(thr)++]);
#else
  for (;;) {
    curr_instr = *TIPP(thr)++;
    switch (FIX2INT(curr_instr)) {
#endif
    INST(inop):
      NEXT;
    INST(ipush):
      CPUSH(thr, TVALR(thr));
      NEXT;
    INST(ipop):
      TVALR(thr) = CPOP(thr);
      NEXT;
    INST(ildi):
      TVALR(thr) = *TIPP(thr)++;
      NEXT;
    INST(ildl): {
	value lidx = *TIPP(thr)++;
	TVALR(thr) = CODE_LITERAL(CLOS_CODE(TFUNR(thr)), FIX2INT(lidx));
      }
      NEXT;
    INST(ildg):
      {
	value tmp, tmpstr;
	char *cstr;

	tmp = CODE_LITERAL(CLOS_CODE(TFUNR(thr)), FIX2INT(*TIPP(thr)++));
	/* XXX - should we use the more general hash lookup?  Don't think
	   it should be possible to use anything besides symbols to index
	   the global top-level environment. */
	TVALR(thr) = arc_hash_lookup(c, c->genv, tmp);
	if (TVALR(thr) == CUNBOUND) {
	  tmpstr = arc_sym2name(c, tmp);
	  cstr = alloca(sizeof(char)*(FIX2INT(arc_strutflen(c, tmpstr)) + 1));
	  arc_str2cstr(c, tmpstr, cstr);
	  /* arc_print_string(c, arc_prettyprint(c, tmp)); printf("\n"); */
	  arc_err_cstrfmt(c, "Unbound symbol: _%s", cstr);
	  TVALR(thr) = CNIL;
	}
      }
      NEXT;
    INST(istg):
      if (TYPE(TVALR(thr)) == T_CLOS)
	__arc_clos_env2heap(c, thr, TVALR(thr));
      arc_hash_insert(c, c->genv, CODE_LITERAL(CLOS_CODE(TFUNR(thr)),
					       FIX2INT(*TIPP(thr)++)),
		      TVALR(thr));
      NEXT;
    INST(ilde):
      {
	int ienv, iindx;

	ienv = FIX2INT(*TIPP(thr)++);
	iindx = FIX2INT(*TIPP(thr)++);
	TVALR(thr) = *__arc_getenv(c, thr, ienv, iindx);
      }
      NEXT;
    INST(iste):
      {
	int ienv, iindx;

	ienv = FIX2INT(*TIPP(thr)++);
	iindx = FIX2INT(*TIPP(thr)++);
	if (ienv != 0 && TYPE(TVALR(thr)) == T_CLOS)
	  __arc_clos_env2heap(c, thr, TVALR(thr));
	*__arc_getenv(c, thr, ienv, iindx) = TVALR(thr);
      }
      NEXT;
    INST(icont):
      {
	int icofs = FIX2INT(*TIPP(thr)++);
	value *target = TIPP(thr) + icofs - 2;

	/* Compute the absolute target */
	icofs = target - &VINDEX(CODE_CODE(CLOS_CODE(TFUNR(thr))), 0);
	TCONR(thr) = __arc_mkcont(c, thr, icofs);
      }
      NEXT;
    INST(ienv):
      {
	int minenv, dsenv, optenv;

	minenv = FIX2INT(*TIPP(thr)++);
	dsenv = FIX2INT(*TIPP(thr)++);
	optenv = FIX2INT(*TIPP(thr)++);
	if (TARGC(thr) < minenv) {
	  arc_err_cstrfmt(c, "too few arguments, at least %d required, %d passed", minenv, TARGC(thr));
	} else if (TARGC(thr) > minenv + optenv) {
	  arc_err_cstrfmt(c, "too many arguments, at most %d allowed, %d passed", minenv + optenv, TARGC(thr));
	} else {
	  /* Make a new environment */
	  __arc_mkenv(c, thr, TARGC(thr), minenv + optenv - TARGC(thr) + dsenv);
	}
      }
      NEXT;
    INST(ienvr):
      {
	int minenv, dsenv, optenv, i;
	value rest;

	minenv = FIX2INT(*TIPP(thr)++);
	dsenv = FIX2INT(*TIPP(thr)++);
	optenv = FIX2INT(*TIPP(thr)++);
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
	  *__arc_getenv(c, thr, 0, minenv + optenv + dsenv) = rest;
	}
      }
      NEXT;
    INST(iapply):
      {
	/* Set up the argc based on the call.  Everything else required
	   for function application has already been set up beforehand */
	TARGC(thr) = FIX2INT(*TIPP(thr)++);
	return(TR_FNAPP);
      }
      NEXT;
    INST(iret):
      if (TYPE(TVALR(thr)) == T_CLOS) {
	/* XXX - move the environments referenced by the closure to the
	   heap if needed. */
	__arc_clos_env2heap(c, thr, TVALR(thr));
      }
      /* Return to the trampoline, and make it restore the current
	 continuation in the continuation register */
      return(TR_RC);
      NEXT;
    INST(ijmp):
      {
	int itarget = FIX2INT(*TIPP(thr)++);
	TIPP(thr) += itarget-2;
      }
      NEXT;
    INST(ijt):
      {
	int itarget = FIX2INT(*TIPP(thr)++);
	if (!NIL_P(TVALR(thr)))
	  TIPP(thr) += itarget-2;
      }
      NEXT;
    INST(ijf):
      {
	int itarget = FIX2INT(*TIPP(thr)++);
	if (NIL_P(TVALR(thr)))
	  TIPP(thr) += itarget-2;
      }
      NEXT;
    INST(ijbnd):
      {
	int itarget = FIX2INT(*TIPP(thr)++);
	if (TVALR(thr) != CUNBOUND)
	  TIPP(thr) += itarget-2;
      }
      NEXT;
    INST(itrue):
      TVALR(thr) = CTRUE;
      NEXT;
    INST(inil):
      TVALR(thr) = CNIL;
      NEXT;
    INST(ihlt):
      TSTATE(thr) = Trelease;
      goto endquantum;
      NEXT;
    INST(iadd):
      TVALR(thr) = __arc_add2(c, CPOP(thr), TVALR(thr));
      NEXT;
    INST(isub):
      TVALR(thr) = __arc_sub2(c, CPOP(thr), TVALR(thr));
      NEXT;
    INST(imul):
      TVALR(thr) = __arc_mul2(c, CPOP(thr), TVALR(thr));
      NEXT;
    INST(idiv):
      TVALR(thr) = __arc_div2(c, CPOP(thr), TVALR(thr));
      NEXT;
    INST(icons):
      TVALR(thr) = cons(c, CPOP(thr), TVALR(thr));
      NEXT;
    INST(icar):
      if (NIL_P(TVALR(thr)))
	TVALR(thr) = CNIL;
      else if (TYPE(TVALR(thr)) != T_CONS)
	arc_err_cstrfmt(c, "can't take car of value");
      else
	TVALR(thr) = car(TVALR(thr));
      NEXT;
    INST(icdr):
      if (NIL_P(TVALR(thr)))
	TVALR(thr) = CNIL;
      else if (TYPE(TVALR(thr)) != T_CONS)
	arc_err_cstrfmt(c, "can't take cdr of value");
      else
	TVALR(thr) = cdr(TVALR(thr));
      NEXT;
    INST(iscar):
      scar(CPOP(thr), TVALR(thr));
      NEXT;
    INST(iscdr):
      scdr(CPOP(thr), TVALR(thr));
      NEXT;
    INST(iis):
      TVALR(thr) = arc_is2(c, TVALR(thr), CPOP(thr));
      NEXT;
    INST(idup):
      TVALR(thr) = *(TSP(thr)+1);
      NEXT;
    INST(icls):
      TVALR(thr) = arc_mkclos(c, TVALR(thr), TENVR(thr));
      NEXT;
    INST(iconsr):
      TVALR(thr) = cons(c, TVALR(thr), CPOP(thr));
      NEXT;
    INST(imenv): {
	int n = FIX2INT(*TIPP(thr)++);

	__arc_menv(c, thr, n);
	TARGC(thr) = n;
      }
      NEXT;
#ifndef HAVE_THREADED_INTERPRETER
    default:
#else
    INST(invalid):
#endif
      arc_err_cstrfmt(c, "invalid opcode %02x", *TIPP(thr));
#ifdef HAVE_THREADED_INTERPRETER
#else
    }
    if (--TQUANTA(thr) <= 0)
      goto endquantum;
  }
#endif

 endquantum:
  return(TR_SUSPEND);
}
