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

#include <stdlib.h>
#include <math.h>
#include <float.h>
#include "carc.h"
#include "arith.h"
#include "../config.h"

#define ABS(x) (((x)>=0)?(x):(-(x)))

double __carc_flonum_conv_tolerance = DBL_EPSILON;

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
  mpz_init(REP(bignum)._bignum);
  mpz_set_si(REP(bignum)._bignum, val);
  return(bignum);
#else
  c->signal_error(c, "Overflow error (this version of CArc does not have bignum support)");
  return(CNIL);
#endif
}

value carc_mkrationall(carc *c, long num, long den)
{
#ifdef HAVE_GMP_H
  value rat;

  rat = c->get_cell(c);
  BTYPE(rat) = T_RATIONAL;
  mpq_init(REP(rat)._rational);
  mpq_set_si(REP(rat)._rational, num, den);
  return(rat);
#else
  c->signal_error(c, "Overflow error (this version of CArc does not have bignum support)");
  return(CNIL);
#endif
}

value carc_mkrationalb(carc *c, value b)
{
#ifdef HAVE_GMP_H
  value rat;

  rat = c->get_cell(c);
  BTYPE(rat) = T_RATIONAL;
  mpq_init(REP(rat)._rational);
  mpq_set_z(REP(rat)._rational, REP(b)._bignum);
  return(rat);
#else
  c->signal_error(c, "Overflow error (this version of CArc does not have bignum support)");
  return(CNIL);
#endif
}


/* Type conversions */
double carc_coerce_flonum(carc *c, value v)
{
  double val;

  switch (TYPE(v)) {
#ifdef HAVE_GMP_H
  case T_BIGNUM:
    val = mpz_get_d(REP(v)._bignum);
    break;
  case T_RATIONAL:
    val = mpq_get_d(REP(v)._rational);
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

void carc_coerce_bignum(carc *c, value v, void *bptr)
{
#ifdef HAVE_GMP_H
  mpz_t *bignum = (mpz_t *)bptr;

  switch (TYPE(v)) {
  case T_BIGNUM:
    mpz_set(*bignum, REP(v)._bignum);
    break;
  case T_RATIONAL:
    mpz_cdiv_q(*bignum,
	       mpq_numref(REP(v)._rational),
	       mpq_denref(REP(v)._rational));
    break;
  case T_FLONUM:
    mpz_set_d(*bignum, REP(v)._flonum);
    break;
  case T_FIXNUM:
    mpz_set_si(*bignum, FIX2INT(v));
    break;
  default:
    c->signal_error(c, "Cannot convert operand %v into a bignum", v);
    break;
  }
#else
  c->signal_error(c, "Overflow error (no bignum support)");
#endif
}

void carc_coerce_rational(carc *c, value v, void *bptr)
{
#ifdef HAVE_GMP_H
  mpq_t *rat = (mpq_t *)bptr;

  switch (TYPE(v)) {
  case T_RATIONAL:
    mpq_set(*rat, REP(v)._rational);
    break;
  case T_BIGNUM:
    mpq_set_z(*rat, REP(v)._bignum);
    break;
  case T_FLONUM:
    mpq_set_d(*rat, REP(v)._flonum);
    break;
  case T_FIXNUM:
    mpq_set_si(*rat, FIX2INT(v), 1);
    break;
  default:
    c->signal_error(c, "Cannot convert operand %v into a rational", v);
    break;
  }
#else
  c->signal_error(c, "Overflow error (no bignum support)");
#endif
}

/* Attempt to coerce the value to a fixnum.  If the number is too large
   in magnitude to be converted, return nil. */
value carc_coerce_fixnum(carc *c, value v)
{
  long val;

  switch (TYPE(v)) {
  case T_FIXNUM:
    return(v);
  case T_FLONUM:
    if (ABS(REP(v)._flonum) > FIXNUM_MAX)
      return(CNIL);
    val = (long)REP(v)._flonum;
    return(INT2FIX(val));
  case T_BIGNUM:
#ifdef HAVE_GMP_H
    if (mpz_cmp_si(REP(v)._bignum, FIXNUM_MAX) <= 0
	&& mpz_cmp_si(REP(v)._bignum, FIXNUM_MIN) >= 0) {
      return(INT2FIX(mpz_get_si(REP(v)._bignum)));
    }
#else
    c->signal_error(c, "Overflow error (this version of CArc does not have bignum support)");
#endif
    break;
  case T_RATIONAL:
#ifdef HAVE_GMP_H
    if (mpz_cmp_si(mpq_denref(REP(v)._rational), 1)) {
      mpz_t temp;

      mpz_init(temp);
      mpz_cdiv_q(temp,
		 mpq_numref(REP(v)._rational),
		 mpq_denref(REP(v)._rational));
      if (mpz_cmp_si(temp, FIXNUM_MAX) <= 0
	  && mpz_cmp_si(temp, FIXNUM_MIN) >= 0) {
	value val;

	val = INT2FIX(mpz_get_si(temp));
	mpz_clear(temp);
	return(val);
      }
      mpz_clear(temp);
    }
#else
    c->signal_error(c, "Overflow error (this version of CArc does not have bignum support)");
#endif
    break;
  }
  return(CNIL);
}

/* Coerce a rational back to an integral type where possible.  Otherwise
   return v.  Used by most rational arithmetic operators. */
static value integer_coerce(carc *c, value v)
{
#ifdef HAVE_GMP_H
  value val;

  /* Attempt to coerce back to a fixnum if possible.  If the denominator
     is 1 in this case, try to convert back. */
  if (!mpz_cmp_si(mpq_denref(REP(v)._rational), 1)) {
    /* Not 1, cannot coerce */
    return(v);
  }

  /* It is an integer--try to convert to fixnum or bignum */
  val = carc_coerce_fixnum(c, v);
  if (val != CNIL)
    return(val);

  /* Coerce to bignum */
  val = carc_mkbignuml(c, 0);
  carc_coerce_bignum(c, val, &REP(v)._bignum);
  return(val);
#endif
}

/*================================= Basic arithmetic functions */

/* All arithmetic functions take two arguments, and return their
   result, not modifying their arguments.  A new cell is allocated
   from the heap for the result if this is needed.

   The following implicit type conversions occur for arithmetic
   operations:

              Fixnum   Bignum   Rational   Flonum   Complex
   Fixnum     Fixnum   Bignum   Rational   Flonum   Complex
   Bignum     Bignum   Bignum   Rational   Flonum   Complex
   Rational   Rational Rational Rational   Flonum   Complex
   Flonum     Flonum   Flonum   Flonum     Flonum   Complex
   Complex    Complex  Complex  Complex    Complex  Complex

   If a bignum result is smaller than ±FIXNUM_MAX, it is implicitly
   converted to a fixnum.

   If a rational has a denominator of 1, the result is implicitly
   converted to a fixnum, if the range allows, or a bignum if not.

   If a complex has an imaginary part of 0, the result is implicitly
   converted to a flonum.
 */
static value add2_flonum(carc *c, value arg1, value arg2)
{
  double coerced_flonum;

  coerced_flonum = (TYPE(arg2) == T_FLONUM) ? REP(arg2)._flonum
    : carc_coerce_flonum(c, arg2);
  coerced_flonum += REP(arg1)._flonum;
  return(carc_mkflonum(c, coerced_flonum));
}

static value add2_rational(carc *c, value arg1, value arg2)
{
#ifdef HAVE_GMP_H
  mpq_t coerced_rational;

  if (TYPE(arg2) == T_RATIONAL) {
    mpq_add(REP(arg1)._rational, REP(arg1)._rational, REP(arg2)._rational);
  } else {
    mpq_init(coerced_rational);
    carc_coerce_rational(c, arg2, &coerced_rational);
    mpq_add(REP(arg1)._rational, REP(arg1)._rational, coerced_rational);
    mpq_clear(coerced_rational);
  }

  return(integer_coerce(c, arg1));
#else
  c->signal_error(c, "Overflow error (no bignum support)");
  return(CNIL);
#endif
}

static value add2_bignum(carc *c, value arg1, value arg2)
{
#ifdef HAVE_GMP_H
  value sum, coerced_fixnum;

  switch (TYPE(arg2)) {
  case T_BIGNUM:
    sum = carc_mkbignuml(c, 0);
    mpz_add(REP(sum)._bignum, REP(arg1)._bignum, REP(arg2)._bignum);
    break;
  case T_RATIONAL:
    sum = carc_mkrationalb(c, arg1);
    return(add2_rational(c, arg1, sum));
    break;
  default:
    sum = carc_mkbignuml(c, 0);
    carc_coerce_bignum(c, arg2, &REP(sum)._bignum);
    mpz_add(REP(sum)._bignum, REP(arg1)._bignum, REP(sum)._bignum);
    break;
  }

  /* Attempt to coerce back to a fixnum if possible */
  coerced_fixnum = carc_coerce_fixnum(c, sum);
  return((coerced_fixnum == CNIL) ? sum : coerced_fixnum);
#else
  c->signal_error(c, "Overflow error (no bignum support)");
  return(CNIL);
#endif
}

value __carc_add2(carc *c, value arg1, value arg2)
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
  case T_RATIONAL:
    return(add2_rational(c, arg1, arg2));
  }

  switch (TYPE(arg2)) {
  case T_FLONUM:
    return(add2_flonum(c, arg2, arg1));
  case T_BIGNUM:
    return(add2_bignum(c, arg2, arg1));
  case T_RATIONAL:
    return(add2_rational(c, arg1, arg2));
  }

  c->signal_error(c, "Invalid types for addition");
  return(CNIL);
}

value __carc_neg(carc *c, value arg)
{
  switch (TYPE(arg)) {
  case T_FIXNUM:
    return(INT2FIX(-FIX2INT(arg)));
  case T_FLONUM:
    return(carc_mkflonum(c, -REP(arg)._flonum));
#ifdef HAVE_GMP_H
  case T_BIGNUM:
    {
      value big;
      big = carc_mkbignuml(c, 0);
      mpz_neg(REP(big)._bignum, REP(arg)._bignum);
      return(big);
    }
    break;
  case T_RATIONAL:
    {
      value rat;
      rat = carc_mkrationall(c, 0, 1);
      mpq_neg(REP(rat)._rational, REP(arg)._rational);
      return(rat);
    }
    break;
#endif
  default:
    c->signal_error(c, "Invalid type for negation");
  }
  return(CNIL);
}

/*
value __carc_sub2(carc *c, value arg1, value arg2)
{
  long fixnum_diff;

  if (TYPE(arg1) == T_FIXNUM && TYPE(arg2) == T_FIXNUM) {
    fixnum_diff = FIX2INT(arg1) - FIX2INT(arg2);
    if (ABS(fixnum_diff) > FIXNUM_MAX)
      return(carc_mkbignuml(c, fixnum_diff));
    return(INT2FIX(fixnum_diff));
  }

  return(__carc_add2(c, arg1, __carc_neg(c, arg2)));
}
*/

static value mul2_flonum(carc *c, value arg1, value arg2)
{
  double coerced_flonum;

  coerced_flonum = (TYPE(arg2) == T_FLONUM) ? REP(arg2)._flonum
    : carc_coerce_flonum(c, arg2);
  REP(arg1)._flonum *= coerced_flonum;
  return(arg1);
}

static value mul2_bignum(carc *c, value arg1, value arg2)
{
#ifdef HAVE_GMP_H
  mpz_t coerced_bignum;
  value coerced_fixnum;

  if (TYPE(arg2) == T_BIGNUM) {
    mpz_mul(REP(arg1)._bignum, REP(arg1)._bignum, REP(arg2)._bignum);
  } else {
    mpz_init(coerced_bignum);
    carc_coerce_bignum(c, arg2, &coerced_bignum);
    mpz_mul(REP(arg1)._bignum, REP(arg1)._bignum, coerced_bignum);
    mpz_clear(coerced_bignum);
  }
  /* Attempt to coerce back to a fixnum if possible */
  coerced_fixnum = carc_coerce_fixnum(c, arg1);
  return((coerced_fixnum == CNIL) ? arg1 : coerced_fixnum);
#else
  c->signal_error(c, "Overflow error (no bignum support)");
  return(CNIL);
#endif
}

value __carc_mul2(carc *c, value arg1, value arg2)
{

  if (TYPE(arg1) == T_FIXNUM && TYPE(arg2) == T_FIXNUM) {
    long varg1, varg2;

    varg1 = FIX2INT(arg1);
    varg2 = FIX2INT(arg2);
#if VALUE_SIZE == 8
    /* 64-bit platform.  We can mask against the high bits of the
       absolute value to determine whether or not bignum arithmetic
       is necessary. */
    if ((ABS(varg1) | ABS(varg2)) & 0xffffffff80000000) {
#elif VALUE_SIZE == 4
      /* 32-bit platform.  Similarly. */
    if ((ABS(varg1) | ABS(varg2)) & 0xffff8000) {
#else
    /* This rather complicated test is a (sorta) portable check for
       multiplication overflow.  If the product would overflow, we need to
       use bignum arithmetic.  This should work on any oddball platform
       no matter what the actual size of a value is. */
    if ((varg1 > 0 && varg2 > 0 && varg1 > (FIXNUM_MAX / varg2))
	|| (varg1 > 0 && varg2 <= 0 && (varg2 < (FIXNUM_MIN / varg1)))
	|| (varg1 <= 0 && varg2 > 0 && (varg1 < (FIXNUM_MIN / varg2)))
	|| (varg1 != 0 && (varg2 < (FIXNUM_MAX / varg1)))) {
#endif
      return(mul2_bignum(c, carc_mkbignuml(c, varg1), arg2));
    }
    return(INT2FIX(varg1 * varg2));
  }

  switch (TYPE(arg1)) {
  case T_FLONUM:
    return(mul2_flonum(c, arg1, arg2));
  case T_BIGNUM:
    return(mul2_bignum(c, arg1, arg2));
  }

  switch (TYPE(arg2)) {
  case T_FLONUM:
    return(mul2_flonum(c, arg2, arg1));
  case T_BIGNUM:
    return(mul2_bignum(c, arg2, arg1));
  }

  c->signal_error(c, "Invalid types for multiplication");
  return(CNIL);

}
