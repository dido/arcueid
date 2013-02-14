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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <float.h>
#include <inttypes.h>
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


void __arc_null_marker(arc *c, value v, int depth,
			void (*markfn)(arc *, value, int))
{
  /* Does nothing */
}

void __arc_null_sweeper(arc *c, value v)
{
  /* Does nothing */
}

value arc_prettyprint(arc *c, value sexpr, value *ppstr)
{
  switch (TYPE(sexpr)) {
  case T_NIL:
    __arc_append_cstring(c, "nil", ppstr);
    break;
  case T_TRUE:
    __arc_append_cstring(c, "t", ppstr);
    break;
  case T_FIXNUM:
    {
      long val = FIX2INT(sexpr);
      int len;
      char *outstr;

      len = snprintf(NULL, 0, "%ld", val) + 1;
      outstr = (char *)alloca(sizeof(char)*len);
      snprintf(outstr, len+1, "%ld", val);
      __arc_append_cstring(c, outstr, ppstr);
    }
    break;
  case T_SYMBOL:
    /* XXX - handle this case */
    break;
  case T_NONE:
    /* XXX - this is an error case that needs handling */
    break;
  default:
    /* non-immediate type */
    ((struct cell *)(sexpr))->pprint(c, sexpr, ppstr);
    break;
  }
  return(*ppstr);
}

static value cons_pprint(arc *c, value sexpr, value *ppstr)
{
  __arc_append_cstring(c, "(", ppstr);
  while (TYPE(sexpr) == T_CONS) {
    arc_prettyprint(c, car(sexpr), ppstr);
    sexpr = cdr(sexpr);
    if (!NIL_P(sexpr))
      __arc_append_cstring(c,  " ", ppstr);
  }

  if (sexpr != CNIL) {
    __arc_append_cstring(c,  ". ", ppstr);
    arc_prettyprint(c, sexpr, ppstr);
  }
  __arc_append_cstring(c, ")", ppstr);
  return(*ppstr);
}

/* Mark the car and cdr */
static void cons_marker(arc *c, value v, int depth,
			void (*markfn)(arc *, value, int))
{
  markfn(c, car(v), depth);
  markfn(c, cdr(v), depth);
}

value cons(arc *c, value x, value y)
{
  struct cell *cc;
  value ccv;

  cc = (struct cell *)c->alloc(c, sizeof(struct cell) + sizeof(value));
  cc->_type = T_CONS;
  cc->pprint = cons_pprint;
  cc->marker = cons_marker;
  cc->sweeper = __arc_null_sweeper;
  ccv = (value)cc;
  car(ccv) = x;
  cdr(ccv) = y;
  return(ccv);
}
