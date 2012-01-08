/* 
  Copyright (C) 2010 Rafael R. Sevilla

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

#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <complex.h>
#include "arcueid.h"
#include "arith.h"
#include "../config.h"
#include "utf.h"

#define ABS(x) (((x)>=0)?(x):(-(x)))
#define SGN(x) (((x)>=0)?(1):(-(1)))

/* Type constructors */
value arc_mkflonum(arc *c, double val)
{
  value fnum;

  fnum = c->get_cell(c);
  BTYPE(fnum) = T_FLONUM;
  REP(fnum)._flonum = val;
  return(fnum);
}

value arc_mkcomplex(arc *c, double re, double im)
{
  value cnum;

  cnum = c->get_cell(c);
  BTYPE(cnum) = T_COMPLEX;
  REP(cnum)._complex.re = re;
  REP(cnum)._complex.im = im;
  return(cnum);
}

value arc_mkbignuml(arc *c, long val)
{
#ifdef HAVE_GMP_H
  value bignum;

  bignum = c->get_cell(c);
  BTYPE(bignum) = T_BIGNUM;
  mpz_init(REP(bignum)._bignum);
  mpz_set_si(REP(bignum)._bignum, val);
  return(bignum);
#else
  arc_err_cstrfmt(c, "Overflow error (this version of Arcueid does not have bignum support)");
  return(CNIL);
#endif
}

value arc_mkrationall(arc *c, long num, long den)
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
  arc_err_cstrfmt(c, "Overflow error (this version of Arcueid does not have bignum support)");
  return(CNIL);
#endif
}

/* Type conversions */
double arc_coerce_flonum(arc *c, value v)
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
    arc_err_cstrfmt(c, "Cannot convert operand of type %d into a flonum", TYPE(v));
    break;
  }
  return(val);
}

void arc_coerce_complex(arc *c, value v, double *re, double *im)
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
    arc_err_cstrfmt(c, "Cannot convert operand of type %d into a flonum", TYPE(v));
    break;
  }
  *im = 0.0;
}

void arc_coerce_bignum(arc *c, value v, void *bptr)
{
#ifdef HAVE_GMP_H
  mpz_t *bignum = (mpz_t *)bptr;

  switch (TYPE(v)) {
  case T_BIGNUM:
    mpz_set(*bignum, REP(v)._bignum);
    break;
  case T_RATIONAL:
    mpz_tdiv_q(*bignum,
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
    arc_err_cstrfmt(c, "Cannot convert operand of type %d into a bignum", TYPE(v));
    break;
  }
#else
  arc_err_cstrfmt(c, "Overflow error (no bignum support)");
#endif
}

void arc_coerce_rational(arc *c, value v, void *bptr)
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
    arc_err_cstrfmt(c, "Cannot convert operand of type %d into a rational", TYPE(v));
    break;
  }
#else
  arc_err_cstrfmt(c, "Overflow error (no bignum support)");
#endif
}

/* Attempt to coerce the value to a fixnum.  If the number is too large
   in magnitude to be converted, return nil. */
value arc_coerce_fixnum(arc *c, value v)
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
    arc_err_cstrfmt(c, "Overflow error (this version of Arcueid does not have bignum support)");
#endif
    break;
  case T_RATIONAL:
#ifdef HAVE_GMP_H
    {
      mpz_t temp;

      mpz_init(temp);
      mpz_fdiv_q(temp,
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
    arc_err_cstrfmt(c, "Overflow error (this version of Arcueid does not have bignum support)");
#endif
    break;
  }
  return(CNIL);
}

/* Coerce a rational back to an integral type where possible.  Otherwise
   return v.  Used by most rational arithmetic operators.  Not needed if
   gmp is disabled. */
#ifdef HAVE_GMP_H
static value integer_coerce(arc *c, value v)
{
  value val;

  /* Attempt to coerce back to a fixnum if possible.  If the denominator
     is 1 in this case, try to convert back. */
  if (mpz_cmp_si(mpq_denref(REP(v)._rational), 1) != 0) {
    /* Not 1, cannot coerce */
    return(v);
  }

  /* It is an integer--try to convert to fixnum or bignum */
  val = arc_coerce_fixnum(c, v);
  if (val != CNIL)
    return(val);

  /* Coerce to bignum */
  val = arc_mkbignuml(c, 0);
  arc_coerce_bignum(c, v, &REP(val)._bignum);
  return(val);
}
#endif

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

static inline value add2_complex(arc *c, value arg1, double re, double im)
{
  re += REP(arg1)._complex.re;
  im += REP(arg1)._complex.im;
  return(arc_mkcomplex(c, re, im));
}

static inline value add2_flonum(arc *c, value arg1, double arg2)
{
  arg2 += REP(arg1)._flonum;
  return(arc_mkflonum(c, arg2));
}

#ifdef HAVE_GMP_H
static inline value add2_rational(arc *c, value arg1, mpq_t *arg2)
{
  mpq_add(*arg2, REP(arg1)._rational, *arg2);
  return(CTRUE);
}

static inline void add2_bignum(arc *c, value arg1, mpz_t *arg2)
{
  mpz_add(*arg2, REP(arg1)._bignum, *arg2);
}
#endif

#define COERCE_OP_COMPLEX(func, arg1, arg2) {	\
    double re, im;				\
    arc_coerce_complex(c, arg2, &re, &im);	\
    return(func(c, arg1, re, im));		\
  }

#define COERCE_OP_FLONUM(func, arg1, arg2) {	\
    double f;					\
    f = arc_coerce_flonum(c, arg2);		\
    return(func(c, arg1, f));			\
  }

#define COERCE_OP_RATIONAL(func, arg1, arg2) {		\
    value v;						\
    v = arc_mkrationall(c, 0, 1);			\
    arc_coerce_rational(c, arg2, &(REP(v)._rational)); \
    if (func(c, arg1, &(REP(v)._rational)) == CNIL)	\
      return(CNIL);					\
    return(integer_coerce(c, v));			\
  }

#define COERCE_OP_BIGNUM(func, arg1, arg2) {			\
    value v, cf;						\
    v = arc_mkbignuml(c, 0);					\
    arc_coerce_bignum(c, arg2, &(REP(v)._bignum));		\
    func(c, arg1, &(REP(v)._bignum));				\
    cf = arc_coerce_fixnum(c, v);				\
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

value __arc_add2(arc *c, value arg1, value arg2)
{
  long fixnum_sum;

  if (TYPE(arg1) == T_FIXNUM && TYPE(arg2) == T_FIXNUM) {
    fixnum_sum = FIX2INT(arg1) + FIX2INT(arg2);
    if (ABS(fixnum_sum) > FIXNUM_MAX)
      return(arc_mkbignuml(c, fixnum_sum));
    return(INT2FIX(fixnum_sum));
  } 

  /* Frankly, I think overloading + in this way is a mistake,
     but it is not my design decision. */
  if ((NIL_P(arg1) || TYPE(arg1) == T_CONS)
      && (NIL_P(arg2) || TYPE(arg2) == T_CONS)) {
    if (arg1 == CNIL)
      return(arg2);
    if (arg2 == CNIL)
      return(arg1);
    return(arc_list_append(arg2, arg1));
  }

  if ((NIL_P(arg1) || TYPE(arg1) == T_STRING)
      && (NIL_P(arg2) || TYPE(arg2) == T_STRING)) {
    if (arg1 == CNIL)
      return(arg2);
    if (arg2 == CNIL)
      return(arg1);
    return(arc_strcat(c, arg2, arg1));
  }

  TYPE_CASES(add, arg1, arg2);

  arc_err_cstrfmt(c, "Invalid types for addition");
  return(CNIL);
}

value __arc_neg(arc *c, value arg)
{
  switch (TYPE(arg)) {
  case T_FIXNUM:
    return(INT2FIX(-FIX2INT(arg)));
  case T_FLONUM:
    return(arc_mkflonum(c, -REP(arg)._flonum));
  case T_COMPLEX:
    return(arc_mkcomplex(c, -REP(arg)._complex.re, -REP(arg)._complex.im));
#ifdef HAVE_GMP_H
  case T_BIGNUM:
    {
      value big;
      big = arc_mkbignuml(c, 0);
      mpz_neg(REP(big)._bignum, REP(arg)._bignum);
      return(big);
    }
    break;
  case T_RATIONAL:
    {
      value rat;
      rat = arc_mkrationall(c, 0, 1);
      mpq_neg(REP(rat)._rational, REP(arg)._rational);
      return(rat);
    }
    break;
#endif
  default:
    arc_err_cstrfmt(c, "Invalid type for negation");
  }
  return(CNIL);
}

static inline value mul2_complex(arc *c, value arg1, double re, double im)
{
  double r2, i2;

  r2 = REP(arg1)._complex.re * re - REP(arg1)._complex.im * im;
  i2 = REP(arg1)._complex.im * re + REP(arg1)._complex.re * im;
  return(arc_mkcomplex(c, r2, i2));
}


static inline value mul2_flonum(arc *c, value arg1, double arg2)
{
  arg2 *= REP(arg1)._flonum;
  return(arc_mkflonum(c, arg2));
}

#ifdef HAVE_GMP_H
static inline value mul2_rational(arc *c, value arg1, mpq_t *arg2)
{
  mpq_mul(*arg2, REP(arg1)._rational, *arg2);
  return(CTRUE);
}

static inline void mul2_bignum(arc *c, value arg1, mpz_t *arg2)
{
  mpz_mul(*arg2, REP(arg1)._bignum, *arg2);
}
#endif

value __arc_mul2(arc *c, value arg1, value arg2)
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
      value v1 = arc_mkbignuml(c, varg1);
      COERCE_OP_BIGNUM(mul2_bignum, v1, arg2);
#else
      arc_err_cstrfmt(c, "Overflow error in multiplication");
      return(CNIL);
#endif
    }
    return(INT2FIX(varg1 * varg2));
  }

  TYPE_CASES(mul, arg1, arg2);

  arc_err_cstrfmt(c, "Invalid types for multiplication");
  return(CNIL);

}

static inline value sub2_complex(arc *c, value arg1, double re, double im)
{
  re = REP(arg1)._complex.re - re;
  im = REP(arg1)._complex.im - im;
  return(arc_mkcomplex(c, re, im));
}

static inline value sub2r_complex(arc *c, value arg1, double re, double im)
{
  re -= REP(arg1)._complex.re;
  im -= REP(arg1)._complex.im;
  return(arc_mkcomplex(c, re, im));
}

static inline value sub2_flonum(arc *c, value arg1, double arg2)
{
  arg2 = REP(arg1)._flonum - arg2;
  return(arc_mkflonum(c, arg2));
}

static inline value sub2r_flonum(arc *c, value arg1, double arg2)
{
  arg2 -= REP(arg1)._flonum;
  return(arc_mkflonum(c, arg2));
}

#ifdef HAVE_GMP_H
static inline value sub2_rational(arc *c, value arg1, mpq_t *arg2)
{
  mpq_sub(*arg2, REP(arg1)._rational, *arg2);
  return(CTRUE);
}

static inline value sub2r_rational(arc *c, value arg1, mpq_t *arg2)
{
  mpq_sub(*arg2, *arg2, REP(arg1)._rational);
  return(CTRUE);
}

static inline void sub2_bignum(arc *c, value arg1, mpz_t *arg2)
{
  mpz_sub(*arg2, REP(arg1)._bignum, *arg2);
}

static inline void sub2r_bignum(arc *c, value arg1, mpz_t *arg2)
{
  mpz_sub(*arg2, *arg2, REP(arg1)._bignum);
}
#endif

value __arc_sub2(arc *c, value arg1, value arg2)
{
  long fixnum_diff;

  if (TYPE(arg1) == T_FIXNUM && TYPE(arg2) == T_FIXNUM) {
    fixnum_diff = FIX2INT(arg1) - FIX2INT(arg2);
    if (ABS(fixnum_diff) > FIXNUM_MAX)
      return(arc_mkbignuml(c, fixnum_diff));
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

  arc_err_cstrfmt(c, "Invalid types for subtraction");
  return(CNIL);
}

static inline value div2_complex(arc *c, value arg1, double re, double im)
{
  double den = (re*re + im*im);
  double r2, i2;

  if (den == 0.0) {
    arc_err_cstrfmt(c, "Division by zero");
    return(CNIL);
  }

  r2 = (REP(arg1)._complex.re * re + REP(arg1)._complex.im * im)/den;
  i2 = (REP(arg1)._complex.im * re - REP(arg1)._complex.re * im)/den;
  return(arc_mkcomplex(c, r2, i2));
}

static inline value div2r_complex(arc *c, value arg1, double re, double im)
{
  double r1 = REP(arg1)._complex.re, i1 = REP(arg1)._complex.im, r2, i2;
  double den = r1*r1 + i1*i1;

  if (den == 0.0) {
    arc_err_cstrfmt(c, "Division by zero");
    return(CNIL);
  }

  r2 = (REP(arg1)._complex.re * re + REP(arg1)._complex.im * im)/den;
  i2 = (im*REP(arg1)._complex.re - re*REP(arg1)._complex.im)/den;
  return(arc_mkcomplex(c, r2, i2));
}

static inline value div2_flonum(arc *c, value arg1, double arg2)
{
  if (arg2 == 0.0) {
    arc_err_cstrfmt(c, "Division by zero");
    return(CNIL);
  }
  arg2 = REP(arg1)._flonum / arg2;
  return(arc_mkflonum(c, arg2));
}

static inline value div2r_flonum(arc *c, value arg1, double arg2)
{
  if (REP(arg1)._flonum == 0.0) {
    arc_err_cstrfmt(c, "Division by zero");
    return(CNIL);
  }

  arg2 /= REP(arg1)._flonum;
  return(arc_mkflonum(c, arg2));
}

#ifdef HAVE_GMP_H
static inline value div2_rational(arc *c, value arg1, mpq_t *arg2)
{
  if (mpq_cmp_si(*arg2, 0, 1) == 0) {
    arc_err_cstrfmt(c, "Division by zero");
    return(CNIL);
  }

  mpq_div(*arg2, REP(arg1)._rational, *arg2);
  return(CTRUE);
}

static inline value div2r_rational(arc *c, value arg1, mpq_t *arg2)
{
  if (mpq_cmp_si(REP(arg1)._rational, 0, 1) == 0) {
    arc_err_cstrfmt(c, "Division by zero");
    return(CNIL);
  }

  mpq_div(*arg2, *arg2, REP(arg1)._rational);
  return(CTRUE);
}

/* Division of two bignums may change the type of the value.  This
   assumes both arg1 and arg2 are bignums initially. This changes
   arg2. */
static inline value div2_bignum(arc *c, value arg1, value arg2)
{
  mpz_t t;

  if (mpz_cmp_si(REP(arg2)._bignum, 0) == 0) {
    arc_err_cstrfmt(c, "Division by zero");
    return(CNIL);
  }

  if (mpz_divisible_p(REP(arg1)._bignum, REP(arg2)._bignum)) {
    mpz_divexact(REP(arg2)._bignum, REP(arg1)._bignum, REP(arg2)._bignum);
    return(CTRUE);
  }
  mpz_init(t);
  mpz_set(t, REP(arg2)._bignum);
  mpz_clear(REP(arg2)._bignum);
  mpq_init(REP(arg2)._rational);
  BTYPE(arg2) = T_RATIONAL;
  mpq_set_num(REP(arg2)._rational, REP(arg1)._bignum);
  mpq_set_den(REP(arg2)._rational, t);
  mpz_clear(t);
  return(CTRUE);
}

/* This divides the two values in reverse, storing the result in arg2 */
static inline value div2r_bignum(arc *c, value arg1, value arg2)
{
  mpz_t t;

  if (mpz_cmp_si(REP(arg1)._bignum, 0) == 0) {
    arc_err_cstrfmt(c, "Division by zero");
    return(CNIL);
  }

  if (mpz_divisible_p(REP(arg2)._bignum, REP(arg1)._bignum)) {
    mpz_divexact(REP(arg2)._bignum, REP(arg2)._bignum, REP(arg1)._bignum);
    return(CTRUE);
  }
  mpz_init(t);
  mpz_set(t, REP(arg2)._bignum);
  mpz_clear(REP(arg2)._bignum);
  mpq_init(REP(arg2)._rational);
  BTYPE(arg2) = T_RATIONAL;
  mpq_set_num(REP(arg2)._rational, t);
  mpq_set_den(REP(arg2)._rational, REP(arg1)._bignum);
  mpz_clear(t);
  return(CTRUE);
}

#endif

value __arc_div2(arc *c, value arg1, value arg2)
{
  ldiv_t res;

  if (TYPE(arg1) == T_FIXNUM && TYPE(arg2) == T_FIXNUM) {
    long varg1, varg2;

    varg1 = FIX2INT(arg1);
    varg2 = FIX2INT(arg2);

    if (varg2 == 0) {
      arc_err_cstrfmt(c, "Division by zero");
      return(CNIL);
    }

    res = ldiv(varg1, varg2);
#ifdef HAVE_GMP_H
    if (res.rem != 0) {
      return(arc_mkrationall(c, varg1, varg2));
    } else {
      if (ABS(res.quot) > FIXNUM_MAX)
	return(arc_mkbignuml(c, res.quot));
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

    v = arc_mkbignuml(c, 0);
    arc_coerce_bignum(c, arg2, &REP(v)._bignum);
    if (div2_bignum(c, arg1, v) == CNIL)
      return(CNIL);
    if (TYPE(v) == T_BIGNUM) {
      cf = arc_coerce_fixnum(c, v);
      return((cf == CNIL) ? v : cf);
    }
    return(v);
  } else if (TYPE(arg2) == T_BIGNUM) {
    value cf, v;

    v = arc_mkbignuml(c, 0);
    arc_coerce_bignum(c, arg1, &REP(v)._bignum);
    if (div2r_bignum(c, arg2, v) == CNIL)
      return(CNIL);
    if (TYPE(v) == T_BIGNUM) {
      cf = arc_coerce_fixnum(c, v);
      return((cf == CNIL) ? v : cf);
    }
    return(v);
  }
#endif

  arc_err_cstrfmt(c, "Invalid types for division");
  return(CNIL);
}

value __arc_mod2(arc *c, value arg1, value arg2)
{
  ldiv_t res;

  if (TYPE(arg2) == T_FIXNUM && FIX2INT(arg2) == 0) {
    arc_err_cstrfmt(c, "Division by zero");
    return(CNIL);
  }

  if (TYPE(arg1) == T_FIXNUM && TYPE(arg2) == T_FIXNUM) {
    long varg1, varg2;

    varg1 = FIX2INT(arg1);
    varg2 = FIX2INT(arg2);
    res = ldiv(varg1, varg2);
    /* Corrections to make the modulo Euclidean */
    if ((varg1 < 0 && varg2 > 0) || (varg1 > 0 && varg2 < 0))
      res = ldiv(res.rem + varg2, varg2);
    return(INT2FIX(res.rem));
  } 

#ifdef HAVE_GMP_H
  else if ((TYPE(arg1) == T_BIGNUM && TYPE(arg2) == T_FIXNUM)
	   || (TYPE(arg1) == T_FIXNUM && TYPE(arg2) == T_BIGNUM)
	   || (TYPE(arg1) == T_BIGNUM && TYPE(arg2) == T_BIGNUM)) {
    mpz_t barg1, barg2;
    value res, r2;

    mpz_init(barg1);
    mpz_init(barg2);
    arc_coerce_bignum(c, arg1, &barg1);
    arc_coerce_bignum(c, arg2, &barg2);
    res = arc_mkbignuml(c, 0);
    mpz_fdiv_r(REP(res)._bignum, barg1, barg2);
    mpz_clear(barg1);
    mpz_clear(barg2);
    /* force to fixnum where possible */
    r2 = arc_coerce_fixnum(c, res);
    if (r2 == CNIL)
      r2 = res;
    return(r2);
  }
#endif

  arc_err_cstrfmt(c, "Invalid types for modulus");
  return(CNIL);
}

value __arc_idiv2(arc *c, value arg1, value arg2)
{
  ldiv_t res;

  if (TYPE(arg2) == T_FIXNUM && FIX2INT(arg2) == 0) {
    arc_err_cstrfmt(c, "Division by zero");
    return(CNIL);
  }

  if (TYPE(arg1) == T_FIXNUM && TYPE(arg2) == T_FIXNUM) {
    long varg1, varg2;

    varg1 = FIX2INT(arg1);
    varg2 = FIX2INT(arg2);

    res = ldiv(varg1, varg2);
    return(INT2FIX(res.quot));
  } 

#ifdef HAVE_GMP_H
  else if ((TYPE(arg1) == T_BIGNUM && TYPE(arg2) == T_FIXNUM)
	   || (TYPE(arg1) == T_FIXNUM && TYPE(arg2) == T_BIGNUM)
	   || (TYPE(arg1) == T_BIGNUM && TYPE(arg2) == T_BIGNUM)) {
    mpz_t barg1, barg2;
    value res, r2;

    mpz_init(barg1);
    mpz_init(barg2);
    arc_coerce_bignum(c, arg1, &barg1);
    arc_coerce_bignum(c, arg2, &barg2);
    res = arc_mkbignuml(c, 0);
    mpz_fdiv_q(REP(res)._bignum, barg1, barg2);
    mpz_clear(barg1);
    mpz_clear(barg2);
    /* force to fixnum where possible */
    r2 = arc_coerce_fixnum(c, res);
    if (r2 == CNIL)
      r2 = res;
    return(r2);
  }
#endif

  arc_err_cstrfmt(c, "Invalid types for modulus");
  return(CNIL);
}

/* Multiplies arg1 by 2^n and adds it to acc.  This is generally used
   for CIEL.  Note that acc is modified if it is a bignum! */
value __arc_amul_2exp(arc *c, value acc, value arg1, int n)
{
#ifdef HAVE_GMP_H
  mpz_t val;
#endif

  if (TYPE(arg1) == T_FIXNUM) {
    /* First, try to use an ordinary bit shift on the fixnum. */
    if (n <= (sizeof(long long) - 1) * 8) {
      long long res = ((long long)FIX2INT(arg1)) << n;

      if (res >= FIX2INT(arg1))
	return(__arc_add2(c, acc, INT2FIX(res)));
    }
    /* Otherwise, promote arg1 to bignum */
#ifdef HAVE_GMP_H
    mpz_init_set_si(val, FIX2INT(arg1));
  } else if (TYPE(arg1) == T_BIGNUM) {
    mpz_init_set(val, REP(arg1)._bignum);
#endif
  } else {
    arc_err_cstrfmt(c, "Invalid types for amul2exp");
  }
#ifdef HAVE_GMP_H
  if (TYPE(acc) == T_FIXNUM) {
    value v;

    /* force acc to bignum if it isn't already */
    v = arc_mkbignuml(c, 0);
    arc_coerce_bignum(c, acc, &(REP(v)._bignum));
    acc = v;
  }
  /* Bignum case -- val contains the value */
  mpz_mul_2exp(val, val, n);
  mpz_add(REP(acc)._bignum, REP(acc)._bignum, val);
  mpz_clear(val);
  return(acc);
#else
  arc_err_cstrfmt(c, "Overflow error (this version of Arcueid does not have bignum support)");
  return(CNIL);
#endif

}

value __arc_add(arc *c, int argc, value *argv)
{
  value sum = INT2FIX(0);
  int i;

  if (argc > 0 && (NIL_P(argv[0]) || CONS_P(argv[0]) || TYPE(argv[0]) == T_STRING))
      sum = CNIL;

  for (i=0; i<argc; i++)
    sum = __arc_add2(c, argv[i], sum);
  return(sum);
}

value __arc_sub(arc *c, int argc, value *argv)
{
  value diff = INT2FIX(0);
  int i = 0;

  if (argc < 1) {
    arc_err_cstrfmt(c, "-: expects at least 1 argument, given %d", argc);
    return(CNIL);
  }
  if (argc > 1) {
    diff = argv[0];
    i = 1;
  }

  for (; i<argc; i++)
    diff = __arc_sub2(c, diff, argv[i]);
  return(diff);
}

value __arc_mul(arc *c, int argc, value *argv)
{
  value prod = INT2FIX(1);
  int i;

  for (i=0; i<argc; i++)
    prod = __arc_mul2(c, prod, argv[i]);
  return(prod);
}

value __arc_div(arc *c, int argc, value *argv)
{
  value quot = INT2FIX(1);
  int i = 0;

  if (argc < 1) {
    arc_err_cstrfmt(c, "/: expects at least 1 argument, given %d", argc);
    return(CNIL);
  }
  if (argc > 1) {
    quot = argv[0];
    i = 1;
  }

  for (; i<argc; i++)
    quot = __arc_div2(c, quot, argv[i]);
  return(quot);
}

static value rune2dig(Rune r, int radix)
{
  Rune rl;
  value v;

  if (!ucisalnum(r))
    return(CNIL);
  rl = tolower(r);
  if (rl >= 0x30 && rl <= 0x39)
    v = rl - 0x30;
  else if (rl >= 0x61 && rl <= 0x7a)
    v = (rl - 0x61) + 10;
  if (v > radix)
    return(CNIL);
  return(INT2FIX(v));
}

static double str2flonum(arc *c, value str, int index, int imagflag)
{
  int state = 1, expn = 0, expnsign = 1;
  double sign = 1.0, mantissa=0.0, mult=0.1, fnum;
  value digitval, imag;
  Rune ch;

  while ((ch = arc_strgetc(c, str, &index)) != Runeerror) {
    switch (state) {
    case 1:
      /* sign */
      switch (ch) {
      case '-':
	sign = -1;
	state = 2;
	break;
      case '+':
	sign = 1;
	state = 2;
	break;
      default:
	if (!isdigit(ch))
	  return(CNIL);
	arc_strungetc(c, &index);
	state = 2;
	break;
      }
      break;
    case 2:
      /* non-fractional part of mantissa */
      switch (ch) {
      case '.':
	state = 3;
	break;
      case 'e':
      case 'E':
	state = 4;
	break;
      case '+':
      case '-':
	/* Complex */
	if (imagflag) {
	  return(CNIL);
	} else {
	  /* read the imaginary part */
	  imag = str2flonum(c, str, index-1, 1);
	  if (TYPE(imag) != T_COMPLEX)
	    return(CNIL);
	  REP(imag)._complex.re = sign * (mantissa * pow(10, expnsign * expn));
	  return(imag);
	}
	break;
      case 'i':
      case 'I':
      case 'j':
      case 'J':
	/* imaginary */
	return(arc_mkcomplex(c, 0.0,
			      sign * (mantissa
				      * pow(10, expnsign * expn))));
      default:
	if (!isdigit(ch))
	  return(CNIL);
	mantissa = (mantissa * 10) + FIX2INT(rune2dig(ch, 10));
	break;
      }
      break;
    case 3:
      /* fractional part of mantissa */
      switch (ch) {
      case 'e':
      case 'E':
	state = 4;
	break;
      case '+':
      case '-':
	/* Complex */
	if (imagflag) {
	  return(CNIL);
	} else {
	  /* read the imaginary part */
	  imag = str2flonum(c, str, index-1, 1);
	  if (TYPE(imag) != T_COMPLEX)
	    return(CNIL);
	  REP(imag)._complex.re = sign * (mantissa * pow(10, expnsign * expn));
	  return(imag);
	}
	break;
      case 'i':
      case 'I':
      case 'j':
      case 'J':
	/* imaginary */
	return(arc_mkcomplex(c, 0.0,
			      sign * (mantissa
				      * pow(10, expnsign * expn))));
      default:
	if (!isdigit(ch))
	  return(CNIL);
	mantissa += FIX2INT(rune2dig(ch, 10)) * mult;
	mult *= 0.1;
	break;
      }
      break;
    case 4:
      /* exponent sign */
      switch (ch) {
      case '-':
	expnsign = -1;
	state = 5;
	break;
      case '+':
	expnsign = 1;
	state = 5;
	break;
      default:
	if (!isdigit(ch))
	  return(CNIL);
	arc_strungetc(c, &index);
	state = 5;
	break;
      }
      break;
    case 5:
      switch (ch) {
      case '+':
      case '-':
	/* Complex */
	if (imagflag) {
	  return(CNIL);
	} else {
	  /* read the imaginary part */
	  imag = str2flonum(c, str, index-1, 1);
	  if (TYPE(imag) != T_COMPLEX)
	    return(CNIL);
	  REP(imag)._complex.re = sign * (mantissa * pow(10, expnsign * expn));
	  return(imag);
	}
	break;
      case 'i':
      case 'I':
      case 'j':
      case 'J':
	/* imaginary */
	return(arc_mkcomplex(c, 0.0,
			      sign * (mantissa
				      * pow(10, expnsign * expn))));
      default:
	/* exponent magnitude */
	digitval = rune2dig(ch, 10);
	if (digitval == CNIL)
	  return(CNIL);
	expn = expn * 10 + FIX2INT(digitval);
	break;
      }
      break;
    }
  }
  /* Combine */
  fnum = sign * (mantissa * pow(10, expnsign * expn));
  return(arc_mkflonum(c, fnum));
}

static value string2numindex(arc *c, value str, int index, int rational)
{
  int state = 1, sign=1, radsel = 0;
  Rune ch;
  value nval = INT2FIX(0), digitval, radix = INT2FIX(10), denom;

  while ((ch = arc_strgetc(c, str, &index)) != Runeerror) {
    switch (state) {
    case 1:
      /* sign */
      switch (ch) {
      case '-':
	sign = -1;
	state = 2;
	break;
      case '+':
	sign = 1;
	state = 2;
	break;
      case '.':
	return(str2flonum(c, str, 0, 0));
	break;
      default:
	if (!isdigit(ch))
	  return(CNIL);
	arc_strungetc(c, &index);
	state = 2;
	break;
      }
      break;
    case 2:
      /* digits, or possible radix */
      switch (ch) {
      case '0':
	radix = INT2FIX(8);
	state = 3;
	break;
      case '.':
	return(str2flonum(c, str, 0, 0));
	break;
      default:
	if (!isdigit(ch))
	  return(CNIL);
	/* more digits */
	arc_strungetc(c, &index);
	state = 4;
	break;
      }
      break;
    case 3:
      /* digits, or possible radix */
      switch (ch) {
      case 'x':
	radix = INT2FIX(16);
	state = 4;
	break;
      case 'b':
	radix = INT2FIX(2);
	state = 4;
	break;
      case '.':
	return(str2flonum(c, str, 0, 0));
	break;
      default:
	if (!isdigit(ch))
	  return(CNIL);
	/* more digits */
	arc_strungetc(c, &index);
	state = 4;
	break;
      }
      break;
    case 4:
      /* digits */
      switch (ch) {
      case '.':
      case 'e':
      case 'E':
	return(str2flonum(c, str, 0, 0));
	break;
      case 'r':
	/* Limbo-style radix selector: the base radix should
	   still be 10, and the value should be between 2 and 36 */
	if (radix == INT2FIX(10) && TYPE(nval) == T_FIXNUM
	    && (FIX2INT(nval) >= 2 && FIX2INT(nval) <= 36)) {
	  radix = nval;
	  nval = INT2FIX(0);
	  state = 5;
	} else if (!radsel) {
	  return(CNIL);		/* invalid radix selector */
	}
	break;
      case '/':
	if (rational)
	  return(CNIL);
	denom = string2numindex(c, str, index, 1);
	if (TYPE(denom) == T_FIXNUM || TYPE(denom) == T_FLONUM)
	  return(__arc_div2(c, nval, denom));
	else
	  return(CNIL);
	break;
      case 'i':
      case 'I':
      case 'j':
      case 'J':
	return(str2flonum(c, str, 0, 0));
      case '+':
      case '-':
	return(str2flonum(c, str, 0, 0));
      default:
	/* Digits */
	digitval = rune2dig(ch, radix);
	if (digitval == CNIL)
	  return(CNIL);
	nval = __arc_add2(c, __arc_mul2(c, nval, radix), digitval);
	break;
      }
      break;
    case 5:
      /* Digits */
      digitval = rune2dig(ch, radix);
      if (digitval == CNIL)
	return(CNIL);
      nval = __arc_add2(c, __arc_mul2(c, nval, radix), digitval);
      break;
    }
  }
  /* For nval to be a valid number, we must have entered at least state 3.
     If we have not, the number is not valid. */
  if (state >= 3) {
    nval = __arc_mul2(c, nval, INT2FIX(sign));
    return(nval);
  }
  return(CNIL);
}

value arc_string2num(arc *c, value str)
{
  return(string2numindex(c, str, 0, 0));
}

#define COMPARE(x) {				\
  if ((x) > 0)					\
    return(INT2FIX(1));				\
  else if ((x) < 0)				\
    return(INT2FIX(-1));			\
  else						\
    return(INT2FIX(0));				\
  }

/* May be faster if implemented by direct comparison, but subtraction
   is a lot easier! :p */
value arc_numcmp(arc *c, value v1, value v2)
{
  value diff;

  diff = __arc_sub2(c, v1, v2);
  switch (TYPE(diff)) {
  case T_FIXNUM:
    COMPARE(FIX2INT(diff));
    break;
  case T_FLONUM:
    COMPARE(REP(diff)._flonum);
    break;
#ifdef HAVE_GMP_H
  case T_BIGNUM:
    return(INT2FIX(mpz_sgn(REP(diff)._bignum)));
    break;
  case T_RATIONAL:
    return(INT2FIX(mpq_sgn(REP(diff)._rational)));
    break;
#endif
  default:
    arc_err_cstrfmt(c, "Invalid types for numeric comparison");
    return(CNIL);
  }
}

value arc_exact(arc *c, value v)
{
  return((FIXNUM_P(v) || TYPE(v) == T_BIGNUM || TYPE(v) == T_RATIONAL) ? CTRUE : CNIL);
}

value __arc_abs(arc *c, value v)
{
#ifdef HAVE_GMP_H
  value val;
#endif

  switch (TYPE(v)) {
  case T_FIXNUM:
    return(INT2FIX(ABS(FIX2INT(v))));
  case T_FLONUM:
    return(arc_mkflonum(c, ABS(REP(v)._flonum)));
  case T_COMPLEX:
    {
      double complex v2;

      v2 = REP(v)._complex.re + I*REP(v)._complex.im;
      return(arc_mkflonum(c, cabs(v2)));
    }
#ifdef HAVE_GMP_H
  case T_BIGNUM:
    val = arc_mkbignuml(c, 0);
    mpz_abs(REP(val)._bignum, REP(v)._bignum);
    return(val);
  case T_RATIONAL:
    val = arc_mkrationall(c, 0, 1);
    mpq_abs(REP(val)._rational, REP(v)._rational);
    return(val);
#endif
  }
    arc_err_cstrfmt(c, "Invalid types for absolute value");
  return(CNIL);
}

value arc_expt(arc *c, value a, value b)
{
  double complex ac, bc, p;
  double re, im;
#ifdef HAVE_GMP_H
  mpz_t anumer, adenom;
  unsigned long int babs;
  value result, cresult;
#endif

  if (!((TYPE(a) == T_FIXNUM || TYPE(a) == T_FLONUM
	 || TYPE(a) == T_BIGNUM || TYPE(a) == T_RATIONAL
	 || TYPE(a) == T_COMPLEX)
	&& (TYPE(b) == T_FIXNUM || TYPE(b) == T_FLONUM
	    || TYPE(b) == T_BIGNUM || TYPE(a) == T_RATIONAL
	    || TYPE(a) == T_COMPLEX))) {
    arc_err_cstrfmt(c, "Invalid types for exponentiation");
    return(CNIL);
  }
  /* We coerce everything to complex and use cpow if any argument
     is a flonum or a complex, or if the exponent is rational.
     Note that if bignum support is unavailable, this
     method is still used even for fixnum arguments. */
#ifdef HAVE_GMP_H
  if (TYPE(a) == T_FLONUM || TYPE(b) == T_FLONUM
      || TYPE(a) == T_COMPLEX || TYPE(b) == T_COMPLEX
      || TYPE(b) == T_RATIONAL) {
#endif
    /* Use complex arithmetic here, convert back to real if
       Im(p) == 0.0.  Note that complex exponents are NOT
       supported by PG-ARC! */
    arc_coerce_complex(c, a, &re, &im);
    ac = re + I*im;
    arc_coerce_complex(c, b, &re, &im);
    bc = re + I*im;
    p = cpow(ac, bc);
    if (cimag(p) < DBL_EPSILON)
      return(arc_mkflonum(c, creal(p)));
    return(arc_mkcomplex(c, creal(p), cimag(p)));
#ifdef HAVE_GMP_H
  }
#endif
#ifdef HAVE_GMP_H
  /* We can't handle a bignum exponent... */
  if (TYPE(b) == T_BIGNUM) {
    arc_err_cstrfmt(c, "Exponent too large");
    return(arc_mkflonum(c, INFINITY));
  }
  /* Special case, zero base */
  if (a == INT2FIX(0))
    return(INT2FIX(0));

  /* Special case, zero exponent */
  if (b == INT2FIX(0))
    return(INT2FIX(1));
  /* The only unhandled case is a fixnum, bignum, or rational base with
     a fixnum exponent.  */
  mpz_init(anumer);
  mpz_init(adenom);
  switch (TYPE(a)) {
  case T_FIXNUM:
    mpz_set_si(anumer, FIX2INT(a));
    mpz_set_si(adenom, 1);
    break;
  case T_BIGNUM:
    mpz_set(anumer, REP(a)._bignum);
    mpz_set_si(adenom, 1);
    break;
  case T_RATIONAL:
    mpq_get_num(anumer, REP(a)._rational);
    mpq_get_den(adenom, REP(a)._rational);
    break;
  default:
    mpz_clear(anumer);
    mpz_clear(adenom);
    arc_err_cstrfmt(c, "Invalid types for exponentiation");
    return(CNIL);
    break;
  }
  babs = abs(FIX2INT(b));
  mpz_pow_ui(anumer, anumer, babs);
  mpz_pow_ui(adenom, adenom, babs);
  /* if the original value was negative, take the reciprocal */
  if (FIX2INT(b) < 0)
    mpz_swap(anumer, adenom);
  if (mpz_cmp_si(adenom, 1) == 0) {
    result = arc_mkbignuml(c, 0);
    mpz_set(REP(result)._bignum, anumer);
    cresult = arc_coerce_fixnum(c, result);
    if (cresult != CNIL)
      result = cresult;
  } else {
    result = arc_mkrationall(c, 0, 1);
    mpq_set_num(REP(result)._rational, anumer);
    mpq_set_den(REP(result)._rational, adenom);
    mpq_canonicalize(REP(result)._rational);
  }
  mpz_clear(anumer);
  mpz_clear(adenom);
  return(result);
#endif
}

static const char _itoa_lower_digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
static const char _itoa_upper_digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

value __arc_itoa(arc *c, value onum, value base,
			int uc, int sf)
{
  const char *digits = (uc) ? _itoa_upper_digits : _itoa_lower_digits;
  value dig, num;
  value str = arc_mkstringc(c, "");
  int p, q, sign;
  Rune t;

  num = __arc_abs(c, onum);
  sign = FIX2INT(arc_numcmp(c, onum, INT2FIX(0)));
  while (arc_numcmp(c, num, INT2FIX(0)) > INT2FIX(0)) {
    dig = __arc_mod2(c, num, base);
    str = arc_strcatc(c, str, (Rune)digits[FIX2INT(dig)]);
    num = __arc_idiv2(c, num, base);
  }
  /* sign goes at the end if it's present, since the digits come in
     reversed. */
  if (sign < 0)
    str = arc_strcatc(c, str, (Rune)'-');
  else if (sf)
    str = arc_strcatc(c, str, (Rune)'+');
  /* Reverse the string */
  q = arc_strlen(c, str);
  p = 0;
  for (--q; p < q; ++p, --q) {
    t = arc_strindex(c, str, p);
    arc_strsetindex(c, str, p, arc_strindex(c, str, q));
    arc_strsetindex(c, str, q, t);
  }
  return(str);
}


/* Our internal random number generator uses Bob Jenkins' 64-bit ISAAC
   random number generator. */
#define RANDSIZL (8)
#define RANDSIZ (1 << RANDSIZL)

static uint64_t randrsl[RANDSIZ], randcnt;
static uint64_t mm[RANDSIZ], aa=0LL, bb=0LL, cc=0LL;

#define IND(mm,x) (*(uint64_t *)((unsigned char *)(mm) + ((x) & ((RANDSIZ-1) << 3))))
#define RNGSTEP(mix,a,b,mm,m,m2,r,x) \
{ \
  x = *m;  \
  a = (mix) + *(m2++); \
  *(m++) = y = IND(mm,x) + a + b; \
  *(r++) = b = IND(mm,y>>RANDSIZL) + x; \
}

static void isaac64(void)
{
  uint64_t a,b,x,y,*m,*m2,*r,*mend;

  m=mm; r=randrsl;
  a = aa;
  b = bb + (++cc);
  for (m = mm, mend = m2 = m+(RANDSIZ/2); m<mend; ) {
    RNGSTEP(~(a^(a<<21)), a, b, mm, m, m2, r, x);
    RNGSTEP(  a^(a>>5)  , a, b, mm, m, m2, r, x);
    RNGSTEP(  a^(a<<12) , a, b, mm, m, m2, r, x);
    RNGSTEP(  a^(a>>33) , a, b, mm, m, m2, r, x);
  }
  for (m2 = mm; m2<mend; )
  {
    RNGSTEP(~(a^(a<<21)), a, b, mm, m, m2, r, x);
    RNGSTEP(  a^(a>>5)  , a, b, mm, m, m2, r, x);
    RNGSTEP(  a^(a<<12) , a, b, mm, m, m2, r, x);
    RNGSTEP(  a^(a>>33) , a, b, mm, m, m2, r, x);
  }
  bb = b; aa = a;
}

#define MIX(a,b,c,d,e,f,g,h) \
{ \
   a-=e; f^=h>>9;  h+=a; \
   b-=f; g^=a<<9;  a+=b; \
   c-=g; h^=b>>23; b+=c; \
   d-=h; a^=c<<15; c+=d; \
   e-=a; b^=d>>14; d+=e; \
   f-=b; c^=e<<20; e+=f; \
   g-=c; d^=f>>17; f+=g; \
   h-=d; e^=g<<14; g+=h; \
}

value arc_srand(arc *ccc, value seed)
{
  int i;
  uint64_t a,b,c,d,e,f,g,h;

  if (TYPE(seed) != T_FIXNUM) {
    arc_err_cstrfmt(ccc, "srand requires first argument be a fixnum, given object of type %d", TYPE(seed));
    return(CNIL);
  }

  for (i=0; i<RANDSIZ; ++i)
    mm[i]=(uint64_t)0LL;

  aa=bb=cc=(uint64_t)0LL;
  a=b=c=d=e=f=g=h=0x9e3779b97f4a7c13LL;  /* the golden ratio */
  randrsl[0] = (uint64_t)FIX2INT(seed);

  for (i=0; i<RANDSIZ; i+=8) {
    a+=randrsl[i];
    b+=randrsl[i+1];
    c+=randrsl[i+2];
    d+=randrsl[i+3];
    e+=randrsl[i+4];
    f+=randrsl[i+5];
    g+=randrsl[i+6];
    h+=randrsl[i+7];
    MIX(a,b,c,d,e,f,g,h);
    mm[i]=a;
    mm[i+1]=b;
    mm[i+2]=c;
    mm[i+3]=d;
    mm[i+4]=e;
    mm[i+5]=f;
    mm[i+6]=g;
    mm[i+7]=h;
  }

  for (i=0; i<RANDSIZ; i+=8) {
    a+=mm[i];
    b+=mm[i+1];
    c+=mm[i+2];
    d+=mm[i+3];
    e+=mm[i+4];
    f+=mm[i+5];
    g+=mm[i+6];
    h+=mm[i+7];
    MIX(a,b,c,d,e,f,g,h);
    mm[i]=a;
    mm[i+1]=b;
    mm[i+2]=c;
    mm[i+3]=d;
    mm[i+4]=e;
    mm[i+5]=f;
    mm[i+6]=g;
    mm[i+7]=h;
  }
  isaac64();
  randcnt=RANDSIZ;
  return(seed);
}

#define RAND() \
   (!randcnt-- ? (isaac64(), randcnt=RANDSIZ-1, randrsl[randcnt]) : \
                 randrsl[randcnt])
value arc_rand(arc *c, int argc, value *argv)
{
  uint64_t rnd1, rnd2, max;

  if (argc > 1) {
    arc_err_cstrfmt(c, "rand expects 0 or 1 argument, given %d", argc);
    return(CNIL);
  }

  if (argc == 0) {
    rnd1 = RAND() & 0x1fffffffffffffLL;
    rnd2 = RAND() & 0x1fffffffffffffLL;
    return(arc_mkflonum(c, (double)rnd1 / (double)rnd2));
  }

  if (TYPE(argv[0]) != T_FIXNUM) {
    arc_err_cstrfmt(c, "rand expects fixnum argument, given type %d", TYPE(argv[0]));
    return(CNIL);
  }
  max = FIX2INT(argv[0]);
  /* XXX - We should have a better algorithm than this, but this should do
     just fine for now.  This simple method introduces a slight bias into
     the random number. */
  return(INT2FIX(RAND() % max));
}

value arc_fixnump(arc *c, value v)
{
  return((FIXNUM_P(v)) ? CTRUE : CNIL);
}

value arc_trunc(arc *c, value v)
{
  value result = CNIL;

  switch (TYPE(v)) {
  case T_FIXNUM:
  case T_BIGNUM:
    result = v;
    break;
  case T_FLONUM:
    /* try to coerce to fixnum first */
    result = arc_coerce_fixnum(c, v);
    if (result == CNIL) {
#ifdef HAVE_GMP_H
      result = arc_mkbignuml(c, 0);
      arc_coerce_bignum(c, v, &REP(result)._bignum);
#else
      arc_err_cstrfmt(c, "flonum->fixnum conversion overflow (this version of Arcueid does not have bignum support)");
#endif
    }
    break;
#ifdef HAVE_GMP_H
  case T_RATIONAL:
    result = arc_mkbignuml(c, 0);
    arc_coerce_bignum(c, v, &REP(result)._bignum);
    break;
#endif
  case T_COMPLEX:
    arc_err_cstrfmt(c, "cannot truncate complex number to integer");
    break;
  default:
    arc_err_cstrfmt(c, "truncate expects numeric type, given object of type %d", TYPE(v));
    break;
  }
  return(result);
}
