/* 
  Copyright (C) 2009 Rafael R. Sevilla

  This file is part of CArc

  CArc is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; either version 3 of the
  License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA
  02110-1301 USA.
*/
/* miscellaneous procedures and initialization */

#include "carc.h"
#include "alloc.h"
#include "../config.h"

value carc_is(carc *c, value v1, value v2)
{
  int i;

  /* An object is definitely the same as itself */
  if (v1 == v2)
    return(CTRUE);
  /* Two objects with different types are definitely not the same */
  if (TYPE(v1) != TYPE(v2))
    return(CNIL);

  switch (TYPE(v1)) {
#ifdef HAVE_GMP_H
  case T_BIGNUM:
    return((mpz_cmp(REP(v1)._bignum, REP(v2)._bignum) == 0) ? CTRUE : CNIL);
  case T_RATIONAL:
    return((mpq_equal(REP(v1)._rational, REP(v2)._rational)) ? CTRUE : CNIL);
#endif
  case T_FLONUM:
    return((REP(v1)._flonum == REP(v2)._flonum) ? CTRUE : CNIL);
  case T_COMPLEX:
    return((REP(v1)._complex.re == REP(v2)._complex.re)
	   && (REP(v1)._complex.im == REP(v2)._complex.im) ? CTRUE : CNIL);
  case T_CHAR:
    return((REP(v1)._char == REP(v2)._char) ? CTRUE : CNIL);
  case T_STRING:
    if (REP(v1)._str.length != REP(v2)._str.length)
      return(CNIL);
    for (i=0; i<REP(v1)._str.length; i++) {
      if (carc_strindex(c, v1, i) != carc_strindex(c, v2, i))
	return(CNIL);
    }
    return(CTRUE);
  }
  return(CNIL);
}

value carc_equal(carc *c, value v1, value v2)
{
  value elem1, elem2;
  int i;

  /* An object is definitely the same as itself */
  if (v1 == v2)
    return(CTRUE);
  /* Two objects with different types are definitely not the same */
  if (TYPE(v1) != TYPE(v2))
    return(CNIL);
  switch (TYPE(v1)) {
#ifdef HAVE_GMP_H
  case T_BIGNUM:
    return((mpz_cmp(REP(v1)._bignum, REP(v2)._bignum) == 0) ? CTRUE : CNIL);
  case T_RATIONAL:
    return((mpq_equal(REP(v1)._rational, REP(v2)._rational)) ? CTRUE : CNIL);
#endif
  case T_FLONUM:
    return((REP(v1)._flonum == REP(v2)._flonum) ? CTRUE : CNIL);
  case T_COMPLEX:
    return((REP(v1)._complex.re == REP(v2)._complex.re)
	   && (REP(v1)._complex.im == REP(v2)._complex.im) ? CTRUE : CNIL);
  case T_CHAR:
    return((REP(v1)._char == REP(v2)._char) ? CTRUE : CNIL);
  case T_STRING:
    if (REP(v1)._str.length != REP(v2)._str.length)
      return(CNIL);
    for (i=0; i<REP(v1)._str.length; i++) {
      if (carc_strindex(c, v1, i) != carc_strindex(c, v2, i))
	return(CNIL);
    }
    return(CTRUE);
  case T_CONS:
    /* XXX: this naive traversal will loop forever if there is a loop
       in the cons cells! We need something more sophisticated to do
       this properly.  Well, the reference Arc implementation by Paul
       Graham is no better... */
    for (; TYPE(v1) == T_CONS && TYPE(v2) == T_CONS;
	 v1 = cdr(v1), v2 = cdr(v2)) {
      elem1 = car(v1);
      elem2 = car(v2);
      if (!carc_equal(c, elem1, elem2))
	return(CNIL);
    }
    /* compare last cdr */
    return(carc_equal(c, v1, v2));
  }
  return(CNIL);
}

value scar(value x, value y)
{
  WB(&car(x), y);
  return(y);
}

value scdr(value x, value y)
{
  WB(&cdr(x), y);
  return(y);
}

value carc_mkcont(carc *c, int newpc)
{
  return(CNIL);
}

value carc_rcont(carc *c, value cont)
{
  return(CNIL);
}

value carc_mkvector(carc *c, int length)
{
  void *cellptr;
  value vect;

  cellptr = c->get_block(c, sizeof(struct cell) + sizeof(value)*length);
  if (cellptr == NULL)
    return(CNIL);
  vect = (value)cellptr;
  REP(vect)._vector.length = length;
  memset(REP(vect)._vector.data, 0, sizeof(value)*length);
  return(vect);
}
