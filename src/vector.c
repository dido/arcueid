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

value __arc_vector_hash(arc *c, value v, arc_hs *s, value visithash)
{
  unsigned long len;
  int i;

  if (visithash == CNIL)
    visithash = arc_mkhash(c, ARC_HASHBITS);
  if (__arc_visit(c, v, visithash) != CNIL) {
    /* Already visited at some point.  Do not recurse further.  An already
       visited node will still contribute 1 to the length though. */
    return(1);
  }
  len = 0;
  for (i=0; i<VECLEN(v); i++)
    len += arc_hash(c, VINDEX(v, i), visithash);
  return(len);
}

value __arc_vector_isocmp(arc *c, value v1, value v2, value vh1, value vh2)
{
  value vhh1, vhh2;
  int i;

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
  /* Vectors must be identical in length to be the same */
  if (VECLEN(v1) != VECLEN(v2))
    return(CNIL);
  /* Recursive comparisons */
  for (i=0; i<VECLEN(v1); i++) {
    if (arc_iso(c, VINDEX(v1, i), VINDEX(v2, i), vh1, vh2) != CTRUE)
      return(CNIL);
  }
  return(CTRUE);
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
  __arc_vector_hash,
  NULL,
  __arc_vector_isocmp
};
