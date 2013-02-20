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

#define ABS(x) (((x)>=0)?(x):(-(x)))
#define SGN(x) (((x)>=0)?(1):(-(1)))

#define REPFLO(f) *((double *)REP(f))
#define REPCPX(z) *((double complex *)REP(f))

#ifdef HAVE_GMP_H
#define REPBNUM(n) *((mpz_t *)REP(n))
#define REPRAT(q) *((mpq_t *)REP(q))
#endif

static value flonum_pprint(arc *c, value f, value *ppstr, value visithash)
{
  double val = *((double *)REP(f));
  int len;
  char *outstr;

  len = snprintf(NULL, 0, "%g", val);
  outstr = (char *)alloca(sizeof(char)*(len+2));
  snprintf(outstr, len+1, "%g", val);
  __arc_append_cstring(c, outstr, ppstr);
  return(*ppstr);
}

/* XXX - this may not work properly if sizeof(double) does not precisely
   divide sizeof(unsigned long). */
static value flonum_hash(arc *c, value f, arc_hs *s, value visithash)
{
  unsigned long *ptr = (unsigned long *)REP(f);
  int i;

  for (i=0; i<sizeof(double)/sizeof(unsigned long); i++)
    arc_hash_update(s, *ptr++);
  return(1);
}

static value flonum_iscmp(arc *c, value v1, value v2)
{
  return((*((double *)REP(v1)) == *((double *)REP(v2))) ? CTRUE : CNIL);
}

static value flonum_isocmp(arc *c, value v1, value v2, value vh1, value vh2)
{
  return(flonum_iscmp(c, v1, v2));
}

value arc_mkflonum(arc *c, double val)
{
  value cv;

  cv = arc_mkobject(c, sizeof(double), T_FLONUM);
  *((double *)REP(cv)) = val;
  return(cv);
}

typefn_t __arc_flonum_typefn__ = {
  __arc_null_marker,
  __arc_null_sweeper,
  flonum_pprint,
  flonum_hash,
  flonum_iscmp,
  flonum_isocmp
};

static value complex_pprint(arc *c, value z, value *ppstr, value visithash)
{
  double complex val = *((double complex *)REP(z));
  int len;
  char *outstr;

  len = snprintf(NULL, 0, "%g%+gi", creal(val), cimag(val));
  outstr = (char *)alloca(sizeof(char)*(len+2));
  snprintf(outstr, len+1, "%g%+gi", creal(val), cimag(val));
  __arc_append_cstring(c, outstr, ppstr);
  return(*ppstr);
}

/* XXX - this may not work properly if sizeof(double) does not precisely
   divide sizeof(unsigned long). */
static value complex_hash(arc *c, value f, arc_hs *s, value visithash)
{
  unsigned long *ptr = (unsigned long *)REP(f);
  int i;

  for (i=0; i<sizeof(double complex)/sizeof(unsigned long); i++)
    arc_hash_update(s, *ptr++);
  return(1);
}

static value complex_iscmp(arc *c, value v1, value v2)
{
  return((*((double complex *)REP(v1)) == *((double complex *)REP(v2))) ? CTRUE : CNIL);
}

static value complex_isocmp(arc *c, value v1, value v2, value vh1, value vh2)
{
  return(complex_iscmp(c, v1, v2));
}

value arc_mkcomplex(arc *c, double complex z)
{
  value cv;

  cv = arc_mkobject(c, sizeof(double complex), T_COMPLEX);
  *((double complex *)REP(cv)) = z;
  return(cv);
}

typefn_t __arc_complex_typefn__ = {
  __arc_null_marker,
  __arc_null_sweeper,
  complex_pprint,
  complex_hash,
  complex_iscmp,
  complex_isocmp
};

#ifdef HAVE_GMP_H

static void bignum_sweep(arc *c, value v)
{
  mpz_clear(REPBNUM(v));
}

static value bignum_pprint(arc *c, value n, value *ppstr, value visithash)
{
  char *outstr;
  int len;

  len = mpz_sizeinbase(REPBNUM(n), 10) + 1;
  /* XXX should we use a real malloc for this? */
  outstr = (char *)malloc(sizeof(char)*len);
  mpz_get_str(outstr, 10, REPBNUM(n));
  __arc_append_cstring(c, outstr, ppstr);
  free(outstr);
  return(*ppstr);
}

static value bignum_hash(arc *c, value n, arc_hs *s, value visithash)
{
  unsigned long *rop;
  size_t numb = sizeof(unsigned long);
  size_t countp, calc_size;
  int i;
 
  calc_size = (mpz_sizeinbase(REPBNUM(n),  2) + numb-1) / numb;
  rop = (unsigned long *)malloc(calc_size * numb);
  mpz_export(rop, &countp, 1, numb, 0, 0, REPBNUM(n));
  for (i=0; i<countp; i++)
    arc_hash_update(s, rop[i]);
  free(rop);
  return((unsigned long)countp);
}

static value bignum_iscmp(arc *c, value v1, value v2)
{
  return((mpz_cmp(REPBNUM(v1), REPBNUM(v2)) == 0) ?
	 CTRUE : CNIL);
}

static value bignum_isocmp(arc *c, value v1, value v2, value vh1, value vh2)
{
  return(bignum_iscmp(c, v1, v2));
}

value arc_mkbignuml(arc *c, long val)
{
  value cv;

  cv = arc_mkobject(c, sizeof(mpz_t), T_BIGNUM);
  mpz_init(REPBNUM(cv));
  mpz_set_si(REPBNUM(cv), val);
  return(cv);
}

typefn_t __arc_bignum_typefn__ = {
  __arc_null_marker,
  bignum_sweep,
  bignum_pprint,
  bignum_hash,
  bignum_iscmp,
  bignum_isocmp
};

#endif
