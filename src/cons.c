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
#include "arith.h"
#include "builtins.h"

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

/* XXX - we should do something a bit more sophisticated here, e.g.
   arc> (= x '(1 2))
   arc> (scdr x x)
   arc> a

   should print something like:

   #0=(1 . #0#)

   The current algorithm already produces:

   (1 . (...))

   which is already a fair sight better than what official arc3.1 does when
   faced with the same input!

   Doing the former may require us to traverse the conses twice.  We'll
   implement it someday.
*/
static AFFDEF(cons_pprint)
{
  AARG(sexpr, disp, fp);
  AOARG(visithash);
  AVAR(wc, dw, osexpr);
  AFBEGIN;

  WV(osexpr, AV(sexpr));
  if (!BOUND_P(AV(visithash)))
    WV(visithash, arc_mkhash(c, ARC_HASHBITS));

  if (!NIL_P(__arc_visit(c, AV(sexpr), AV(visithash)))) {
    /* already visited at some point. Do not recurse further */
    AFTCALL(arc_mkaff(c, __arc_disp_write, CNIL), arc_mkstringc(c, "(...)"),
	   CTRUE, AV(fp), AV(visithash));
  }
  WV(wc, arc_mkaff(c, arc_writec, CNIL));
  WV(dw, arc_mkaff(c, __arc_disp_write, CNIL));
  AFCALL(AV(wc), arc_mkchar(c, '('), AV(fp));
  while (TYPE(AV(sexpr)) == T_CONS) {
    if (!NIL_P(__arc_visitp(c, car(AV(sexpr)), AV(visithash)))) {
    /* already visited at some point. Do not recurse further */
      AFCALL(AV(dw), arc_mkstringc(c, "(...)"), CTRUE,
	     AV(fp), AV(visithash));
    } else {
      AFCALL(AV(dw), car(AV(sexpr)), AV(disp), AV(fp), AV(visithash));
    }
    WV(sexpr, cdr(AV(sexpr)));
    if (!NIL_P(__arc_visitp(c, AV(sexpr), AV(visithash))))
      goto finish;		/* not sure if break works fine... */
    if (!NIL_P(AV(sexpr)))
      AFCALL(AV(wc), arc_mkchar(c, ' '), AV(fp));
  }
 finish:
  if (!NIL_P(AV(sexpr))) {	/* improper list */
    AFCALL(AV(wc), arc_mkchar(c, ' '), AV(fp));
    AFCALL(AV(wc), arc_mkchar(c, '.'), AV(fp));
    AFCALL(AV(wc), arc_mkchar(c, ' '), AV(fp));
    AFCALL(AV(dw), AV(sexpr), AV(disp), AV(fp), AV(visithash));
  }
  AFCALL(AV(wc), arc_mkchar(c, ')'), AV(fp));
  __arc_unvisit(c, AV(osexpr), AV(visithash));
  ARETURN(CNIL);
  AFEND;
}
AFFEND

/* Mark the car and cdr */
static void cons_marker(arc *c, value v, int depth,
			void (*markfn)(arc *, value, int))
{
  markfn(c, car(v), depth);
  markfn(c, cdr(v), depth);
}

static AFFDEF(cons_isocmp)
{
  AARG(v1, v2, vh1, vh2);
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
  WV(iso2, arc_mkaff(c, arc_iso2, CNIL));
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
    return(TR_RC);
  }

  fidx = arc_thr_pop(c, thr);
  if (TYPE(fidx) != T_FIXNUM) {
    arc_err_cstrfmt(c, "application of a cons expects type <non-negative exact integer> as argument");
    return(TR_RC);
  }
  index = FIX2INT(fidx);

  if (index < 0) {
    arc_err_cstrfmt(c, "application of a cons expects type <non-negative exact integer> as argument");
    return(TR_RC);
  }

  for (i=0;; i++) {
    if (i==index) {
      arc_thr_set_valr(c, thr, car(list));
      return(TR_RC);
    }
    list = cdr(list);
    if (NIL_P(list) || TYPE(list) != T_CONS) {
      arc_err_cstrfmt(c, "index %d too large for list", index);
      return(TR_RC);
    }
  }
  /* never get here */
  return(TR_RC);
}

AFFDEF(cons_xhash)
{
  AARG(obj, ehs, length, visithash);
  AFBEGIN;

  if (!BOUND_P(AV(visithash)))
    WV(visithash, arc_mkhash(c, ARC_HASHBITS));

  /* Already visited at some point.  Do not recurse further. */
  if (__arc_visit(c, AV(obj), AV(visithash)) != CNIL)
    ARETURN(AV(length));

  /* Visit car */
  AFCALL(arc_mkaff(c, arc_xhash_increment, CNIL), car(AV(obj)), AV(ehs),
	 AV(visithash));
  WV(length, __arc_add2(c, AV(length), AFCRV));
  /* Visit cdr */
  AFCALL(arc_mkaff(c, arc_xhash_increment, CNIL), cdr(AV(obj)), AV(ehs),
	 AV(visithash));
  ARETURN(__arc_add2(c, AV(length), AFCRV));
  AFEND;
}
AFFEND

value cons(arc *c, value x, value y)
{
  value cv;

  cv = arc_mkobject(c, 2*sizeof(value), T_CONS);
  scar(cv, x);
  scdr(cv, y);
  return(cv);
}

value arc_car(arc *c, value v)
{
  if (NIL_P(v))
    return(CNIL);
  TYPECHECK(v, T_CONS);
  return(car(v));
}

value arc_cdr(arc *c, value v)
{
  if (NIL_P(v))
    return(CNIL);
  TYPECHECK(v, T_CONS);
  return(cdr(v));
}

value arc_cadr(arc *c, value v)
{
  if (NIL_P(v))
    return(CNIL);
  TYPECHECK(v, T_CONS);
  if (NIL_P(cdr(v)))
    return(CNIL);
  TYPECHECK(cdr(v), T_CONS);
  return(cadr(v));
}

value arc_cddr(arc *c, value v)
{
  if (NIL_P(v))
    return(CNIL);
  TYPECHECK(v, T_CONS);
  if (NIL_P(cdr(v)))
    return(CNIL);
  TYPECHECK(cdr(v), T_CONS);
  return(cddr(v));
}

value arc_scar(arc *c, value x, value y)
{
  if (TYPE(x) == T_CONS)
    return(scar(x, y));

  if (TYPE(x) == T_STRING && TYPE(y) == T_CHAR) {
    arc_strsetindex(c, x, 0, arc_char2rune(c, y));
    return(y);
  }
  TYPECHECK(x, T_CONS);
  return(CNIL);
}

value arc_scdr(arc *c, value x, value y)
{
  TYPECHECK(x, T_CONS);
  return(scdr(x, y));
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

value arc_list_length(arc *c, value list)
{
  value n;

  n = INT2FIX(0);
  for (;;) {
    if (!CONS_P(list)) {
      if (!NIL_P(list))
	n = __arc_add2(c, n, INT2FIX(1));
      break;
    }
    n = __arc_add2(c, n, INT2FIX(1));
    list = cdr(list);
  }
  return(n);
}


static AFFDEF(cons_xcoerce)
{
  AARG(obj, stype, arg);
  AVAR(str, hash);
  AFBEGIN;
  if (FIX2INT(AV(stype)) == T_CONS)
    ARETURN(AV(obj));

  if (FIX2INT(AV(stype)) == T_STRING) {
    WV(str, arc_mkstringc(c, ""));
    /* EXT: Note that this propagates the extra arg in recursive calls to
       coerce: arc3.1 and Anarki do not.  So in Arcueid you can do the
       following:
       (coerce '(10 11 12) 'string 16) => "abc"
       Same form causes an error in other Arc implementations.
     */
    for (; CONS_P(AV(obj)); WV(obj, cdr(AV(obj)))) {
      AFCALL(arc_mkaff(c, arc_coerce, CNIL), car(AV(obj)),
	     ARC_BUILTIN(c, S_STRING), AV(arg));
      WV(str, arc_strcat(c, AV(str), AFCRV));
    }
    if (!NIL_P(AV(obj))) {
      /* XXX - this is the behaviour of reference Arc.  Perhaps the final
	 non-atom should be converted just the same? */
      arc_err_cstrfmt(c, "cannot coerce improper list");
      ARETURN(CNIL);
    }
    ARETURN(AV(str));
  }

  if (FIX2INT(AV(stype)) == T_VECTOR) {
    value len, vec;
    int i;

    len = arc_list_length(c, AV(obj));
    vec = arc_mkvector(c, FIX2INT(len));
    for (i=0; CONS_P(AV(obj)); WV(obj, cdr(AV(obj))), i++)
      SVINDEX(vec, i, car(AV(obj)));
    if (!NIL_P(AV(obj)))
      SVINDEX(vec, i, AV(obj));
    ARETURN(vec);
  }

  /* Assoc tables of the form ( ... (key . value) are the only type
     that can be converted into hashes. */
  if (FIX2INT(AV(stype)) == T_TABLE) {
    WV(hash, arc_mkhash(c, ARC_HASHBITS));

    while (!NIL_P(AV(obj))) {
      value cell = car(AV(obj)), key, val;
      if (TYPE(cell) != T_CONS) {
	arc_err_cstrfmt(c, "cannot coerce to hash, must be assoc form");
	ARETURN(CNIL);
      }
      key = car(cell);
      val = cdr(cell);
      AFCALL(arc_mkaff(c, arc_xhash_insert, CNIL), AV(hash), key, val);
      WV(obj, cdr(AV(obj)));
    }
    ARETURN(AV(hash));
  }

  arc_err_cstrfmt(c, "cannot coerce");
  ARETURN(CNIL);
  AFEND;
}
AFFEND

typefn_t __arc_cons_typefn__ = {
  cons_marker,
  __arc_null_sweeper,
  cons_pprint,
  NULL,
  NULL,
  cons_isocmp,
  cons_apply,
  cons_xcoerce,
  cons_xhash
};
