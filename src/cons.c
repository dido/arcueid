/* 
  Copyright (C) 2017,2018 Rafael R. Sevilla

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

static void consmark(arc *c, value v,
		     void (*marker)(struct arc *, value, int),
		     int depth)
{
  cons_t *cc = (cons_t *)v;
  /* Recursively mark the car and cdr */
  marker(c, cc->car, depth);
  marker(c, cc->cdr, depth);
}

arctype __arc_cons_t = { NULL, consmark, sizeof(cons_t) };
