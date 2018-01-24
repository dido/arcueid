/* Copyright (C) 2017, 2018 Rafael R. Sevilla

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

static void mark(arc *c, value v,
		       void (*marker)(struct arc *, value, int),
		       int depth)
{
  value *vec = (value *)v;
  int i;

  for (i=1; i<=FIX2INT(vec[0]); i++)
    marker(c, vec[i], depth);
}

static enum arc_trstate apply(arc *c, value t, value v)
{
  /* XXX fill this in */
  return(TR_RC);
}

arctype __arc_vector_t = { NULL, mark, NULL, NULL, NULL, NULL, apply };

value arc_vector_new(arc *c, int size)
{
  value vecv = arc_new(c, &__arc_vector_t, sizeof(value)*(size+1));
  value *vec = (value *)vecv;
  int i;

  vec[0] = INT2FIX(size);
  for (i=1; i<=size; i++)
    vec[i] = CNIL;
  return(vecv);
}
