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

/* Continuations in Arcueid, just as with environments, come in two flavours:

   1. Stack-based continuations
   2. Heap-based continuations

   Stack-based continuations are just fixnum offsets into the stack
   pointing to the places where the relevant information saved by the
   continuation was put.

   Heap-based continuations are vectors that contain essentially the
   same information.  Since they are visible distinctly they have their
   own data type.
*/
#include "arcueid.h"
#include "vmengine.h"

/* Creates an empty continuation on the heap */
static value mkcont(arc *c)
{
  value cont = arc_mkvector(c, CONT_SIZE);

  ((struct cell *)cont)->_type = T_CONT;
  return(cont);
}

/* Make a continuation on the stack.  Returns the fixnum offset of
   the top of stack after all the continuation information has been
   saved. */
value __arc_mkcont(arc *c, value thr, int offset)
{
  value cont, tsfn;

  tsfn = INT2FIX(TSFN(thr) - TSBASE(thr));
  CPUSH(thr, tsfn);
  CPUSH(thr, INT2FIX(offset));
  CPUSH(thr, TENVR(thr));
  CPUSH(thr, TFUNR(thr));
  CPUSH(thr, INT2FIX(TARGC(thr)));
  CPUSH(thr, TCONR(thr));
  cont = INT2FIX(TSP(thr) - TSBASE(thr));
  return(cont);
}

void arc_restorecont(arc *c, value thr, value cont)
{
  int offset, i;

  if (TYPE(cont) == T_FIXNUM) {
    /* A continuation on the stack is just an offset into the stack. */
    TSP(thr) = TSBASE(thr) + FIX2INT(cont);
    SCONR(thr, CPOP(thr));
    TARGC(thr) = FIX2INT(CPOP(thr));
    SFUNR(thr, CPOP(thr));
    SENVR(thr, CPOP(thr));
    offset = FIX2INT(CPOP(thr));
    TSFN(thr) = TSBASE(thr) + FIX2INT(CPOP(thr));
  } else {
    /* Heap-based continuations */
    SFUNR(thr, CONT_FUN(cont));
    SENVR(thr, CONT_ENV(cont));
    TARGC(thr) = FIX2INT(CONT_ARGC(cont));
    SCONR(thr, CONT_CONT(cont));
    TSFN(thr) = TSP(thr);
    /* restore saved stack */
    if (TYPE(CONT_STK(cont)) == T_VECTOR) {
      for (i=0; i<VECLEN(CONT_STK(cont)); i++)
	CPUSH(thr, VINDEX(CONT_STK(cont), i));
    }
    offset = FIX2INT(CONT_OFS(cont));
  }
  if (TYPE(TFUNR(thr)) == T_CCODE) {
    TIP(thr).aff_line = offset;
    return;
  }
  TIPP(thr) = &XVINDEX(CODE_CODE(CLOS_CODE(TFUNR(thr))), offset);
}

static value nextcont(arc *c, value thr, value cont)
{
  value *sp;

  if (FIXNUM_P(cont)) {
    sp = TSBASE(thr) + FIX2INT(cont);
    return (*(sp + 1));
  }
  return(CONT_CONT(cont));
}

static value *contenv(arc *c, value thr, value cont)
{
  value *sp;

  if (FIXNUM_P(cont)) {
    sp = TSBASE(thr) + FIX2INT(cont);
    return(sp + 4);
  }
  return(&CONT_ENV(cont));
}

/* Update environments in the continuations referred to in the continuation
   register so oldenv always points to nenv */
void __arc_update_cont_envs(arc *c, value thr, value oldenv, value nenv)
{
  value cont, *ce;

  for (cont=TCONR(thr); !NIL_P(cont); cont = nextcont(c, thr, cont)) {
    ce = contenv(c, thr, cont);
    if (*ce == oldenv) {
      __arc_wb(*ce, nenv);
      *ce = nenv;
    }
  }
}

/* Move a single continuation to the heap.  This will move any environments
   referenced by the continuation to the heap as well. */
static value heap_cont(arc *c, value thr, value cont)
{
  value *sp, *tsfn, ncont, env;
  int sslen, i;

  if (TYPE(cont) == T_CONT)
    return(cont);

  ncont = mkcont(c);
  sp = TSBASE(thr) + FIX2INT(cont);
  __arc_wb(CONT_CONT(ncont), *(sp+1));
  CONT_CONT(ncont) = *(sp+1);
  __arc_wb(CONT_ARGC(ncont), *(sp+2));
  CONT_ARGC(ncont) = *(sp+2);
  __arc_wb(CONT_FUN(ncont), *(sp+3));
  CONT_FUN(ncont) = *(sp+3);
  env = __arc_env2heap(c, thr, *(sp+4));
  __arc_wb(CONT_ENV(ncont), env);
  CONT_ENV(ncont) = env;
  if (TENVR(thr) == *(sp+4))
    SENVR(thr, env);
  __arc_wb(CONT_OFS(ncont), *(sp+5));
  CONT_OFS(ncont) = *(sp+5);
  /* save the stack up to the saved TSFN */
  tsfn = TSBASE(thr) + FIX2INT(*(sp+6));
  sslen = tsfn - (sp + 6);
  CONT_STK(ncont) = arc_mkvector(c, sslen);
  for (i=0; i<sslen; i++)
    SVINDEX(CONT_STK(ncont), i, *(tsfn - i));
  return(ncont);
}

/* Move a continuation and all its parent continuations into the heap. */
value __arc_cont2heap(arc *c, value thr, value cont)
{
  value oldcont, initcont;

  oldcont = initcont = CNIL;
  do {
    cont = heap_cont(c, thr, cont);
    if (NIL_P(initcont))
      initcont = cont;
    if (!NIL_P(oldcont))
      CONT_CONT(oldcont) = cont;
    oldcont = cont;
    cont = nextcont(c, thr, cont);
  } while (!NIL_P(cont));
  return(initcont);
}

#if 0
static value cont_pprint(arc *c, value sexpr, value *ppstr, value visithash)
{
  __arc_append_cstring(c, "#<continuation>", ppstr);
  return(*ppstr);
}
#endif

static int cont_apply(arc *c, value thr, value cont)
{
  /* Applying a continuation just means it goes on the continuation
     register and we make it go. */
  if (TARGC(thr) == 1)
    SVALR(thr, CPOP(thr));
  else {
    arc_err_cstrfmt(c, "context expected 1 value, received %d values", TARGC(thr));
    SVALR(thr, CNIL);
  }
  SCONR(thr, cont);
  return(TR_RC);
}

typefn_t __arc_cont_typefn__ = {
  __arc_vector_marker,
  __arc_null_sweeper,
  NULL,
  NULL,
  NULL,
  __arc_vector_isocmp,
  cont_apply,
  NULL
};
