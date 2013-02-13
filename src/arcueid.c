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

static value cons_pprint(arc *c, value v)
{
  /* XXX fill this in */
  return(CNIL);
}

/* Mark the car and cdr */
static void cons_marker(arc *c, value v, int depth,
			void (*markfn)(arc *, value, int))
{
  markfn(c, car(v), depth);
  markfn(c, cdr(v), depth);
}

static void cons_sweeper(arc *c, value v)
{
  /* Does nothing */
}

value cons(arc *c, value x, value y)
{
  struct cell *cc;
  value ccv;

  cc = (struct cell *)c->alloc(c, sizeof(struct cell) + sizeof(value));
  cc->_type = T_CONS;
  cc->pprint = cons_pprint;
  cc->marker = cons_marker;
  cc->sweeper = cons_sweeper;
  ccv = (value)cc;
  car(ccv) = x;
  cdr(ccv) = y;
  return(ccv);
}
