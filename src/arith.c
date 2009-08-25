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
value carc_mkflonum(carc *c, double val)
{
  value fnum;

  fnum = c->get_cell(c);
  BTYPE(fnum) = T_FLONUM;
  REP(fnum)->_flonum = val;
  return(fnum);
}

value carc_mkbignuml(carc *c, long val)
{
#ifdef HAVE_GMP_H
  value bignum;

  bignum = c->get_cell(c);
  BTYPE(bignum) = T_BIGNUM;
  mpq_init(REP(bignum)->_bignum);
  mpq_set_si(REP(bignum)->_bignum, val, 1);
  return(fnum)
#else
  c->signal_error(c, "Overflow error (this version of CArc does not have bignum support)");
#endif
}

/* Type conversions */
double carc_coerce_flonum(carc *c, value v)
{
  double val;

  switch (TYPE(v)) {
#ifdef HAVE_GMP_H
  case BIGNUM:
    val = mpq_get_d(REP(v)->_bignum);
    break;
#endif
  case FLONUM:
    return(v);
    break;
  case FIXNUM:
    val = (double)FIX2INT(v);
    break;
  default:
    c->signal_error(c, "Cannot coerce %v into flonum", v);
    break;
  }
  return(val);
}

void carc_coerce_bignum(carc *c, value v, void *bignumptr)
{
#ifdef HAVE_GMP_H
  mpq_t *bignum = (mpq_t *)bignumptr;
  switch (TYPE(v)) {
  case BIGNUM:
    bignum = REP(v)->_bignum;
    break;
  case FLONUM:
    mpq_set_d(*bignum, REP(v)->_flonum);
    break;
  case FIXNUM:
    mpq_set_si(*bignum, FIX2INT(v));
    break;
  default:
    c->signal_error(c, "Cannot coerce %v into bignum", v);
    break;
  }
#else
  c->signal_error(c, "Overflow error (no bignum support)", v);
#endif
}

/* Basic arithmetic functions */

static value add2_flonum(carc *c, value arg1, value arg2)
{
  double coerced_flonum, sum;

  coerced_flonum = (TYPE(arg2) == T_FLONUM) ? REP(arg2)
    : carc_coerce_flonum(c, arg2);
  REP(arg1)->_flonum += coerced_flonum;
  return(arg1);
}

static value add2_bignum(carc *c, value arg1, value arg2)
{
#ifdef HAVE_GMP_H
  mpq_t coerced_bignum;

  coerced_bignum = (TYPE(arg2) == T_BIGNUM) ? arg2 : carc_coerce_bignum(arg2);
  sum = 
#else
#endif
}

static value add2(carc *c, value arg1, value arg2)
{
  long fixnum_sum;

  if (TYPE(arg1) == T_FIXNUM && TYPE(arg2) == T_FIXNUM) {
    fixnum_sum = FIX2INT(arg1) + FIX2INT(arg2);
    if (ABS(fixnum_sum) > FIXNUM_MAX)
      return(carc_mkbignuml(c, fixnum_sum));
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

  c->signal_error(c, "Invalid types for addition");
}
