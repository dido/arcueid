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

static value vector_pprint(arc *c, value sexpr, value *ppstr, value visithash)
{
  /* XXX fill this in */
  return(CNIL);
}

void __arc_vector_marker(arc *c, value v, int depth,
			 void (*markfn)(arc *, value, int))
{
  int i;

  for (i=0; i<VECLEN(v); i++)
    markfn(c, VINDEX(v, i), depth);
}

AFFDEF(__arc_vector_isocmp, v1, v2, vh1, vh2)
{
  value vhh1, vhh2;		/* not required after calls */
  AVAR(iso2, i);
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
  if (__arc_visit2(c, AV(v2), AV(vh2), AV(vhh1)) != CNIL)
    ARETURN(CNIL);
  /* Vectors must be identical in length to be the same */
  if (VECLEN(AV(v1)) != VECLEN(AV(v2)))
    ARETURN(CNIL);
  /* Recursive comparisons */
  AV(iso2) = arc_mkaff(c, arc_iso2, CNIL);
  for (AV(i) = INT2FIX(0); FIX2INT(AV(i))<VECLEN(AV(v1));
       AV(i) = INT2FIX(FIX2INT(AV(i)) + 1)) {
    AFCALL(AV(iso2), VINDEX(AV(v1), AV(i)), VINDEX(AV(v2), AV(i)),
	   AV(vh1), AV(vh2));
    if (NIL_P(AFCRV))
      ARETURN(CNIL);
  }
  ARETURN(CTRUE);
  AFEND;
}
AFFEND

/* A vector can be applied to a fixnum value */
static int vector_apply(arc *c, value thr, value vec)
{
  value fidx;
  int index;

  if (arc_thr_argc(c, thr) != 1) {
    arc_err_cstrfmt(c, "application of a vector expects 1 argument, given %d",
		    arc_thr_argc(c, thr));
    return(APP_RC);
  }
  fidx = arc_thr_pop(c, thr);
  if (TYPE(fidx) != T_FIXNUM) {
    arc_err_cstrfmt(c, "application of a vector expects type <non-negative exact integer> as argument");
    return(APP_RC);
  }
  index = FIX2INT(fidx);

  if (index < 0) {
    arc_err_cstrfmt(c, "application of a vector expects type <non-negative exact integer> as argument");
    return(APP_RC);
  }

  if (index >= VECLEN(vec)) {
    arc_err_cstrfmt(c, "index %d out of range [0, %d] for vector",
		    index, VECLEN(vec)-1);
    return(APP_RC);
  }
  arc_thr_set_valr(c, thr, VINDEX(vec, index));
  return(APP_RC);
}

value arc_mkvector(arc *c, int length)
{
  value vec;
  int i;

  vec = arc_mkobject(c, (length+1)*sizeof(value), T_VECTOR);
  VINDEX(vec, -1) = INT2FIX(length);
  for (i=0; i<length; i++)
    VINDEX(vec, i) = CNIL;
  return(vec);
}

typefn_t __arc_vector_typefn__ = {
  __arc_vector_marker,
  __arc_null_sweeper,
  vector_pprint,
  NULL,
  NULL,
  __arc_vector_isocmp,
  vector_apply
};
