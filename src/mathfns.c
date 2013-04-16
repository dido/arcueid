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

  coerce_a = __arc_coercefn(c, a);
  coerce_b = __arc_coercefn(c, b);
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
    result = __arc_bignum_fixnum(c, result);
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
    COMPARE(REPFLO(diff));
    break;
#ifdef HAVE_GMP_H
  case T_BIGNUM:
    return(INT2FIX(mpz_sgn(REPBNUM(diff))));
    break;
  case T_RATIONAL:
    return(INT2FIX(mpq_sgn(REPRAT(diff))));
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
AFFDEF(arc_rand)
{
  AOARG(mmax);
  uint64_t rnd1, rnd2, max;
  AFBEGIN;

  if (!BOUND_P(AV(mmax))) {
    rnd1 = RAND() & 0x1fffffffffffffLL;
    rnd2 = RAND() & 0x1fffffffffffffLL;
    ARETURN(arc_mkflonum(c, (double)rnd1 / (double)rnd2));
  }

  if (TYPE(AV(mmax)) != T_FIXNUM) {
    arc_err_cstrfmt(c, "rand expects fixnum argument, given type %d", TYPE(AV(mmax)));
    ARETURN(CNIL);
  }
  max = FIX2INT(AV(mmax));
  /* XXX - We should have a better algorithm than this, but this should do
     just fine for now.  This simple method introduces a slight bias into
     the random number. */
  ARETURN(INT2FIX(RAND() % max));
  AFEND;
}
AFFEND

static value flosqrt(arc *c, value v)
{
  double complex s;
  double d;
  long int i;
  value (*coerce1)(struct arc *, value, enum arc_types);

  coerce1 = __arc_coercefn(c, v);
  if (coerce1 == NULL) {
    arc_err_cstrfmt(c, "cannot coerce to flonum");
    return(CNIL);
  }
  v = coerce1(c, v, T_FLONUM);
  s = csqrt(REPFLO(v));
  if (fabs(cimag(s)) > DBL_EPSILON)
    return(arc_mkcomplex(c, s));
  /* close enough to real number */
  d = creal(s);
  if (d < FIXNUM_MAX) {
    i = lrint(d);
    /* see if it's close enough to int */
    if (fabs(d - (double)i) < DBL_EPSILON)
      return(INT2FIX(i));
    /* no, return the double */
  }
  return(arc_mkflonum(c, d));
}

value arc_sqrt(arc *c, value v)
{
  double complex s;
  double d;
  int i;

  switch (TYPE(v)) {
  case T_FIXNUM:
    return(flosqrt(c, v));
    break;
#ifdef HAVE_GMP_H
  case T_BIGNUM:
    {
      value v2;

      /* fall back to floating point square root if inexact or negative */
      if (mpz_cmp_si(REPBNUM(v), 0) <= 0
	  || !mpz_perfect_square_p(REPBNUM(v)))
	return(flosqrt(c, v));
      v2 = arc_mkbignuml(c, 0);
      mpz_sqrt(REPBNUM(v2), REPBNUM(v));
      return(__arc_bignum_fixnum(c, v2));
    }
    break;
#endif
  case T_FLONUM:
    return(flosqrt(c, v));
    break;
#ifdef HAVE_GMP_H
  case T_RATIONAL:
    /* check for inexact case */
    if ((mpq_sgn(REPRAT(v)) <= 0)
	|| !mpz_perfect_square_p(mpq_numref(REPRAT(v)))
	|| !mpz_perfect_square_p(mpq_denref(REPRAT(v)))) {
      return(flosqrt(c, v));
    }
    /* exact case */
    {
      value v2;

      v2 = arc_mkrationall(c, 0, 1);
      mpz_sqrt(mpq_numref(REPRAT(v2)), mpq_numref(REPRAT(v)));
      mpz_sqrt(mpq_denref(REPRAT(v2)), mpq_denref(REPRAT(v)));
      mpq_canonicalize(REPRAT(v2));
      return(__arc_rational_int(c, v2));
    }
    break;
#endif
  case T_COMPLEX:
    s = csqrt(REPCPX(v));
    if (fabs(cimag(s)) < DBL_EPSILON) {
      d = creal(s);
      if (d < FIXNUM_MAX) {
	i = lrint(d);
	/* see if it's close enough to int */
	if (fabs(d - (double)i) < DBL_EPSILON)
	  return(INT2FIX(i));
      }
      /* no, return the double */
      return(arc_mkflonum(c, d));
    }
    /* complex case */
    return(arc_mkcomplex(c, s));
  default:
    arc_err_cstrfmt(c, "sqrt expects numeric type, given object of type %d",
		    TYPE(v));
  }
  return(CNIL);
}

value __arc_real(arc *c, value v)
{
  switch (TYPE(v)) {
  case T_FIXNUM:
  case T_BIGNUM:
  case T_RATIONAL:
  case T_FLONUM:
    return(v);
  case T_COMPLEX:
    return(arc_mkflonum(c, creal(REPCPX(v))));
  default:
    arc_err_cstrfmt(c, "real expects numeric type, given object of type %d",
		    TYPE(v));
  }
  return(CNIL);
}

value __arc_imag(arc *c, value v)
{
  switch (TYPE(v)) {
  case T_FIXNUM:
  case T_BIGNUM:
  case T_RATIONAL:
  case T_FLONUM:
    return(INT2FIX(0));
  case T_COMPLEX:
    return(arc_mkflonum(c, cimag(REPCPX(v))));
  default:
    arc_err_cstrfmt(c, "imag expects numeric type, given object of type %d",
		    TYPE(v));
  }
  return(CNIL);
}

value __arc_conj(arc *c, value v)
{
  switch (TYPE(v)) {
  case T_FIXNUM:
  case T_BIGNUM:
  case T_RATIONAL:
  case T_FLONUM:
    return(v);
  case T_COMPLEX:
    return(arc_mkcomplex(c, conj(REPCPX(v))));
  default:
    arc_err_cstrfmt(c, "conj expects numeric type, given object of type %d",
		    TYPE(v));
  }
  return(CNIL);
}

value __arc_arg(arc *c, value v)
{
  switch (TYPE(v)) {
  case T_FIXNUM:
  case T_BIGNUM:
  case T_RATIONAL:
  case T_FLONUM:
    return(INT2FIX(0));
  case T_COMPLEX:
    return(arc_mkflonum(c, carg(REPCPX(v))));
  default:
    arc_err_cstrfmt(c, "arg expects numeric type, given object of type %d",
		    TYPE(v));
  }
  return(CNIL);
}

value arc_trunc(arc *c, value v)
{
  switch (TYPE(v)) {
  case T_FIXNUM:
  case T_BIGNUM:
    return(v);
    break;
  case T_FLONUM:
    /* try to coerce to fixnum first */
    if (ABS(REPFLO(v)) <= FIXNUM_MAX)
      return(INT2FIX((int)REPFLO(v)));
#ifdef HAVE_GMP_H
    {
      value bn;

      if (!isfinite(REPFLO(v)) || isnan(REPFLO(v))) {
	arc_err_cstrfmt(c, "cannot truncate infinity or NAN");
	return(CNIL);
      }
      bn = arc_mkbignuml(c, 0L);
      mpz_set_d(REPBNUM(bn), REPFLO(v));
      return(bn);
    }
#else
    arc_err_cstrfmt(c, "flonum->fixnum conversion overflow (this version of Arcueid does not have bignum support)");
    return(CNIL);
#endif
    break;
#ifdef HAVE_GMP_H
  case T_RATIONAL:
    {
      value bn;

      bn = arc_mkbignuml(c, 0L);
      mpz_tdiv_q(REPBNUM(bn), mpq_numref(REPRAT(v)), mpq_denref(REPRAT(v)));
      return(bn);
    }
    break;
#endif
  case T_COMPLEX:
    arc_err_cstrfmt(c, "cannot truncate complex number to integer");
    break;
  default:
    arc_err_cstrfmt(c, "truncate expects numeric type, given object of type %d", TYPE(v));
    break;
  }
  return(CNIL);
}
