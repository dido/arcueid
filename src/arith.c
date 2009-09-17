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

/* Type constructors */
value carc_mkflonum(carc *c, double val)
{
  value fnum;

  fnum = c->get_cell(c);
  BTYPE(fnum) = T_FLONUM;
  REP(fnum)._flonum = val;
  return(fnum);
}

value carc_mkcomplex(carc *c, double re, double im)
{
  value cnum;

  cnum = c->get_cell(c);
  BTYPE(cnum) = T_COMPLEX;
  REP(cnum)._complex.re = re;
  REP(cnum)._complex.im = im;
  return(cnum);
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
  mpq_canonicalize(REP(rat)._rational);
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

void carc_coerce_complex(carc *c, value v, double *re, double *im)
{
  switch (TYPE(v)) {
#ifdef HAVE_GMP_H
  case T_BIGNUM:
    *re = mpz_get_d(REP(v)._bignum);
    break;
  case T_RATIONAL:
    *re = mpq_get_d(REP(v)._rational);
    break;
#endif
  case T_FLONUM:
    *re = REP(v)._flonum;
    break;
  case T_COMPLEX:
    *re = REP(v)._complex.re;
    *im = REP(v)._complex.im;
    return;
    break;
  case T_FIXNUM:
    *re = (double)FIX2INT(v);
    break;
  default:
    c->signal_error(c, "Cannot convert operand %v into a flonum", v);
    break;
  }
  *im = 0.0;
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
    {
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
  if (mpz_cmp_si(mpq_denref(REP(v)._rational), 1) != 0) {
    /* Not 1, cannot coerce */
    return(v);
  }

  /* It is an integer--try to convert to fixnum or bignum */
  val = carc_coerce_fixnum(c, v);
  if (val != CNIL)
    return(val);

  /* Coerce to bignum */
  val = carc_mkbignuml(c, 0);
  carc_coerce_bignum(c, v, &REP(val)._bignum);
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
   Fixnum     Fixnum*  Bignum   Rational   Flonum   Complex
   Bignum     Bignum   Bignum   Rational   Flonum   Complex
   Rational   Rational Rational Rational   Flonum   Complex
   Flonum     Flonum   Flonum   Flonum     Flonum   Complex
   Complex    Complex  Complex  Complex    Complex  Complex

   * If a bignum result is smaller than ±FIXNUM_MAX, it is
   implicitly converted to a fixnum.  If arithmetic on fixnums
   would give a result greater than ±FIXNUM_MAX, it will
   automatically extend to a bignum unless bignum support is not
   compiled in (in which case an overflow error is signaled).

   If a rational has a denominator of 1, the result is implicitly
   converted to a fixnum, if the range allows, or a bignum if not.

   Division of fixnums and/or bignums will result in a fixnum or a
   bignum only if the two numbers divide each other exactly.  If the
   division is inexact, the result will be a rational number if
   support for bignum/rational arithmetic has been compiled in.  If
   not, the quotient alone is returned.
 */

static inline value add2_complex(carc *c, value arg1, double re, double im)
{
  re += REP(arg1)._complex.re;
  im += REP(arg1)._complex.im;
  return(carc_mkcomplex(c, re, im));
}

static inline value add2_flonum(carc *c, value arg1, double arg2)
{
  arg2 += REP(arg1)._flonum;
  return(carc_mkflonum(c, arg2));
}

#ifdef HAVE_GMP_H
static inline void add2_rational(carc *c, value arg1, mpq_t *arg2)
{
  mpq_add(*arg2, REP(arg1)._rational, *arg2);
}

static inline void add2_bignum(carc *c, value arg1, mpz_t *arg2)
{
  mpz_add(*arg2, REP(arg1)._bignum, *arg2);
}
#endif

#define COERCE_OP_COMPLEX(func, arg1, arg2) {	\
    double re, im;				\
    carc_coerce_complex(c, arg2, &re, &im);	\
    return(func(c, arg1, re, im));		\
  }

#define COERCE_OP_FLONUM(func, arg1, arg2) {	\
    double f;					\
    f = carc_coerce_flonum(c, arg2);		\
    return(func(c, arg1, f));			\
  }

#define COERCE_OP_RATIONAL(func, arg1, arg2) {		\
    value v;						\
    v = carc_mkrationall(c, 0, 1);			\
    carc_coerce_rational(c, arg2, &(REP(v)._rational)); \
    func(c, arg1, &(REP(v)._rational));			\
    return(integer_coerce(c, v));			\
  }

#define COERCE_OP_BIGNUM(func, arg1, arg2) {			\
    value v, cf;						\
    v = carc_mkbignuml(c, 0);					\
    carc_coerce_bignum(c, arg2, &(REP(v)._bignum));		\
    func(c, arg1, &(REP(v)._bignum));				\
    cf = carc_coerce_fixnum(c, v);				\
    return((cf == CNIL) ? v : cf);				\
  }

#ifdef HAVE_GMP_H

#define TYPE_CASES(func, arg1, arg2) {			\
    if (TYPE(arg1) == T_COMPLEX) {			\
      COERCE_OP_COMPLEX(func##2_complex, arg1, arg2);	\
    } else if (TYPE(arg2) == T_COMPLEX) {		\
      COERCE_OP_COMPLEX(func##2_complex, arg2, arg1);	\
    } else if (TYPE(arg1) == T_FLONUM) {		\
      COERCE_OP_FLONUM(func##2_flonum, arg1, arg2);	\
    } else if (TYPE(arg2) == T_FLONUM) {		\
      COERCE_OP_FLONUM(func##2_flonum, arg2, arg1);	\
    } else if (TYPE(arg1) == T_RATIONAL) {		\
      COERCE_OP_RATIONAL(func##2_rational, arg1, arg2);	\
    } else if (TYPE(arg2) == T_RATIONAL) {		\
      COERCE_OP_RATIONAL(func##2_rational, arg2, arg1);	\
    } else if (TYPE(arg1) == T_BIGNUM) {		\
      COERCE_OP_BIGNUM(func##2_bignum, arg1, arg2);	\
    } else if (TYPE(arg2) == T_BIGNUM) {		\
      COERCE_OP_BIGNUM(func##2_bignum, arg2, arg1);	\
    }							\
  }
#else
#define TYPE_CASES(func, arg1, arg2) {			\
    if (TYPE(arg1) == T_COMPLEX) {			\
      COERCE_OP_COMPLEX(func##2_complex, arg1, arg2);	\
    } else if (TYPE(arg2) == T_COMPLEX) {		\
      COERCE_OP_COMPLEX(func##2_complex, arg2, arg1);	\
    } else if (TYPE(arg1) == T_FLONUM) {		\
      COERCE_OP_FLONUM(func##2_flonum, arg1, arg2);	\
    } else if (TYPE(arg2) == T_FLONUM) {		\
      COERCE_OP_FLONUM(func##2_flonum, arg2, arg1);	\
    }							\
  }
#endif

value __carc_add2(carc *c, value arg1, value arg2)
{
  long fixnum_sum;

  if (TYPE(arg1) == T_FIXNUM && TYPE(arg2) == T_FIXNUM) {
    fixnum_sum = FIX2INT(arg1) + FIX2INT(arg2);
    if (ABS(fixnum_sum) > FIXNUM_MAX)
      return(carc_mkbignuml(c, fixnum_sum));
    return(INT2FIX(fixnum_sum));
  } 
  TYPE_CASES(add, arg1, arg2);

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
  case T_COMPLEX:
    return(carc_mkcomplex(c, -REP(arg)._complex.re, -REP(arg)._complex.im));
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

static inline value mul2_complex(carc *c, value arg1, double re, double im)
{
  double r2, i2;

  r2 = REP(arg1)._complex.re * re - REP(arg1)._complex.im * im;
  i2 = REP(arg1)._complex.im * re + REP(arg1)._complex.re * im;
  return(carc_mkcomplex(c, r2, i2));
}


static inline value mul2_flonum(carc *c, value arg1, double arg2)
{
  arg2 *= REP(arg1)._flonum;
  return(carc_mkflonum(c, arg2));
}

#ifdef HAVE_GMP_H
static inline void mul2_rational(carc *c, value arg1, mpq_t *arg2)
{
  mpq_mul(*arg2, REP(arg1)._rational, *arg2);
}

static inline void mul2_bignum(carc *c, value arg1, mpz_t *arg2)
{
  mpz_mul(*arg2, REP(arg1)._bignum, *arg2);
}
#endif

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
    if ((ABS(varg1) | ABS(varg2)) & 0xffffffff80000000)
#elif VALUE_SIZE == 4
      /* 32-bit platform.  Similarly. */
    if ((ABS(varg1) | ABS(varg2)) & 0xffff8000)
#else
    /* This rather complicated test is a (sorta) portable check for
       multiplication overflow.  If the product would overflow, we need to
       use bignum arithmetic.  This should work on any oddball platform
       no matter what the actual size of a value is. */
    if ((varg1 > 0 && varg2 > 0 && varg1 > (FIXNUM_MAX / varg2))
	|| (varg1 > 0 && varg2 <= 0 && (varg2 < (FIXNUM_MIN / varg1)))
	|| (varg1 <= 0 && varg2 > 0 && (varg1 < (FIXNUM_MIN / varg2)))
	|| (varg1 != 0 && (varg2 < (FIXNUM_MAX / varg1))))
#endif
    {
#ifdef HAVE_GMP_H
      value v1 = carc_mkbignuml(c, varg1);
      COERCE_OP_BIGNUM(mul2_bignum, v1, arg2);
#else
      c->signal_error(c, "Overflow error in multiplication");
      return(CNIL);
#endif
    }
    return(INT2FIX(varg1 * varg2));
  }

  TYPE_CASES(mul, arg1, arg2);

  c->signal_error(c, "Invalid types for multiplication");
  return(CNIL);

}

static inline value sub2_complex(carc *c, value arg1, double re, double im)
{
  re = REP(arg1)._complex.re - re;
  im = REP(arg1)._complex.im - im;
  return(carc_mkcomplex(c, re, im));
}

static inline value sub2r_complex(carc *c, value arg1, double re, double im)
{
  re -= REP(arg1)._complex.re;
  im -= REP(arg1)._complex.im;
  return(carc_mkcomplex(c, re, im));
}

static inline value sub2_flonum(carc *c, value arg1, double arg2)
{
  arg2 = REP(arg1)._flonum - arg2;
  return(carc_mkflonum(c, arg2));
}

static inline value sub2r_flonum(carc *c, value arg1, double arg2)
{
  arg2 -= REP(arg1)._flonum;
  return(carc_mkflonum(c, arg2));
}

#ifdef HAVE_GMP_H
static inline void sub2_rational(carc *c, value arg1, mpq_t *arg2)
{
  mpq_sub(*arg2, REP(arg1)._rational, *arg2);
}

static inline void sub2r_rational(carc *c, value arg1, mpq_t *arg2)
{
  mpq_sub(*arg2, *arg2, REP(arg1)._rational);
}

static inline void sub2_bignum(carc *c, value arg1, mpz_t *arg2)
{
  mpz_sub(*arg2, REP(arg1)._bignum, *arg2);
}

static inline void sub2r_bignum(carc *c, value arg1, mpz_t *arg2)
{
  mpz_sub(*arg2, *arg2, REP(arg1)._bignum);
}
#endif

value __carc_sub2(carc *c, value arg1, value arg2)
{
  long fixnum_diff;

  if (TYPE(arg1) == T_FIXNUM && TYPE(arg2) == T_FIXNUM) {
    fixnum_diff = FIX2INT(arg1) - FIX2INT(arg2);
    if (ABS(fixnum_diff) > FIXNUM_MAX)
      return(carc_mkbignuml(c, fixnum_diff));
    return(INT2FIX(fixnum_diff));
  } 

  if (TYPE(arg1) == T_COMPLEX) {
    COERCE_OP_COMPLEX(sub2_complex, arg1, arg2);
  } else if (TYPE(arg2) == T_COMPLEX) {
    COERCE_OP_COMPLEX(sub2r_complex, arg2, arg1);
  } else if (TYPE(arg1) == T_FLONUM) {
    COERCE_OP_FLONUM(sub2_flonum, arg1, arg2);
  } else if (TYPE(arg2) == T_FLONUM) {
    COERCE_OP_FLONUM(sub2r_flonum, arg2, arg1);
  }
#ifdef HAVE_GMP_H
  else if (TYPE(arg1) == T_RATIONAL) {
    COERCE_OP_RATIONAL(sub2_rational, arg1, arg2);
  } else if (TYPE(arg2) == T_RATIONAL) {
    COERCE_OP_RATIONAL(sub2r_rational, arg2, arg1);
  } else if (TYPE(arg1) == T_BIGNUM) {
    COERCE_OP_BIGNUM(sub2_bignum, arg1, arg2);
  } else if (TYPE(arg2) == T_BIGNUM) {
    COERCE_OP_BIGNUM(sub2r_bignum, arg2, arg1);
  }
#endif

  c->signal_error(c, "Invalid types for subtraction");
  return(CNIL);
}

static inline value div2_complex(carc *c, value arg1, double re, double im)
{
  double den = (re*re + im*im);
  double r2, i2;

  r2 = (REP(arg1)._complex.re * re + REP(arg1)._complex.im * im)/den;
  i2 = (REP(arg1)._complex.im * re - REP(arg1)._complex.re * im)/den;
  return(carc_mkcomplex(c, r2, i2));
}

static inline value div2r_complex(carc *c, value arg1, double re, double im)
{
  double r1 = REP(arg1)._complex.re, i1 = REP(arg1)._complex.im, r2, i2;
  double den = r1*r1 + i1*i1;

  r2 = (REP(arg1)._complex.re * re + REP(arg1)._complex.im * im)/den;
  i2 = im*(REP(arg1)._complex.re - re*REP(arg1)._complex.im)/den;
  return(carc_mkcomplex(c, r2, i2));
}

static inline value div2_flonum(carc *c, value arg1, double arg2)
{
  arg2 = REP(arg1)._flonum / arg2;
  return(carc_mkflonum(c, arg2));
}

static inline value div2r_flonum(carc *c, value arg1, double arg2)
{
  arg2 /= REP(arg1)._flonum;
  return(carc_mkflonum(c, arg2));
}

#ifdef HAVE_GMP_H
static inline void div2_rational(carc *c, value arg1, mpq_t *arg2)
{
  mpq_div(*arg2, REP(arg1)._rational, *arg2);
}

static inline void div2r_rational(carc *c, value arg1, mpq_t *arg2)
{
  mpq_div(*arg2, *arg2, REP(arg1)._rational);
}

/* Division of two bignums may change the type of the value.  This
   assumes both arg1 and arg2 are bignums initially. This changes
   arg2. */
static inline void div2_bignum(carc *c, value arg1, value arg2)
{
  mpz_t t;
  value cf;

  if (mpz_divisible_p(REP(arg1)._bignum, REP(arg2)._bignum)) {
    mpz_divexact(REP(arg2)._bignum, REP(arg1)._bignum, REP(arg2)._bignum);
    return;
  }
  mpz_init(t);
  mpz_set(t, REP(arg2)._bignum);
  mpz_clear(REP(arg2)._bignum);
  mpq_init(REP(arg2)._rational);
  BTYPE(arg2) = T_RATIONAL;
  mpq_set_num(REP(arg2)._rational, REP(arg1)._bignum);
  mpq_set_den(REP(arg2)._rational, t);
  mpz_clear(t);
  return;
}

/* This divides the two values in reverse, storing the result in arg2 */
static inline void div2r_bignum(carc *c, value arg1, value arg2)
{
  mpz_t t;
  value cf;

  if (mpz_divisible_p(REP(arg1)._bignum, REP(arg2)._bignum)) {
    mpz_divexact(REP(arg2)._bignum, REP(arg2)._bignum, REP(arg1)._bignum);
    return;
  }
  mpz_init(t);
  mpz_set(t, REP(arg2)._bignum);
  mpz_clear(REP(arg2)._bignum);
  mpq_init(REP(arg2)._rational);
  BTYPE(arg2) = T_RATIONAL;
  mpq_set_num(REP(arg2)._rational, t);
  mpq_set_den(REP(arg2)._rational, REP(arg1)._bignum);
  mpz_clear(t);
  return;
}

#endif

value __carc_div2(carc *c, value arg1, value arg2)
{
  ldiv_t res;

  if (TYPE(arg1) == T_FIXNUM && TYPE(arg2) == T_FIXNUM) {
    long varg1, varg2;

    varg1 = FIX2INT(arg1);
    varg2 = FIX2INT(arg2);

    res = ldiv(varg1, varg2);
#ifdef HAVE_GMP_H
    if (res.rem != 0) {
      return(carc_mkrationall(c, varg1, varg2));
    } else {
      if (ABS(res.quot) > FIXNUM_MAX)
	return(carc_mkbignuml(c, res.quot));
#endif
      /* The conditional compilation produces only this if
	 we don't have GMP */
      return(INT2FIX(res.quot));
#ifdef HAVE_GMP_H
    }
#endif
  } 

  if (TYPE(arg1) == T_COMPLEX) {
    COERCE_OP_COMPLEX(div2_complex, arg1, arg2);
  } else if (TYPE(arg2) == T_COMPLEX) {
    COERCE_OP_COMPLEX(div2r_complex, arg2, arg1);
  } else if (TYPE(arg1) == T_FLONUM) {
    COERCE_OP_FLONUM(div2_flonum, arg1, arg2);
  } else if (TYPE(arg2) == T_FLONUM) {
    COERCE_OP_FLONUM(div2r_flonum, arg2, arg1);
  }
#ifdef HAVE_GMP_H
  else if (TYPE(arg1) == T_RATIONAL) {
    COERCE_OP_RATIONAL(div2_rational, arg1, arg2);
  } else if (TYPE(arg2) == T_RATIONAL) {
    COERCE_OP_RATIONAL(div2r_rational, arg2, arg1);
  } else if (TYPE(arg1) == T_BIGNUM) {
    value cf, v;
    /* The old macros don't work here because bignum division
       can result in a rational result. */

    v = carc_mkbignuml(c, 0);
    carc_coerce_bignum(c, arg2, &REP(v)._bignum);
    div2_bignum(c, arg1, v);
    if (TYPE(v) == T_BIGNUM) {
      cf = carc_coerce_fixnum(c, v);
      return((cf == CNIL) ? v : cf);
    }
    return(v);
  } else if (TYPE(arg2) == T_BIGNUM) {
    value cf, v;

    v = carc_mkbignuml(c, 0);
    carc_coerce_bignum(c, arg1, &REP(v)._bignum);
    div2r_bignum(c, arg2, v);
    if (TYPE(v) == T_BIGNUM) {
      cf = carc_coerce_fixnum(c, v);
      return((cf == CNIL) ? v : cf);
    }
    return(v);
  }
#endif

  c->signal_error(c, "Invalid types for division");
  return(CNIL);
}
