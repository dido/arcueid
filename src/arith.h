/* 
  Copyright (C) 2013 Rafael R. Sevilla

  This file is part of Arcueid

  Arcueid is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __ARITH_H__

#define __ARITH_H__

#include "arcueid.h"

#define REPFLO(f) *((double *)REP(f))
#define REPCPX(z) *((double complex *)REP(z))

extern value arc_mkflonum(arc *c, double val);
extern value arc_mkcomplex(arc *c, double complex z);
extern value arc_mkbignuml(arc *c, long val);

extern value __arc_add2(arc *c, value arg1, value arg2);

#endif
