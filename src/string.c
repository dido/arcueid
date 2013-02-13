/* 
  Copyright (C) 2012 Rafael R. Sevilla

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
#include <inttypes.h>
#include <string.h>
#include "arcueid.h"
#include "utf.h"

typedef struct {
  int len;
  Rune str[1];
} string;

#define STRSIZE ((long)((string *)0)->str)

static value string_pprint(arc *c, value v)
{
  /* XXX fill this in */
  return(CNIL);
}

value arc_mkstringlen(arc *c, int length)
{
  struct cell *ss;

  ss = (struct cell *)c->alloc(c, CELLSIZE + STRSIZE + length*sizeof(Rune));
  ss->_type = T_STRING;
  ss->pprint = string_pprint;
  ss->marker = __arc_null_marker;
  ss->sweeper = __arc_null_sweeper;
  return((value)ss);
}
