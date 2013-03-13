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

#if 0

static value cons_pprint(arc *c, value sexpr, value *ppstr, value visithash)
{
  value osexpr = sexpr;

  /* XXX - we should do something a bit more sophisticated here, e.g.
     arc> (= x '(1 2))
     arc> (scdr x x)
     arc> a

     should print something like:

     #0=(1 . #0#)

     The current algorithm already produces:

     (1 . (...))

     which is already a fair sight better than what arc3 does when faced
     with the same problem!

     Doing the former may require us to traverse the conses twice.  We'll
     implement it someday.
  */
  /* Create a visithash if we don't already have one */
  if (visithash == CNIL)
    visithash = arc_mkhash(c, ARC_HASHBITS);

  if (__arc_visit(c, sexpr, visithash) != CNIL) {
    /* Already visited at some point.  Do not recurse further. */
    __arc_append_cstring(c, "(...)", ppstr);
    return(*ppstr);
  }

  __arc_append_cstring(c, "(", ppstr);
  while (TYPE(sexpr) == T_CONS) {
    if (__arc_visitp(c, car(sexpr), visithash) != CNIL) {
      /* Already visited at some point.  Do not recurse further. */
      __arc_append_cstring(c, "(...)", ppstr);
    } else {
      arc_prettyprint(c, car(sexpr), ppstr, visithash);
    }

    sexpr = cdr(sexpr);
    if (__arc_visitp(c, sexpr, visithash) != CNIL)
      break;
    if (!NIL_P(sexpr))
      __arc_append_cstring(c,  " ", ppstr);
  }

  if (sexpr != CNIL) {
    __arc_append_cstring(c,  " . ", ppstr);
    arc_prettyprint(c, sexpr, ppstr, visithash);
  }
  __arc_append_cstring(c, ")", ppstr);
  __arc_unvisit(c, osexpr, visithash);
  return(*ppstr);
}

#endif

/* Mark the car and cdr */
static void cons_marker(arc *c, value v, int depth,
			void (*markfn)(arc *, value, int))
{
  markfn(c, car(v), depth);
  markfn(c, cdr(v), depth);
}

static AFFDEF(cons_isocmp, v1, v2, vh1, vh2)
{
  value vhh1, vhh2;		/* not required after calls */
  AVAR(iso2);
  AFBEGIN;

  if ((vhh1 = __arc_visit(c, AV(v1), AV(vh1))) != CNIL) {
    /* If we find a visited object, see if v2 is also visited in vh2.
       If not, they are not the same. */
    vhh2 = __arc_visit(c, AV(v2), AV(vh2));
    /* We see if the same value was produced on visiting. */
    ARETURN((vhh2 == vhh1) ? CTRUE : CNIL);
  }

  /* Get value assigned by __arc_visit to v1. */
  vhh1 = __arc_visit(c, AV(v1), AV(vh1));
  /* If we somehow already visited v2 when v1 was not visited in the
     same way, they cannot be the same. */
  if (__arc_visit2(c, AV(v2), AV(vh2), vhh1) != CNIL)
    ARETURN(CNIL);
  /* Recursive comparisons */
  AV(iso2) = arc_mkaff(c, arc_iso2, CNIL);
  AFCALL(AV(iso2), car(AV(v1)), car(AV(v2)), AV(vh1), AV(vh2));
  if (NIL_P(AFCRV))
    ARETURN(CNIL);
  AFCALL(AV(iso2), cdr(AV(v1)), cdr(AV(v2)), AV(vh1), AV(vh2));
  if (NIL_P(AFCRV))
    ARETURN(CNIL);
  ARETURN(CTRUE);
  AFEND;
}
AFFEND

/* A cons can be applied to a fixnum value */
static int cons_apply(arc *c, value thr, value list)
{
  value fidx;
  int index, i;

  if (arc_thr_argc(c, thr) != 1) {
    arc_err_cstrfmt(c, "application of a cons expects 1 argument, given %d",
		    arc_thr_argc(c, thr));
    return(APP_RC);
  }

  fidx = arc_thr_pop(c, thr);
  if (TYPE(fidx) != T_FIXNUM) {
    arc_err_cstrfmt(c, "application of a cons expects type <non-negative exact integer> as argument");
    return(APP_RC);
  }
  index = FIX2INT(fidx);

  if (index < 0) {
    arc_err_cstrfmt(c, "application of a cons expects type <non-negative exact integer> as argument");
    return(APP_RC);
  }

  for (i=0;; i++) {
    if (i==index) {
      arc_thr_set_valr(c, thr, car(list));
      return(APP_RC);
    }
    list = cdr(list);
    if (NIL_P(list) || TYPE(list) != T_CONS) {
      arc_err_cstrfmt(c, "index %d too large for list", index);
      return(APP_RC);
    }
  }
  /* never get here */
  return(APP_RC);
}

value cons(arc *c, value x, value y)
{
  value cv;

  cv = arc_mkobject(c, 2*sizeof(value), T_CONS);
  car(cv) = x;
  cdr(cv) = y;
  return(cv);
}

value arc_list_append(value list1, value val)
{
  value list;

  if (val == CNIL)
    return(list1);
  if (list1 == CNIL)
    return(val);
  list = list1;
  while (cdr(list) != CNIL)
    list = cdr(list);
  scdr(list, val);
  return(list1);
}

typefn_t __arc_cons_typefn__ = {
  cons_marker,
  __arc_null_sweeper,
  NULL,
  NULL,
  NULL,
  cons_isocmp,
  cons_apply,
  NULL
};
