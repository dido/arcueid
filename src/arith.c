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

/* Type for fixnums */
arctype __arc_fixnum_t = { NULL, NULL, 0 };

/* Type for flonums */
arctype __arc_flonum_t = { NULL, NULL, sizeof(double) };

value arc_flonum_new(arc *c, double f)
{
  value fl = arc_new(c, &__arc_flonum_t, sizeof(double));
  double *flp = (double *)fl;
  *flp = f;
  return(fl);
}
