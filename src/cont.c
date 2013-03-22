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

#if 0

/* Creates an empty reified continuation */
static value mkcont(arc *c)
{
  value cont = arc_mkvector(c, CONT_SIZE);

  ((struct cell *)cont)->_type = T_CONT;
  return(cont);
}

#endif

/* Make a continuation on the stack.  Returns the fixnum offset of
   the top of stack after all the continuation information has been
   saved. */
value __arc_mkcont(arc *c, value thr, int offset)
{
  value cont;

  CPUSH(thr, INT2FIX(offset));
  CPUSH(thr, TENVR(thr));
  CPUSH(thr, TFUNR(thr));
  CPUSH(thr, INT2FIX(TARGC(thr)));
  CPUSH(thr, TCONR(thr));
  cont = INT2FIX(TSBASE(thr) - TSP(thr));
  return(cont);
}

void arc_restorecont(arc *c, value thr, value cont)
{
  int offset, i;

  if (TYPE(cont) == T_FIXNUM) {
    /* A continuation on the stack is just an offset into the stack. */
    TSP(thr) = TSBASE(thr) - FIX2INT(cont);
    TCONR(thr) = CPOP(thr);
    TARGC(thr) = FIX2INT(CPOP(thr));
    TFUNR(thr) = CPOP(thr);
    TENVR(thr) = CPOP(thr);
    offset = FIX2INT(CPOP(thr));
  } else {
    /* Reified continuations */
    TFUNR(thr) = CONT_FUN(cont);
    TENVR(thr) = CONT_ENV(cont);
    TARGC(thr) = FIX2INT(CONT_ARGC(cont));
    TCONR(thr) = CONT_CONT(cont);
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
  TIPP(thr) = &VINDEX(CODE_CODE(CLOS_CODE(TFUNR(thr))), offset);
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
  TCONR(thr) = cont;
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
