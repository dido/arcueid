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

#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <complex.h>
#include <string.h>
#include <ctype.h>
#include "arcueid.h"
#include "arith.h"
#include "../config.h"
#include "utf.h"

#ifdef HAVE_GMP_H
#include <gmp.h>
#endif

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
#ifndef alloca
# define alloca __builtin_alloca
#endif
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# include <stddef.h>
void *alloca (size_t);
#endif

#ifdef HAVE_GMP_H
static value bignum_fixnum(arc *c, value n);
static value div_rational(arc *c, value v1, value v2);
#endif

static value (*coercefn(arc *c, value val))(arc *, value, enum arc_types);

/*================================= Fixnums */

static const char _itoa_lower_digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
static const char _itoa_upper_digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

value __arc_itoa(arc *c, value onum, value base,
			int uc, int sf)
{
  const char *digits = (uc) ? _itoa_upper_digits : _itoa_lower_digits;
  value dig, num;
  value str;
  long int p, q, sign;
  Rune t;

  sign = 1;
  sign = FIX2INT(onum);
  sign = (sign > 0) ? 1 : ((sign < 0) ? -1 : 0);
  num = INT2FIX(sign * FIX2INT(onum));
  /* special case for 0 */
  if (sign == 0) {
    return(arc_mkstringc(c, "0"));
  } else {
    str = arc_mkstringc(c, "");
    while (FIX2INT(num) > 0) {
      dig = __arc_mod2(c, num, base);
      str = arc_strcatc(c, str, (Rune)digits[FIX2INT(dig)]);
      num = __arc_idiv2(c, num, base);
    }
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

static value fixnum_coerce(arc *c, value x, enum arc_types t)
{
  switch(t) {
  case T_FIXNUM:
    return(x);
  case T_COMPLEX:
    return(arc_mkcomplex(c, (double)FIX2INT(x) + I*0.0));
  case T_FLONUM:
    return(arc_mkflonum(c, (double)FIX2INT(x)));
#ifdef HAVE_GMP_H
  case T_BIGNUM:
    return(arc_mkbignuml(c, FIX2INT(x)));
  case T_RATIONAL:
    return(arc_mkrationall(c, FIX2INT(x), 1));
#endif
  default:
    break;
  }
  return(CNIL);
}

static AFFDEF(fixnum_xcoerce)
{
  AARG(obj, stype, arg);
  AFBEGIN;

  /* trivial cases */
  if (FIX2INT(AV(stype)) == T_FIXNUM || FIX2INT(AV(stype)) == T_BIGNUM
      || FIX2INT(AV(stype)) == T_RATIONAL || FIX2INT(AV(stype)) == T_NUM
      || FIX2INT(AV(stype)) == T_INT)
    ARETURN(AV(obj));
  if (FIX2INT(AV(stype)) == T_FLONUM || FIX2INT(AV(stype)) == T_COMPLEX)
    ARETURN(arc_mkflonum(c, (double)FIX2INT(AV(obj))));
  if (FIX2INT(AV(stype)) == T_CHAR) {
    Rune ch;
    /* Check for invalid ranges */
    ch = FIX2INT(AV(obj));
    if (ch < 0 || ch > 0x10FFFF || (ch >= 0xd800 && ch <= 0xdfff)) {
      arc_err_cstrfmt(c, "integer->char: expects argument of type <exact integer in [0,#x10FFFF], not in [#xD800,#xDFFF]>");
      ARETURN(CNIL);
    }
    ARETURN(arc_mkchar(c, ch));
  }

  if (!BOUND_P(AV(arg)))
    AV(arg) = INT2FIX(10);

  if (FIX2INT(AV(stype)) == T_STRING) {
    ARETURN(__arc_itoa(c, AV(obj), AV(arg), 0, 0));
  }

  arc_err_cstrfmt(c, "cannot coerce");
  ARETURN(CNIL);
  AFEND;
}
AFFEND

/*================================= Flonums */

static AFFDEF(flonum_pprint)
{
  AARG(f);
  AFBEGIN;
  double val = *((double *)REP(AV(f)));
  int len;
  char *outstr;

  len = snprintf(NULL, 0, "%g", val);
  outstr = (char *)alloca(sizeof(char)*(len+2));
  snprintf(outstr, len+1, "%g", val);
  ARETURN(arc_mkstringc(c, outstr));
  AFEND;
}
AFFEND

static unsigned long flonum_hash(arc *c, value f, arc_hs *s)
{
  char *ptr = (char *)REP(f);
  int i;

  for (i=0; i<sizeof(double)/sizeof(char); i++)
    arc_hash_update(s, *ptr++);
  return(1);
}

static value flonum_iscmp(arc *c, value v1, value v2)
{
  return((*((double *)REP(v1)) == *((double *)REP(v2))) ? CTRUE : CNIL);
}

static value flonum_coerce(arc *c, value v, enum arc_types t)
{
  switch(t) {
  case T_FIXNUM:
    if (ABS(REPFLO(v)) > FIXNUM_MAX)
      return(CNIL);
    return(INT2FIX((int)REPFLO(v)));
  case T_COMPLEX:
    return(arc_mkcomplex(c, REPFLO(v) + I*0.0));
  case T_FLONUM:
    /* trivial case */
    return(v);
#ifdef HAVE_GMP_H
  case T_BIGNUM: {
    double base, drem;
    value bignum = arc_mkbignuml(c, 0L);

    if (!isfinite(REPFLO(v)) || isnan(REPFLO(v)))
      return(CNIL);
    /* see if we need to round */
    base = floor(REPFLO(v));
    drem = REPFLO(v) - base;
    mpz_set_d(REPBNUM(bignum), REPFLO(v));
    if (drem > 0.5 || (drem == 0.5 && (((int)base)&0x1) == 1))
      mpz_add_ui(REPBNUM(bignum), REPBNUM(bignum), 1);
    return(bignum);
  }
  case T_RATIONAL: {
    value rat;

    rat = arc_mkrationall(c, 0, 1);
    mpq_set_d(REPRAT(rat), REPFLO(v));
    return(rat);
  }
#endif
  default:
    break;
  }
  return(CNIL);
}

static AFFDEF(flonum_xcoerce)
{
  AARG(obj, stype, arg);
  double base, drem;
  AFBEGIN;

  (void)arg;
  if (FIX2INT(AV(stype)) == T_FLONUM || FIX2INT(AV(stype)) == T_COMPLEX
      || FIX2INT(AV(stype)) == T_NUM)
    ARETURN(AV(obj));

  if (FIX2INT(AV(stype)) == T_FIXNUM || FIX2INT(AV(stype)) == T_BIGNUM
      || FIX2INT(AV(stype)) == T_INT) {
    if (fabs(REPFLO(AV(obj))) > FIXNUM_MAX) {
#ifdef HAVE_GMP_H
      value bn;

      bn = arc_mkbignuml(c, 0L);
      mpz_set_d(REPBNUM(bn), REPFLO(AV(obj)));
      ARETURN(bn);
#else
      /* sorry, overflow! */
      arc_err_cstrfmt(c, "cannot coerce (overflow)");
      ARETURN(CNIL);
#endif
    }
    /* rounding algorithm */
    base = floor(REPFLO(AV(obj)));
    drem = REPFLO(AV(obj)) - base;
    if (drem > 0.5 || (drem == 0.5 && (((int)base)&0x1) == 1))
      ARETURN(INT2FIX((long)REPFLO(AV(obj)) + 1));
    ARETURN(INT2FIX((long)REPFLO(AV(obj))));
  }

  if (FIX2INT(AV(stype)) == T_RATIONAL) {
#ifdef HAVE_GMP_H
    value rat;

    rat = arc_mkrationall(c, 0, 1);
    mpq_set_d(REPRAT(rat), REPFLO(AV(obj)));
    ARETURN(rat);
#else
    /* we cannot coerce flonums to rationals without gmp!*/
    arc_err_cstrfmt(c, "cannot coerce");
    ARETURN(CNIL);
#endif
  }

  if (BOUND_P(AV(arg)) && AV(arg) != INT2FIX(10)) {
    arc_err_cstrfmt(c, "inexact numbers can only be printed in base 10");
    ARETURN(CNIL);
  }

  if (FIX2INT(AV(stype)) == T_STRING) {
    char *str;
    int len;

    len = snprintf(NULL, 0, "%g", REPFLO(AV(obj)));
    str = alloca(sizeof(char)*(len+1));
    snprintf(str, len+1, "%g", REPFLO(AV(obj)));
    ARETURN(arc_mkstringc(c, str));
  }

  arc_err_cstrfmt(c, "cannot coerce");
  ARETURN(CNIL);
  AFEND;
}
AFFEND

static value mul_flonum(arc *c, value v1, value v2)
{
  return(arc_mkflonum(c, REPFLO(v1) * REPFLO(v2)));
}

static value div_flonum(arc *c, value v1, value v2)
{
  if (REPFLO(v2) == 0.0) {
    arc_err_cstrfmt(c, "Division by zero");
    return(CNIL);
  }
  return(arc_mkflonum(c, REPFLO(v1) / REPFLO(v2)));
}

static value add_flonum(arc *c, value v1, value v2)
{
  return(arc_mkflonum(c, REPFLO(v1) + REPFLO(v2)));
}

static value sub_flonum(arc *c, value v1, value v2)
{
  return(arc_mkflonum(c, REPFLO(v1) - REPFLO(v2)));
}

value arc_mkflonum(arc *c, double val)
{
  value cv;

  cv = arc_mkobject(c, sizeof(double), T_FLONUM);
  *((double *)REP(cv)) = val;
  return(cv);
}

/*================================= End Flonums */

/*================================= Complex */

static AFFDEF(complex_pprint)
{
  AARG(z);
  AFBEGIN;
  double complex val = *((double *)REP(AV(z)));
  int len;
  char *outstr;

  len = snprintf(NULL, 0, "%g%+gi", creal(val), cimag(val));
  outstr = (char *)alloca(sizeof(char)*(len+2));
  snprintf(outstr, len+1, "%g%+gi", creal(val), cimag(val));
  ARETURN(arc_mkstringc(c, outstr));
  AFEND;
}
AFFEND

static unsigned long complex_hash(arc *c, value f, arc_hs *s)
{
  char *ptr = (char *)REP(f);
  int i;

  for (i=0; i<sizeof(double complex)/sizeof(char); i++)
    arc_hash_update(s, *ptr++);
  return(1);
}

static value complex_iscmp(arc *c, value v1, value v2)
{
  return((*((double complex *)REP(v1)) == *((double complex *)REP(v2))) ? CTRUE : CNIL);
}

/* Note: most of the coercions here will result in loss of the imaginary
   part of the complex number. */
static value complex_coerce(arc *c, value z, enum arc_types t)
{
  double re;

  re = creal(REPCPX(z));

  switch(t) {
  case T_FIXNUM:
    if (ABS(re) > FIXNUM_MAX)
      return(CNIL);
    return(INT2FIX((long)re));
  case T_COMPLEX:
    /* trivial case */
    return(z);
  case T_FLONUM:
    return(arc_mkflonum(c, re));
#ifdef HAVE_GMP_H
  case T_BIGNUM: {
    double base, drem;
    value bignum = arc_mkbignuml(c, 0L);

    if (!isfinite(re) || isnan(re))
      return(CNIL);
    /* see if we need to round */
    base = floor(re);
    drem = re - base;
    mpz_set_d(REPBNUM(bignum), re);
    if (drem > 0.5 || (drem == 0.5 && (((int)base)&0x1) == 1))
      mpz_add_ui(REPBNUM(bignum), REPBNUM(bignum), 1);
    return(bignum);
  }
  case T_RATIONAL: {
    value rat;

    rat = arc_mkrationall(c, 0, 1);
    mpq_set_d(REPRAT(rat), re);
    return(rat);
  }
#endif
  default:
    break;
  }
  return(CNIL);
}

static AFFDEF(complex_xcoerce)
{
  AARG(obj, stype, arg);
  AFBEGIN;

  (void)arg;
  if (FIX2INT(AV(stype)) == T_COMPLEX || FIX2INT(AV(stype)) == T_NUM)
    ARETURN(AV(obj));

  if (FIX2INT(AV(stype)) == T_STRING) {
    char *str;
    int len;

    if (BOUND_P(AV(arg)) && AV(arg) != INT2FIX(10)) {
      arc_err_cstrfmt(c, "inexact numbers can only be printed in base 10");
      ARETURN(CNIL);
    }

    len = snprintf(NULL, 0, "%g+%gi", creal(REPCPX(AV(obj))),
		   cimag(REPCPX(AV(obj))));
    str = alloca(sizeof(char)*(len+1));
    snprintf(str, len+1, "%g+%gi", creal(REPCPX(AV(obj))),
	     cimag(REPCPX(AV(obj))));
    ARETURN(arc_mkstringc(c, str));
  }

  if (FIX2INT(AV(stype)) == T_CONS) {
    ARETURN(cons(c, arc_mkflonum(c, creal(REPCPX(AV(obj)))),
		 arc_mkflonum(c, cimag(REPCPX(AV(obj))))));
  }

  /* T_INT is not valid for complex args */
  arc_err_cstrfmt(c, "cannot coerce");
  ARETURN(CNIL);
  AFEND;
}
AFFEND

static value mul_complex(arc *c, value v1, value v2)
{
  return(arc_mkcomplex(c, REPCPX(v1) * REPCPX(v2)));
}

static value div_complex(arc *c, value v1, value v2)
{
  if (creal(REPCPX(v2)) == 0.0 && cimag(REPCPX(v2)) == 0.0) {
    arc_err_cstrfmt(c, "Division by zero");
    return(CNIL);
  }
  return(arc_mkcomplex(c, REPCPX(v1) / REPCPX(v2)));
}

static value add_complex(arc *c, value v1, value v2)
{
  return(arc_mkcomplex(c, REPCPX(v1) + REPCPX(v2)));
}

static value sub_complex(arc *c, value v1, value v2)
{
  return(arc_mkcomplex(c, REPCPX(v1) - REPCPX(v2)));
}

value arc_mkcomplex(arc *c, double complex z)
{
  value cv;

  cv = arc_mkobject(c, sizeof(double complex), T_COMPLEX);
  *((double complex *)REP(cv)) = z;
  return(cv);
}

/*================================= End Complex */

#ifdef HAVE_GMP_H

/*================================= Bignum */

static void bignum_sweep(arc *c, value v)
{
  mpz_clear(REPBNUM(v));
}

static AFFDEF(bignum_pprint)
{
  AARG(n);
  AFBEGIN;
  char *outstr;
  int len;
  value psv;

  len = mpz_sizeinbase(REPBNUM(AV(n)), 10) + 1;
  /* XXX should we be using a real malloc for this? */
  outstr = (char *)malloc(sizeof(char)*len);
  mpz_get_str(outstr, 10, REPBNUM(AV(n)));
  psv = arc_mkstringc(c, outstr);
  free(outstr);
  ARETURN(psv);
  AFEND;
}
AFFEND

static value bignum_coerce(arc *c, value n, enum arc_types t)
{
  switch(t) {
  case T_FIXNUM:
    if ((mpz_cmp_si(REPBNUM(n), FIXNUM_MAX) >= 0) ||
	(mpz_cmp_si(REPBNUM(n), -FIXNUM_MAX) <= 0))
      return(CNIL);
    return(INT2FIX(mpz_get_si(REPBNUM(n))));
  case T_COMPLEX:
    return(arc_mkcomplex(c, mpz_get_d(REPBNUM(n)) + I*0.0));
  case T_FLONUM:
    return(arc_mkflonum(c, mpz_get_d(REPBNUM(n))));
  case T_BIGNUM:
    /* trivial case */
    return(n);
  case T_RATIONAL: {
      value rat;

      rat = arc_mkrationall(c, 0, 1); 
      mpq_set_z(REPRAT(rat), REPBNUM(n));
      return(rat);
    }
  default:
    break;
  }
  return(CNIL);
}

static unsigned long bignum_hash2(arc *c, mpz_t n, arc_hs *s)
{
  unsigned long *rop;
  size_t numb = sizeof(unsigned long);
  size_t countp, calc_size;
  int i;
 
  calc_size = (mpz_sizeinbase(n,  2) + numb-1) / numb;
  rop = (unsigned long *)malloc(calc_size * numb);
  mpz_export(rop, &countp, 1, numb, 0, 0, n);
  for (i=0; i<countp; i++)
    arc_hash_update(s, rop[i]);
  free(rop);
  return((unsigned long)countp);
}

static unsigned long bignum_hash(arc *c, value n, arc_hs *s)
{
  return(bignum_hash2(c, REPBNUM(n), s));
}

static value bignum_iscmp(arc *c, value v1, value v2)
{
  return((mpz_cmp(REPBNUM(v1), REPBNUM(v2)) == 0) ?
	 CTRUE : CNIL);
}

/* If a bignum can fit in a fixnum, return the fixnum result */
static value bignum_fixnum(arc *c, value n)
{
  if ((mpz_cmp_si(REPBNUM(n), 0) >= 0 &&
       mpz_cmp_si(REPBNUM(n), FIXNUM_MAX) <= 0) ||
      (mpz_cmp_si(REPBNUM(n), 0) < 0 &&
       mpz_cmp_si(REPBNUM(n), -FIXNUM_MAX) >= 0))
    return(INT2FIX(mpz_get_si(REPBNUM(n))));
  return(n);
}

static value mul_bignum(arc *c, value v1, value v2)
{
  value prod;

  prod = arc_mkbignuml(c, 0L);
  mpz_mul(REPBNUM(prod), REPBNUM(v1), REPBNUM(v2));
  prod = bignum_fixnum(c, prod);
  return(prod);
}

static value div_bignum(arc *c, value v1, value v2)
{
  /* This actually results in a rational divide.  If v1 and v2 divide
     each other exactly, then a bignum is produced all the same. */
  v1 = bignum_coerce(c, v1, T_RATIONAL);
  v2 = bignum_coerce(c, v2, T_RATIONAL);
  return(div_rational(c, v1, v2));
}

static value add_bignum(arc *c, value v1, value v2)
{
  value sum;

  sum = arc_mkbignuml(c, 0L);
  mpz_add(REPBNUM(sum), REPBNUM(v1), REPBNUM(v2));
  sum = bignum_fixnum(c, sum);
  return(sum);
}

static value sub_bignum(arc *c, value v1, value v2)
{
  value diff;

  diff = arc_mkbignuml(c, 0L);
  mpz_sub(REPBNUM(diff), REPBNUM(v1), REPBNUM(v2));
  diff = bignum_fixnum(c, diff);
  return(diff);
}

value arc_mkbignuml(arc *c, long val)
{
  value cv;

  cv = arc_mkobject(c, sizeof(mpz_t), T_BIGNUM);
  mpz_init(REPBNUM(cv));
  mpz_set_si(REPBNUM(cv), val);
  return(cv);
}

static AFFDEF(bignum_xcoerce)
{
  AARG(obj, stype, arg);
  AFBEGIN;

  /* trivial cases */
  if (FIX2INT(AV(stype)) == T_FIXNUM || FIX2INT(AV(stype)) == T_BIGNUM
      || FIX2INT(AV(stype)) == T_RATIONAL || FIX2INT(AV(stype)) == T_NUM
      || FIX2INT(AV(stype)) == T_INT)
    ARETURN(AV(obj));
  if (FIX2INT(AV(stype)) == T_FLONUM || FIX2INT(AV(stype)) == T_COMPLEX)
    ARETURN(arc_mkflonum(c, mpz_get_d(REPBNUM(AV(obj)))));

  if (!BOUND_P(AV(arg)))
    AV(arg) = INT2FIX(10);

  if (FIX2INT(AV(stype)) == T_STRING) {
    char *str = mpz_get_str(NULL, FIX2INT(AV(arg)), REPBNUM(AV(obj)));
    value astr = arc_mkstringc(c, str);
    free(str);
    ARETURN(astr);
  }

  arc_err_cstrfmt(c, "cannot coerce");
  ARETURN(CNIL);
  AFEND;
}
AFFEND

/*================================= End Bignum */

/*================================= Rational */

static void rational_sweep(arc *c, value v)
{
  mpq_clear(REPRAT(v));
}

static AFFDEF(rational_pprint)
{
  AARG(q);
  AFBEGIN;
  (void)q;
  /* XXX fill this in */
  ARETURN(CNIL);
  AFEND;
}
AFFEND

static value rational_coerce(arc *c, value q, enum arc_types t)
{
  double d;

  switch(t) {
  case T_FIXNUM: {
    mpz_t quot;
    value fix;

    if (mpq_cmp_si(REPRAT(q), FIXNUM_MAX, 1) > 0 ||
	mpq_cmp_si(REPRAT(q), -FIXNUM_MAX, 1) < 0)
      return(CNIL);
    mpz_init(quot);
    mpz_tdiv_q(quot, mpq_numref(REPRAT(q)), mpq_denref(REPRAT(q)));
    fix = INT2FIX(mpz_get_si(quot));
    mpz_clear(quot);
    return(fix);
  }
  case T_COMPLEX:
    d = mpq_get_d(REPRAT(q));
    return(arc_mkcomplex(c, d + I*0.0));
  case T_FLONUM:
    d = mpq_get_d(REPRAT(q));
    return(arc_mkflonum(c, d));
  case T_BIGNUM: {
    value bn;

    bn = arc_mkbignuml(c, 0L);
    mpz_tdiv_q(REPBNUM(bn), mpq_numref(REPRAT(q)), mpq_denref(REPRAT(q)));
    return(bn);
  }
  case T_RATIONAL:
    /* trivial case */
    return(q);
  default:
    break;
  }
  return(CNIL);
}

static AFFDEF(rational_xcoerce)
{
  AARG(obj, stype, arg);
  value bignum;
  mpq_t rem;
  double drem;

  AFBEGIN;
  if (FIX2INT(AV(stype)) == T_FIXNUM || FIX2INT(AV(stype)) == T_BIGNUM) {
    bignum = arc_mkbignuml(c, 0L);
    mpq_init(rem);
    mpz_tdiv_qr(REPBNUM(bignum), mpq_numref(rem),
		mpq_numref(REPRAT(AV(obj))),
		mpq_denref(REPRAT(AV(obj))));
    mpz_set(mpq_denref(rem), mpq_denref(REPRAT(AV(obj))));
    mpq_canonicalize(rem);
    drem = mpq_get_d(rem);
    mpq_clear(rem);
    if (drem >  0.5)
      mpz_add_ui(REPBNUM(bignum), REPBNUM(bignum), 1);
    else if (drem == 0.5 && mpz_odd_p(REPBNUM(bignum)))
      mpz_add_ui(REPBNUM(bignum), REPBNUM(bignum), 1);
    ARETURN(bignum_fixnum(c, bignum));
  }

  if (FIX2INT(AV(stype)) == T_RATIONAL || FIX2INT(AV(stype)) == T_NUM)
    ARETURN(AV(obj));

  if (FIX2INT(AV(stype)) == T_FLONUM || FIX2INT(AV(stype)) == T_COMPLEX)
    ARETURN(arc_mkflonum(c, mpq_get_d(REPRAT(AV(obj)))));

  if (FIX2INT(AV(stype)) == T_STRING) {
    char *str;
    value astr;

    if (!BOUND_P(AV(arg)))
      AV(arg) = INT2FIX(10);

    str = mpq_get_str(NULL, FIX2INT(AV(arg)), REPRAT(AV(obj)));
    astr = arc_mkstringc(c, str);
    free(str);
    ARETURN(astr);
  }

  if (FIX2INT(AV(stype)) == T_CONS) {
    value num, den;

    num = arc_mkbignuml(c, 0L);
    den = arc_mkbignuml(c, 0L);
    mpz_set(REPBNUM(num), mpq_numref(REPRAT(AV(obj))));
    mpz_set(REPBNUM(den), mpq_denref(REPRAT(AV(obj))));
    num = bignum_fixnum(c, num);
    den = bignum_fixnum(c, den);
    ARETURN(cons(c, num, den));
  }

  arc_err_cstrfmt(c, "cannot coerce");
  ARETURN(CNIL);
  AFEND;
}
AFFEND

static unsigned long rational_hash(arc *c, value q, arc_hs *s)
{
  unsigned long count;

  count = bignum_hash2(c, mpq_numref(REPRAT(q)), s);
  count += bignum_hash2(c, mpq_denref(REPRAT(q)), s);
  return(count);
}

static value rational_iscmp(arc *c, value v1, value v2)
{
  return((mpq_cmp(REPRAT(v1), REPRAT(v2)) == 0) ? CTRUE : CNIL);
}

/* Convert the rational number q to an integer type where possible.
   Otherwise return q.  Used by most rational arithmetic operators. */
value rational_int(arc *c, value q)
{
  value val;
  if (mpz_cmp_si(mpq_denref(REPRAT(q)), 1) != 0) {
    /* not 1, cannot coerce */
    return(q);
  }

  /* It is an integer, try to convert to fixnum or bignum */
  val = rational_coerce(c, q, T_FIXNUM);
  if (!NIL_P(val))
    return(val);

  /* Coerce to bignum */
  return(rational_coerce(c, q, T_BIGNUM));
}

static value mul_rational(arc *c, value v1, value v2)
{
  value prod;

  prod = arc_mkrationall(c, 0, 1);
  mpq_mul(REPRAT(prod), REPRAT(v1), REPRAT(v2));
  prod = rational_int(c, prod);
  return(prod);
}

static value div_rational(arc *c, value v1, value v2)
{
  value quot;

  quot = arc_mkrationall(c, 0, 1);
  /* test for division by zero */
  if (rational_iscmp(c, quot, v2) == CTRUE) {
    arc_err_cstrfmt(c, "Division by zero");
    return(CNIL);
  }
  mpq_div(REPRAT(quot), REPRAT(v1), REPRAT(v2));
  quot = rational_int(c, quot);
  return(quot);
}

static value add_rational(arc *c, value v1, value v2)
{
  value sum;

  sum = arc_mkrationall(c, 0, 1);
  mpq_add(REPRAT(sum), REPRAT(v1), REPRAT(v2));
  sum = rational_int(c, sum);
  return(sum);
}

static value sub_rational(arc *c, value v1, value v2)
{
  value diff;

  diff = arc_mkrationall(c, 0, 1);
  mpq_sub(REPRAT(diff), REPRAT(v1), REPRAT(v2));
  diff = rational_int(c, diff);
  return(diff);
}

value arc_mkrationall(arc *c, long num, long den)
{
  value cv;

  cv = arc_mkobject(c, sizeof(mpq_t), T_RATIONAL);
  mpq_init(REPRAT(cv));
  mpq_set_si(REPRAT(cv), num, den);
  mpq_canonicalize(REPRAT(cv));
  return(cv);
}

/*================================= End Rational */

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
   available (in which case results are extended to flonum).

   If a rational has a denominator of 1, the result is implicitly
   converted to a fixnum, if the range allows, or a bignum if not.

   Division of fixnums and/or bignums will result in a fixnum or a
   bignum only if the two numbers divide each other exactly.  If the
   division is inexact, the result will be a rational number if
   support for bignum/rational arithmetic has been compiled in.  If
   not, the quotient alone is returned.
 */

#ifdef HAVE_GMP_H

#define TYPE_CASES(func, arg1, arg2) do {				\
    value (*coerce1)(struct arc *, value, enum arc_types);		\
    value (*coerce2)(struct arc *, value, enum arc_types);		\
    coerce1 = coercefn(c, arg1);					\
    coerce2 = coercefn(c, arg2);					\
    if (coerce1 == NULL || coerce2 == NULL) {				\
      arc_err_cstrfmt(c, "cannot coerce %d %d", TYPE(arg1), TYPE(arg2)); \
      return(CNIL);							\
    }									\
    if (TYPE(arg1) == T_COMPLEX || TYPE(arg2) == T_COMPLEX) {		\
      arg1 = coerce1(c, arg1, T_COMPLEX);				\
      arg2 = coerce2(c, arg2, T_COMPLEX);				\
      return(func##_complex(c, arg1, arg2));				\
    } else if (TYPE(arg1) == T_FLONUM || TYPE(arg2) == T_FLONUM) {	\
      arg1 = coerce1(c, arg1, T_FLONUM);				\
      arg2 = coerce2(c, arg2, T_FLONUM);				\
      return(func##_flonum(c, arg1, arg2));				\
    } else if (TYPE(arg1) == T_RATIONAL || TYPE(arg2) == T_RATIONAL) {	\
      arg1 = coerce1(c, arg1, T_RATIONAL);				\
      arg2 = coerce2(c, arg2, T_RATIONAL);				\
      return(func##_rational(c, arg1, arg2));				\
    } else if (TYPE(arg1) == T_BIGNUM || TYPE(arg2) == T_BIGNUM) {	\
      arg1 = coerce1(c, arg1, T_BIGNUM);				\
      arg2 = coerce2(c, arg2, T_BIGNUM);				\
      return(func##_bignum(c, arg1, arg2));				\
    }									\
  } while (0)

#else

#define TYPE_CASES(func, arg1, arg2) do {				\
    value (*coerce1)(struct arc *, value, enum arc_types);		\
    value (*coerce2)(struct arc *, value, enum arc_types);		\
    coerce1 = coercefn(c, arg1);					\
    coerce2 = coercefn(c, arg2);					\
    if (coerce1 == NULL || coerce2 == NULL) {				\
      arc_err_cstrfmt(c, "cannot coerce %d %d", TYPE(arg1), TYPE(arg2)); \
      return(CNIL);							\
    }									\
    if (TYPE(arg1) == T_COMPLEX || TYPE(arg2) == T_COMPLEX) {		\
      arg1 = coerce1(c, arg1, T_COMPLEX);				\
      arg2 = coerce2(c, arg2, T_COMPLEX);				\
      return(func##_complex(c, arg1, arg2));				\
    } else if (TYPE(arg1) == T_FLONUM || TYPE(arg2) == T_FLONUM) {	\
      arg1 = coerce1(c, arg1, T_FLONUM);				\
      arg2 = coerce2(c, arg2, T_FLONUM);				\
      return(func##_flonum(c, arg1, arg2));				\
    }									\
  } while (0)

#endif

static int hibit(unsigned long n)
{
  int ret;

  if (!n)
    return(0);
  ret = 1;
  while (n >>= 1 && n)
    ret++;
  return(ret);
}

value __arc_mul2(arc *c, value arg1, value arg2)
{
  if (TYPE(arg1) == T_FIXNUM && TYPE(arg2) == T_FIXNUM) {
    long varg1, varg2;

    varg1 = FIX2INT(arg1);
    varg2 = FIX2INT(arg2);

    /* Check to see if the size of the product will fit in a fixnum.
       Promote to bignum or flonum as needed. */
    if (hibit(ABS(varg1)) + hibit(ABS(varg2)) >= hibit(FIXNUM_MAX)) {
#ifdef HAVE_GMP_H
      return(mul_bignum(c, arc_mkbignuml(c, varg1),
			arc_mkbignuml(c, varg2)));
#else
      /* Multiply as flonums in this case */
      return(mul_flonum(c, arc_mkflonum(c, (double)varg1),
			arc_mkflonum(c, (double)varg2)));
#endif
    }
    return(INT2FIX(varg1 * varg2));
  }

  TYPE_CASES(mul, arg1, arg2);

  arc_err_cstrfmt(c, "Invalid types for multiplication");
  return(CNIL);
}

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
    if (res.rem == 0) {
      if (ABS(res.quot) > FIXNUM_MAX) {
#ifdef HAVE_GMP_H
	return(arc_mkbignuml(c, res.quot));
#else
	return(arc_mkflonum(c, (double)res.quot));
#endif
      }
      return(INT2FIX(res.quot));
    } else {
      /* Not an exact integer division.  Will produce either a
	 rational or a flonum depending on whether or not gmp
	 support is enabled. */
#ifdef HAVE_GMP_H
      return(arc_mkrationall(c, varg1, varg2));
#else
      return(arc_mkflonum(c, ((double)varg1) / ((double)varg2)));
#endif
    }
  }

  TYPE_CASES(div, arg1, arg2);

  arc_err_cstrfmt(c, "Invalid types for division");
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
  else if ((TYPE(arg1) == T_BIGNUM || TYPE(arg1) == T_FIXNUM) &&
	   (TYPE(arg1) == T_BIGNUM || TYPE(arg1) == T_FIXNUM)) {
    value barg1, barg2, quot;
    value (*coerce1)(struct arc *, value, enum arc_types);
    value (*coerce2)(struct arc *, value, enum arc_types);

    coerce1 = coercefn(c, arg1);
    coerce2 = coercefn(c, arg2);
    /* Coerce both args to bignum */
    barg1 = coerce1(c, arg1, T_BIGNUM);
    barg2 = coerce2(c, arg2, T_BIGNUM);
    quot = arc_mkbignuml(c, 0L);
    mpz_fdiv_q(REPBNUM(quot), REPBNUM(barg1), REPBNUM(barg2));
    return(bignum_fixnum(c, quot));
  }
#endif
  arc_err_cstrfmt(c, "Invalid types for idiv");
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
  else if ((TYPE(arg1) == T_BIGNUM || TYPE(arg1) == T_FIXNUM) &&
	   (TYPE(arg1) == T_BIGNUM || TYPE(arg1) == T_FIXNUM)) {
    value barg1, barg2, modulus;
    value (*coerce1)(struct arc *, value, enum arc_types);
    value (*coerce2)(struct arc *, value, enum arc_types);

    coerce1 = coercefn(c, arg1);
    coerce2 = coercefn(c, arg2);

    /* Coerce both args to bignum */
    barg1 = coerce1(c, arg1, T_BIGNUM);
    barg2 = coerce2(c, arg2, T_BIGNUM);
    modulus = arc_mkbignuml(c, 0L);
    mpz_fdiv_r(REPBNUM(modulus), REPBNUM(barg1), REPBNUM(barg2));
    return(bignum_fixnum(c, modulus));
  }
#endif
  arc_err_cstrfmt(c, "Invalid types for modulus");
  return(CNIL);
}

value __arc_add2(arc *c, value arg1, value arg2)
{
  long fixnum_sum;

  if (TYPE(arg1) == T_FIXNUM && TYPE(arg2) == T_FIXNUM) {
    fixnum_sum = FIX2INT(arg1) + FIX2INT(arg2);
    if (ABS(fixnum_sum) > FIXNUM_MAX) {
#ifdef HAVE_GMP_H
      return(arc_mkbignuml(c, fixnum_sum));
#else
/* without bignum support, we extend to flonum */
      return(arc_mkflonum(c, (double)fixnum_sum));
#endif
    }
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
    return(arc_list_append(arg1, arg2));
  }

  if ((NIL_P(arg1) || TYPE(arg1) == T_STRING)
      && (NIL_P(arg2) || TYPE(arg2) == T_STRING)) {
    if (arg1 == CNIL)
      return(arg2);
    if (arg2 == CNIL)
      return(arg1);
    return(arc_strcat(c, arg1, arg2));
  }

  if (NIL_P(arg1) && TYPE(arg2) == T_CHAR) {
    Rune data[1];

    data[0] = *((Rune *)REP(arg2));
    return(arc_mkstring(c, data, 1));
  }

  if (NIL_P(arg2) && TYPE(arg1) == T_CHAR) {
    Rune data[1];

    data[0] = *((Rune *)REP(arg1));
    return(arc_mkstring(c, data, 1));
  }

  if (TYPE(arg1) == T_CHAR && TYPE(arg2) == T_CHAR) {
    Rune data1, data2;

    data1 = *((Rune *)REP(arg1));
    data2 = *((Rune *)REP(arg2));
    return(arc_strcat(c, arc_mkstring(c, &data1, 1), 
		      arc_mkstring(c, &data2, 1)));
  }

  if (TYPE(arg1) == T_STRING && TYPE(arg2) == T_CHAR) {
    return(arc_strcatc(c, arg1, *((Rune *)REP(arg2))));
  }

  if (TYPE(arg1) == T_CHAR && TYPE(arg2) == T_STRING) {
    Rune data[1];

    data[0] = *((Rune *)REP(arg1));
    return(arc_strcat(c, arc_mkstring(c, data, 1), arg2));
  }

  if (TYPE(arg1) == T_STRING) {
    value carg2;
    value (*coerce1)(struct arc *, value, enum arc_types);

    coerce1 = coercefn(c, arg2);

    if (coerce1 == NULL) {
      arc_err_cstrfmt(c, "cannot coerce to string");
      return(CNIL);
    }
    carg2 = coerce1(c, arg2, T_STRING);
    return(arc_strcat(c, arg1, carg2));
  }

  TYPE_CASES(add, arg1, arg2);

  arc_err_cstrfmt(c, "Invalid types for addition");
  return(CNIL);
}

value __arc_sub2(arc *c, value arg1, value arg2)
{
  long fixnum_diff;

  if (TYPE(arg1) == T_FIXNUM && TYPE(arg2) == T_FIXNUM) {
    fixnum_diff = FIX2INT(arg1) - FIX2INT(arg2);
    if (ABS(fixnum_diff) > FIXNUM_MAX) {
#ifdef HAVE_GMP_H
      return(arc_mkbignuml(c, fixnum_diff));
#else
      /* promote to flonum */
      return(arc_mkflonum(c, (double)fixnum_diff));
#endif
    }
    return(INT2FIX(fixnum_diff));
  } 

  TYPE_CASES(sub, arg1, arg2);

  arc_err_cstrfmt(c, "Invalid types for subtraction");
  return(CNIL);
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

  while ((ch = __arc_strgetc(c, str, &index)) != Runeerror) {
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
	if (!(isdigit(ch) || ch == '.'))
	  return(CNIL);
	__arc_strungetc(c, &index);
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
	  REPCPX(imag) += sign * (mantissa * pow(10, expnsign * expn));
	  return(imag);
	}
	break;
      case 'i':
      case 'I':
      case 'j':
      case 'J':
	/* imaginary */
	return(arc_mkcomplex(c, I * sign * (mantissa
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
	  REPCPX(imag) += sign * (mantissa * pow(10, expnsign * expn));
	  return(imag);
	}
	break;
      case 'i':
      case 'I':
      case 'j':
      case 'J':
	/* imaginary */
	return(arc_mkcomplex(c, 0.0 +
			      I *sign * (mantissa
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
	__arc_strungetc(c, &index);
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
	  REPCPX(imag) += sign * (mantissa * pow(10, expnsign * expn));
	  return(imag);
	}
	break;
      case 'i':
      case 'I':
      case 'j':
      case 'J':
	/* imaginary */
	return(arc_mkcomplex(c, 0.0 + 
			     I * sign * (mantissa
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

value arc_string2num(arc *c, value str, int index, int rational)
{
  int state = 1, sign=1, radsel = 0, i;
  Rune ch;
  value nval = INT2FIX(0), digitval, radix = INT2FIX(10), denom;
  Rune chs[4];

  while ((ch = __arc_strgetc(c, str, &index)) != Runeerror) {
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
	__arc_strungetc(c, &index);
	state = 2;
	break;
      }
      break;
    case 2:
      /* digits, or possible radix */
      switch (ch) {
	/* Use the Limbo-style radix selector!
      case '0':
	radix = INT2FIX(8);
	state = 3;
	break;
	*/
      case '.':
	return(str2flonum(c, str, 0, 0));
	break;
      default:
	if (tolower(ch) == 'i') {
	  for (i=0; i<4; i++) {
	    chs[i] = __arc_strgetc(c, str, &index);
	    if (chs[i] == Runeerror)
	      return(CNIL);
	  }
	  if (chs[0] == 'n' && chs[1] == 'f' && chs[2] == '.' && chs[3] == '0')
	    return(arc_mkflonum(c, sign*INFINITY));
	  return(CNIL);
	}
	if (tolower(ch) == 'n') {
	  for (i=0; i<4; i++) {
	    chs[i] = __arc_strgetc(c, str, &index);
	    if (chs[i] == Runeerror)
	      return(CNIL);
	  }
	  if (chs[0] == 'a' && chs[1] == 'n' && chs[2] == '.' && chs[3] == '0')
	    return(arc_mkflonum(c, NAN));
	  return(CNIL);
	}
	if (!isdigit(ch))
	  return(CNIL);
	/* more digits */
	__arc_strungetc(c, &index);
	state = 4;
	break;
      }
      break;
    case 3:
      /* digits, or possible radix */
      switch (ch) {
	/*
      case 'x':
	radix = INT2FIX(16);
	state = 4;
	break;
      case 'b':
	radix = INT2FIX(2);
	state = 4;
	break;x
	*/
      case '.':
	return(str2flonum(c, str, 0, 0));
	break;
      case '+':
	/* complex */
	return(str2flonum(c, str, 0, 0));
	break;
      case '-':
	/* complex */
	return(str2flonum(c, str, 0, 0));
	break;
      default:
	if (!isdigit(ch))
	  return(CNIL);
	/* more digits */
	__arc_strungetc(c, &index);
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
	denom = arc_string2num(c, str, index, 1);
	if (TYPE(denom) == T_FIXNUM || TYPE(denom) == T_BIGNUM)
	  return(__arc_div2(c, __arc_mul2(c, INT2FIX(sign), nval), denom));
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
    case 6:
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

static value (*coercefn(arc *c, value val))(arc *, value, enum arc_types)
{
  if (TYPE(val) == T_FIXNUM)
    return(fixnum_coerce);
  if (TYPE(val) == T_FLONUM)
    return(flonum_coerce);
  if (TYPE(val) == T_COMPLEX)
    return(complex_coerce);
#ifdef HAVE_GMP_H
  if (TYPE(val) == T_BIGNUM)
    return(bignum_coerce);
  if (TYPE(val) == T_RATIONAL)
    return(rational_coerce);
#endif
  return(NULL);
}

value __arc_neg(arc *c, value arg)
{
  switch (TYPE(arg)) {
  case T_FIXNUM:
    return(INT2FIX(-FIX2INT(arg)));
  case T_FLONUM:
    return(arc_mkflonum(c, -REPFLO(arg)));
  case T_COMPLEX:
    return(arc_mkcomplex(c, -REPCPX(arg)));
#ifdef HAVE_GMP_H
  case T_BIGNUM:
    {
      value big;
      big = arc_mkbignuml(c, 0);
      mpz_neg(REPBNUM(big), REPBNUM(arg));
      return(big);
    }
    break;
  case T_RATIONAL:
    {
      value rat;
      rat = arc_mkrationall(c, 0, 1);
      mpq_neg(REPRAT(rat), REPRAT(arg));
      return(rat);
    }
    break;
#endif
  default:
    arc_err_cstrfmt(c, "Invalid type for negation");
  }
  return(CNIL);
}

value arc_expt(arc *c, value a, value b)
{
  value ac, bc;
  double complex p;
  value (*coerce_a)(struct arc *, value, enum arc_types);
  value (*coerce_b)(struct arc *, value, enum arc_types);
#ifdef HAVE_GMP_H
  mpz_t anumer, adenom;
  unsigned long int babs;
  value result;
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

  coerce_a = coercefn(c, a);
  coerce_b = coercefn(c, b);
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
       Im(p) ~== 0.0.  Note that complex exponents are NOT
       supported by PG-ARC! */
    ac = coerce_a(c, a, T_COMPLEX);
    bc = coerce_b(c, b, T_COMPLEX);
    p = cpow(REPCPX(ac), REPCPX(bc));
    if (fabs(cimag(p)) < DBL_EPSILON)
      return(arc_mkflonum(c, creal(p)));
    return(arc_mkcomplex(c, p));
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
    mpz_set(anumer, REPBNUM(a));
    mpz_set_si(adenom, 1);
    break;
  case T_RATIONAL:
    mpq_get_num(anumer, REPRAT(a));
    mpq_get_den(adenom, REPRAT(a));
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
    mpz_set(REPBNUM(result), anumer);
    result = bignum_fixnum(c, result);
  } else {
    result = arc_mkrationall(c, 0, 1);
    mpq_set_num(REPRAT(result), anumer);
    mpq_set_den(REPRAT(result), adenom);
    mpq_canonicalize(REPRAT(result));
  }
  mpz_clear(anumer);
  mpz_clear(adenom);
  return(result);
#endif
}

value __arc_str2int(arc *c, value obj, value base, int strptr, int limit)
{
  /* Tell me if this looks too much like glibc's implementation of
     strtol... */
  value res;
  int save, negative;
  Rune r;

  if (strptr >= limit)
    goto noconv;
  if (arc_strindex(c, obj, strptr) == '-') {
    negative = 1;
    ++strptr;
  } else if (arc_strindex(c, obj, strptr) == '+') {
    negative = 0;
    ++strptr;
  } else
    negative = 0;
  /* save the pointer so we can check later if anything happened */
  save = strptr;
  res = INT2FIX(0);
  for (r = arc_strindex(c, obj, strptr); strptr < limit; r = arc_strindex(c, obj, ++strptr)) {
    if (isdigit(r))
      r -= '0';
    else if (isalpha(r))
      r = toupper(r) - 'A' + 10;
    else
      goto noconv;
    if (r > FIX2INT(base))
      goto noconv;
    res = __arc_mul2(c, res, base);
    res = __arc_add2(c, res, INT2FIX(r));
  }

  /* check if anything actually happened */
  if (strptr == save)
    goto noconv;

  if (negative)
    res = __arc_neg(c, res);
  return(res);
 noconv:
  return(CNIL);
}

static int digitval(Rune r, int base)
{
  int v;

  if (isdigit(r)) {
    v = r - '0';
  } else if (isalpha(r)) {
    v = toupper(r) - 'A' + 10;
  } else
    return(-1);

  if (v >= base)
    return(-1);
  return(v);
}

/* convert a string to a floating point number */
value __arc_str2flo(arc *c, value obj, value b, int strptr, int limit)
{
  int sign;
  double num;			/* the number so far */
  int got_dot;			/* found a (decimal) point */
  int got_digit;		/* seen any digits */
  value exponent;		/* exponent of the number */
  Rune r;
  int dv;			/* digit value */
  int base;			/* base */
  value res;
  char str[4];
  value (*coercer)(struct arc *, value, enum arc_types); 

  base = FIX2INT(b);
  if (strptr >= limit)
    goto noconv;

  /* Get the sign */
  r = arc_strindex(c, obj, strptr);
  sign = (r == '-') ? -1 : 1;
  if (r == '-' || r == '+')
    ++strptr;

  num = 0.0;
  got_dot = 0;
  got_digit = 0;
  exponent = INT2FIX(0);

  /* Check special cases of INF and NAN */
  if (limit - strptr == 3 || limit - strptr == 5) {
    int i;

    for (i=0; i<3; i++)
      str[i] = (char)arc_strindex(c, obj, strptr + i);
    str[3] = 0;
    if (strcasecmp(str, "INF") == 0 || strcasecmp(str, "inf.0") == 0)
      return(arc_mkflonum(c, sign*INFINITY));
    if (strcasecmp(str, "NAN") == 0)
      return(arc_mkflonum(c, sign*NAN));
  }
  for (;; ++strptr) {
    if (strptr >= limit)
      break;
    r = arc_strindex(c, obj, strptr);
    dv = digitval(r, base);
    if (dv >= 0) {
      got_digit = 1;
      num = (num * (double)base) + (double)dv;
      /* Keep track of the number of digits after the decimal point.
	 If we just divided by base here, we would lose precision. */
      if (got_dot)
	exponent = INT2FIX(FIX2INT(exponent) - 1);
    } else if (!got_dot && r == '.') {
      got_dot = 1;
    } else {
      /* any other character terminates the number */
      break;
    }
  }

  if (!got_digit)
    goto noconv;

  if (strptr < limit) {
    r = tolower(arc_strindex(c, obj, strptr++));
    if (r == 'e' || r == 'p' || r == '&') {
      /* get the exponent specified after the 'e', 'p', or '&' */
      value exp;

      exp = __arc_str2int(c, obj, b, strptr, limit);

#ifdef HAVE_GMP_H
      if (TYPE(exp) == T_BIGNUM) {
	/* The exponent overflowed a fixnum.  It is probably a safe assumption
	   that an exponent that needs to be represented by a bignum exceeds
	   the limits of a double!  A highly negative exponent is essentially
	   zero. */
	if (mpz_cmp_si(REPBNUM(exp), 0) < 0)
	  return(arc_mkflonum(c, 0.0));
	else
	  return(arc_mkflonum(c, INFINITY));
      }
#endif
      exponent = __arc_add2(c, exponent, exp);
      if (num == 0.0)
	return(arc_mkflonum(c, 0.0));

#ifdef HAVE_GMP_H
      /* check the exponent again */
      if (TYPE(exponent) == T_BIGNUM) {
	if (mpz_cmp_si(REPBNUM(exp), 0) < 0)
	  return(arc_mkflonum(c, 0.0));
	else
	  return(arc_mkflonum(c, INFINITY));
      }
#endif
    }
  }

  coercer = coercefn(c, b);
  b = coercer(c, b, T_FLONUM);
  coercer = coercefn(c, exponent);
  exponent = coercer(c, exponent, T_FLONUM);
  /* Multiply NUM by BASE to the EXPONENT power */
  res = __arc_mul2(c, arc_mkflonum(c, sign*num), arc_expt(c, b, exponent));
  return(res);
 noconv:
  return(CNIL);
}

typefn_t __arc_fixnum_typefn__ = {
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  fixnum_xcoerce
};

typefn_t __arc_flonum_typefn__ = {
  __arc_null_marker,
  __arc_null_sweeper,
  flonum_pprint,
  flonum_hash,
  flonum_iscmp,
  NULL,
  NULL,
  flonum_xcoerce
};

typefn_t __arc_complex_typefn__ = {
  __arc_null_marker,
  __arc_null_sweeper,
  complex_pprint,
  complex_hash,
  complex_iscmp,
  NULL,
  NULL,
  complex_xcoerce
};

#ifdef HAVE_GMP_H

typefn_t __arc_bignum_typefn__ = {
  __arc_null_marker,
  bignum_sweep,
  bignum_pprint,
  bignum_hash,
  bignum_iscmp,
  NULL,
  NULL,
  bignum_xcoerce
};

typefn_t __arc_rational_typefn__ = {
  __arc_null_marker,
  rational_sweep,
  rational_pprint,
  rational_hash,
  rational_iscmp,
  NULL,
  NULL,
  rational_xcoerce
};

#endif
