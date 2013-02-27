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

/* Mark the car and cdr */
static void cons_marker(arc *c, value v, int depth,
			void (*markfn)(arc *, value, int))
{
  markfn(c, car(v), depth);
  markfn(c, cdr(v), depth);
}

static unsigned long cons_hash(arc *c, value sexpr, arc_hs *s, value visithash)
{
  unsigned long len;

  if (visithash == CNIL)
    visithash = arc_mkhash(c, ARC_HASHBITS);
  if (__arc_visit(c, sexpr, visithash) != CNIL) {
    /* Already visited at some point.  Do not recurse further.  An already
       visited node will still contribute 1 to the length though. */
    return(1);
  }
  len = arc_hash(c, car(sexpr), visithash);
  len += arc_hash(c, cdr(sexpr), visithash);
  return(len);
}

static value cons_isocmp(arc *c, value v1, value v2, value vh1, value vh2)
{
  value vhh1, vhh2;

  if (vh1 == CNIL) {
    vh1 = arc_mkhash(c, ARC_HASHBITS);
    vh2 = arc_mkhash(c, ARC_HASHBITS);
  }

  if ((vhh1 = __arc_visit(c, v1, vh1)) != CNIL) {
    /* If we find a visited object, see if v2 is also visited in vh2.
       If not, they are not the same. */
    vhh2 = __arc_visit(c, v2, vh2);
    /* We see if the same value was produced on visiting. */
    return((vhh2 == vhh1) ? CTRUE : CNIL);
  }

  /* Get value assigned by __arc_visit to v1. */
  vhh1 = __arc_visit(c, v1, vh1);
  /* If we somehow already visited v2 when v1 was not visited in the
     same way, they cannot be the same. */
  if (__arc_visit2(c, v2, vh2, vhh1) != CNIL)
    return(CNIL);
  /* Recursive comparisons */
  if (arc_iso(c, car(v1), car(v2), vh1, vh2) != CTRUE)
    return(CNIL);
  if (arc_iso(c, cdr(v1), cdr(v2), vh1, vh2) != CTRUE)
    return(CNIL);
  return(CTRUE);
}

/* A cons can be applied to a fixnum value */
static value cons_apply(arc *c, value thr, value list)
{
  value fidx;
  int index, i;

  if (arc_thr_argc(c, thr) != 1) {
    arc_err_cstrfmt(c, "application of a cons expects 1 argument, given %d",
		    arc_thr_argc(c, thr));
    return(CNIL);
  }

  fidx = arc_thr_pop(c, thr);
  if (TYPE(fidx) != T_FIXNUM) {
    arc_err_cstrfmt(c, "application of a cons expects type <non-negative exact integer> as argument");
    return(CNIL);
  }
  index = FIX2INT(fidx);

  if (index < 0) {
    arc_err_cstrfmt(c, "application of a cons expects type <non-negative exact integer> as argument");
    return(CNIL);
  }

  for (i=0;; i++) {
    if (i==index) {
      arc_thr_set_valr(c, thr, car(list));
      return(CNIL);
    }
    list = cdr(list);
    if (NIL_P(list) || TYPE(list) != T_CONS) {
      arc_err_cstrfmt(c, "index %d too large for list", index);
      return(CNIL);
    }
  }
  /* never get here */
  return(CNIL);
}

value cons(arc *c, value x, value y)
{
  value cv;

  cv = arc_mkobject(c, 2*sizeof(value), T_CONS);
  car(cv) = x;
  cdr(cv) = y;
  return(cv);
}

typefn_t __arc_cons_typefn__ = {
  cons_marker,
  __arc_null_sweeper,
  cons_pprint,
  cons_hash,
  NULL,
  cons_isocmp,
  cons_apply
};
