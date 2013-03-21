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

/* Blank continuation */
static value mkcont(arc *c)
{
  value cont = arc_mkvector(c, CONT_SIZE);

  ((struct cell *)cont)->_type = T_CONT;
  return(cont);
}

value __arc_mkcont(arc *c, value thr, int offset)
{
  value cont = mkcont(c);

  CONT_OFS(cont) = INT2FIX(offset);
  CONT_ENV(cont) = TENVR(thr);
  CONT_FUN(cont) = TFUNR(thr);
  CONT_ARGC(cont) = INT2FIX(TARGC(thr));
  CONT_CONT(cont) = TCONR(thr);
  CONT_SP(cont) = INT2FIX(TSP(thr) - TSBASE(thr));
  return(cont);
}

void arc_restorecont(arc *c, value thr, value cont)
{
  int offset;

  /* restore function and environment */
  TFUNR(thr) = CONT_FUN(cont);
  TENVR(thr) = CONT_ENV(cont);
  TARGC(thr) = FIX2INT(CONT_ARGC(cont));
  TCONR(thr) = CONT_CONT(cont);
  TSP(thr) = TSBASE(thr) + FIX2INT(CONT_SP(cont));
  offset = FIX2INT(CONT_OFS(cont));
  if (TYPE(TFUNR(thr)) == T_CCODE) {
    TIP(thr).aff_line = offset;
    return;
  }
  /* XXX - work out what to do for a closure-based continuation */
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
