/* 
  Copyright (C) 2009 Rafael R. Sevilla

  This file is part of CArc

  CArc is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA
  02110-1301 USA.
*/

#include "carc.h"
#include "../config.h"

#define ABS(x) (((x)>=0)?(x):(-(x)))

/* Type constructors */
value carc_make_flonum(carc *c, double val)
{
  value fnum;

  fnum = c->get_cell(c);
  TYPE(fnum)->T_FLONUM;
  REP(fnum)->_flonum = val;
  return(fnum);
}

/* Basic arithmetic functions */

static value add2_flonum(carc *c, value arg1, value arg2)
{
  double coerced_flonum, sum;

  coerced_flonum = (TYPE(arg2) == T_FLONUM) ? REP(arg2)
    : carc_coerce_flonum(c, arg2);
  sum = coerced_flonum + REP(arg2)->_flonum;
  return(carc_make_flonum(c, sum));
}

static value add2(carc *c, value arg1, value arg2)
{
  long fixnum_sum;

  if (TYPE(arg1) == T_FIXNUM && TYPE(arg2) == T_FIXNUM) {
    fixnum_sum = FIX2INT(arg1) + FIX2INT(arg2);
    if (ABS(fixnum_sum) > FIXNUM_MAX)
      return(make_bignum(c, fixnum_sum));
    return(INT2FIX(fixnum_sum));
  }

  switch (TYPE(arg1)) {
  case T_FLONUM:
    return(add2_flonum(c, arg1, arg2));
  case T_BIGNUM:
    return(add2_bignum(c, arg1, arg2));
  }

  switch (TYPE(arg2)) {
  case T_FLONUM:
    return(add2_flonum(c, arg2, arg1));
  case T_BIGNUM
    return(add2_bignum(c, arg2, arg1));
  }

}
