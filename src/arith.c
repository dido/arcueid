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
#include <math.h>
#include <stdlib.h>

#define ABS(x) (((x)>=0)?(x):(-(x)))

/* Type constructors */
value carc_mkflonum(carc *c, double val)
{
  value fnum;

  fnum = c->get_cell(c);
  BTYPE(fnum) = T_FLONUM;
  REP(fnum)._flonum = val;
  return(fnum);
}

value carc_mkbignuml(carc *c, long val)
{
#ifdef HAVE_GMP_H
  value bignum;

  bignum = c->get_cell(c);
  BTYPE(bignum) = T_BIGNUM;
  mpq_init(REP(bignum)._bignum);
  mpq_set_si(REP(bignum)._bignum, val, 1);
  return(bignum);
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
  case T_BIGNUM:
    val = mpq_get_d(REP(v)._bignum);
    break;
#endif
  case T_FLONUM:
    return(REP(v)._flonum);
    break;
  case T_FIXNUM:
    val = (double)FIX2INT(v);
    break;
  default:
    c->signal_error(c, "Cannot convert operand %v into a flonum", v);
    break;
  }
  return(val);
}

void carc_coerce_bignum(carc *c, value v, void *bignumptr)
{
#ifdef HAVE_GMP_H
  mpq_t *bignum = (mpq_t *)bignumptr;
  switch (TYPE(v)) {
  case T_BIGNUM:
    bignum = &REP(v)._bignum;
    break;
  case T_FLONUM:
    mpq_set_d(*bignum, REP(v)._flonum);
    break;
  case T_FIXNUM:
    mpq_set_si(*bignum, FIX2INT(v), 1);
    break;
  default:
    c->signal_error(c, "Cannot convert operand %v into a bignum", v);
    break;
  }
#else
  c->signal_error(c, "Overflow error (no bignum support)");
#endif
}

/* Attempt to coerce the value to a fixnum.  If this is not possible,
   return CNIL. */
value carc_coerce_fixnum(carc *c, value v)
{
  long val;

  switch (TYPE(v)) {
  case T_FIXNUM:
    return(v);
  case T_FLONUM:
    val = (long)REP(v)._flonum;
    if (abs(val) > FIXNUM_MAX)
      return(CNIL);
    return(INT2FIX(val));
  case T_BIGNUM:
#ifdef HAVE_GMP_H
    if (mpq_cmp_si(REP(v)._bignum, FIXNUM_MAX, 1) <= 0
	&& mpq_cmp_si(REP(v)._bignum, FIXNUM_MIN, 1) >= 0) {
      long num, den;

      num = mpz_get_si(mpq_numref(REP(v)._bignum));
      den = mpz_get_si(mpq_denref(REP(v)._bignum));
      return(INT2FIX(num/den));
    }
#else
    c->signal_error(c, "Overflow error (this version of CArc does not have bignum support)");
#endif
  }
  return(CNIL);
}

/* Basic arithmetic functions */

static value add2_flonum(carc *c, value arg1, value arg2)
{
  double coerced_flonum;

  coerced_flonum = (TYPE(arg2) == T_FLONUM) ? REP(arg2)._flonum
    : carc_coerce_flonum(c, arg2);
  REP(arg1)._flonum += coerced_flonum;
  return(arg1);
}

static value add2_bignum(carc *c, value arg1, value arg2)
{
#ifdef HAVE_GMP_H
  mpq_t coerced_bignum;
  value coerced_fixnum;

  if (TYPE(arg2) == T_BIGNUM) {
    mpq_add(REP(arg1)._bignum, REP(arg1)._bignum, REP(arg2)._bignum);
  } else {
    mpq_init(coerced_bignum);
    carc_coerce_bignum(c, arg2, &coerced_bignum);
    mpq_add(REP(arg1)._bignum, REP(arg1)._bignum, coerced_bignum);
    mpq_clear(coerced_bignum);
  }
  /* Attempt to coerce back to a fixnum if possible */
  coerced_fixnum = carc_coerce_fixnum(c, arg1);
  return((coerced_fixnum == CNIL) ? arg1 : coerced_fixnum);
#else
  c->signal_error(c, "Overflow error (no bignum support)");
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
  case T_BIGNUM:
    return(add2_bignum(c, arg2, arg1));
  }

  c->signal_error(c, "Invalid types for addition");
  return(CNIL);
}

value carc_arith_op(carc *c, int opval, value args)
{
  value x, v = INT2FIX(0);
  value (*op)(carc *, value, value);

  switch (opval) {
  case '+':
    op = add2;
    break;
  default:
    c->signal_error(c, "Invalid operator %c");
  }

  for (x=args; x != CNIL; x=cdr(x))
    v = op(c, v, car(x));
  return(v);
}
