/* 
  Copyright (C) 2010 Rafael R. Sevilla

  This file is part of Arcueid

  Arcueid is free software; you can redistribute it and/or modify it
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
#include <string.h>
#include <check.h>
#include <math.h>
#include <stdio.h>
#include "../src/arcueid.h"
#include "../src/arith.h"
#include "../config.h"

arc c;

START_TEST(test_add_fixnum)
{
  int i;
  value v = INT2FIX(0);

  for (i=1; i<=100; i++)
    v = __arc_add2(&c, v, INT2FIX(i));
  fail_unless(TYPE(v) == T_FIXNUM);
  fail_unless(FIX2INT(v) == 5050);
  
}
END_TEST

START_TEST(test_add_fixnum2bignum)
{
#ifdef HAVE_GMP_H
  value maxfixnum, one, negone, sum;

  maxfixnum = INT2FIX(FIXNUM_MAX);
  one = INT2FIX(1);
  negone = INT2FIX(-1);
  sum = __arc_add2(&c, maxfixnum, one);
  fail_unless(TYPE(sum) == T_BIGNUM);
  fail_unless(mpz_get_si(REP(sum)._bignum) == FIXNUM_MAX + 1);

  sum = __arc_add2(&c, negone, sum);
  fail_unless(TYPE(sum) == T_FIXNUM);
  fail_unless(FIX2INT(sum) == FIXNUM_MAX);
#endif
}
END_TEST

START_TEST(test_add_fixnum2flonum)
{
  value val1, val2, sum;

  val1 = INT2FIX(1);
  val2 = arc_mkflonum(&c, 3.14159);

  sum = __arc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_FLONUM);
  fail_unless(fabs(4.14159 - REP(sum)._flonum) < 1e-6);

  val1 = INT2FIX(-1);
  sum = __arc_add2(&c, sum, val1);
  fail_unless(TYPE(sum) == T_FLONUM);
  fail_unless(fabs(3.14159 - REP(sum)._flonum) < 1e-6);
}
END_TEST

START_TEST(test_add_fixnum2complex)
{
  value val1, val2, sum;

  val1 = INT2FIX(1);
  val2 = arc_mkcomplex(&c, 1.1, 2.2);

  sum = __arc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_COMPLEX);
  fail_unless(fabs(2.1 - REP(sum)._complex.re) < 1e-6);
  fail_unless(fabs(2.2 - REP(sum)._complex.im) < 1e-6);

  val1 = INT2FIX(-1);
  sum = __arc_add2(&c, sum, val1);
  fail_unless(TYPE(sum) == T_COMPLEX);
  fail_unless(fabs(1.1 - REP(sum)._complex.re) < 1e-6);
  fail_unless(fabs(2.2 - REP(sum)._complex.im) < 1e-6);
}
END_TEST

START_TEST(test_add_fixnum2rational)
{
#ifdef HAVE_GMP_H
  value v1, v2, sum;

  v1 = arc_mkrationall(&c, 1, 2);
  v2 = INT2FIX(1);
  sum = __arc_add2(&c, v1, v2);
  fail_unless(TYPE(sum) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REP(sum)._rational, 3, 2) == 0);

  v1 = INT2FIX(1);
  v2 = arc_mkrationall(&c, 1, 2);
  sum = __arc_add2(&c, v1, v2);
  fail_unless(TYPE(sum) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REP(sum)._rational, 3, 2) == 0);

#endif
}
END_TEST

START_TEST(test_add_flonum)
{
  value val1, val2, sum;

  val1 = arc_mkflonum(&c, 2.71828);
  val2 = arc_mkflonum(&c, 3.14159);

  sum = __arc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_FLONUM);
  fail_unless(fabs(5.85987 - REP(sum)._flonum) < 1e-6);
}
END_TEST

START_TEST(test_add_complex)
{
  value val1, val2, sum;

  val1 = arc_mkcomplex(&c, 1, -2);
  val2 = arc_mkcomplex(&c, 3, -4);

  sum = __arc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_COMPLEX);
  fail_unless(fabs(4 - REP(sum)._complex.re) < 1e-6);
  fail_unless(fabs(-6 - REP(sum)._complex.im) < 1e-6);
}
END_TEST

START_TEST(test_add_rational2complex)
{
#ifdef HAVE_GMP_H
  value val1, val2, sum;

  val1 = arc_mkcomplex(&c, 0.5, 0.5);
  val2 = arc_mkrationall(&c, 1, 2);
  sum = __arc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_COMPLEX);
  fail_unless(fabs(1.0 - REP(sum)._complex.re) < 1e-6);
  fail_unless(fabs(0.5 - REP(sum)._complex.im) < 1e-6);

  val1 = arc_mkrationall(&c, 1, 2);
  val2 = arc_mkcomplex(&c, 0.5, 0.5);
  sum = __arc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_COMPLEX);
  fail_unless(fabs(1.0 - REP(sum)._complex.re) < 1e-6);
  fail_unless(fabs(0.5 - REP(sum)._complex.im) < 1e-6);
#endif
}
END_TEST

START_TEST(test_add_flonum2rational)
{
#ifdef HAVE_GMP_H
  value val1, val2, sum;

  val1 = arc_mkflonum(&c, 0.5);
  val2 = arc_mkrationall(&c, 1, 2);
  sum = __arc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_FLONUM);
  fail_unless(fabs(1.0 - REP(sum)._flonum) < 1e-6);

  val1 = arc_mkrationall(&c, 1, 2);
  val2 = arc_mkflonum(&c, 0.5);
  sum = __arc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_FLONUM);
  fail_unless(fabs(1.0 - REP(sum)._flonum) < 1e-6);
#endif
}
END_TEST

static int error = 0;

static void signal_error_test(struct arc *c, const char *fmt, ...)
{
  error = 1;
}

START_TEST(test_add_bignum)
{
#ifdef HAVE_GMP_H
  value val1, val2, sum;
  mpz_t expected;

  val1 = arc_mkbignuml(&c, FIXNUM_MAX+1);
  val2 = arc_mkbignuml(&c, -(FIXNUM_MAX+2));
  sum = __arc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_FIXNUM);
  fail_unless(FIX2INT(sum) == -1);

  val1 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(val1)._bignum, "100000000000000000000000000000", 10);
  val2 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(val2)._bignum, "200000000000000000000000000000", 10);
  sum = __arc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_BIGNUM);
  mpz_init(expected);
  mpz_set_str(expected, "300000000000000000000000000000", 10);
  fail_unless(mpz_cmp(expected, REP(sum)._bignum) == 0);
  mpz_clear(expected);
#endif
}
END_TEST

START_TEST(test_add_bignum2flonum)
{
#ifdef HAVE_GMP_H
  value v1, v2, sum;

  v1 = arc_mkflonum(&c, 0.0);
  v2 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v2)._bignum, "10000000000000000000000", 10);
  sum = __arc_add2(&c, v1, v2);
  fail_unless(TYPE(sum) == T_FLONUM);
  fail_unless(fabs(REP(sum)._flonum - 1e22) < 1e-6);

  v1 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v1)._bignum, "10000000000000000000000", 10);
  v2 = arc_mkflonum(&c, 0.0);
  sum = __arc_add2(&c, v1, v2);
  fail_unless(TYPE(sum) == T_FLONUM);
  fail_unless(fabs(REP(sum)._flonum - 1e22) < 1e-6);
#endif
}
END_TEST

START_TEST(test_add_bignum2complex)
{
#ifdef HAVE_GMP_H
  value v1, v2, sum;

  v1 = arc_mkcomplex(&c, 0.0, 1.1);
  v2 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v2)._bignum, "10000000000000000000000", 10);
  sum = __arc_add2(&c, v1, v2);
  fail_unless(TYPE(sum) == T_COMPLEX);
  fail_unless(fabs(REP(sum)._complex.re - 1e22) < 1e-6);
  fail_unless(fabs(REP(sum)._complex.im - 1.1) < 1e-6);

  v1 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v1)._bignum, "10000000000000000000000", 10);
  v2 = arc_mkcomplex(&c, 0.0, 1.1);
  sum = __arc_add2(&c, v1, v2);
  fail_unless(TYPE(sum) == T_COMPLEX);
  fail_unless(fabs(REP(sum)._complex.re - 1e22) < 1e-6);
  fail_unless(fabs(REP(sum)._complex.im - 1.1) < 1e-6);
#endif
}
END_TEST

START_TEST(test_add_bignum2rational)
{
#ifdef HAVE_GMP_H
  value v1, v2, sum;
  mpq_t expected;

  v1 = arc_mkrationall(&c, 1, 2);
  v2 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v2)._bignum, "100000000000000000000000000000", 10);
  sum = __arc_add2(&c, v1, v2);
  fail_unless(TYPE(sum) == T_RATIONAL);
  mpq_init(expected);
  mpq_set_str(expected, "200000000000000000000000000001/2", 10);
  fail_unless(mpq_cmp(expected, REP(sum)._rational) == 0);
  mpq_clear(expected);

  v1 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v1)._bignum, "100000000000000000000000000000", 10);
  v2 = arc_mkrationall(&c, 1, 2);
  sum = __arc_add2(&c, v1, v2);
  fail_unless(TYPE(sum) == T_RATIONAL);
  mpq_init(expected);
  mpq_set_str(expected, "200000000000000000000000000001/2", 10);
  fail_unless(mpq_cmp(expected, REP(sum)._rational) == 0);
  mpq_clear(expected);
#endif
}
END_TEST

START_TEST(test_mul_bignum2complex)
{
#ifdef HAVE_GMP_H
  value v1, v2, prod;

  v1 = arc_mkcomplex(&c, 1.0, 1.0);
  v2 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v2)._bignum, "10000000000000000000000", 10);
  prod = __arc_mul2(&c, v1, v2);
  fail_unless(TYPE(prod) == T_COMPLEX);
  fail_unless(fabs(REP(prod)._complex.re - 1e22) < 1e-6);
  fail_unless(fabs(REP(prod)._complex.im - 1e22) < 1e-6);

  v1 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v1)._bignum, "10000000000000000000000", 10);
  v2 = arc_mkcomplex(&c, 1.0, 1.0);
  prod = __arc_mul2(&c, v1, v2);
  fail_unless(TYPE(prod) == T_COMPLEX);
  fail_unless(fabs(REP(prod)._complex.re - 1e22) < 1e-6);
  fail_unless(fabs(REP(prod)._complex.im - 1e22) < 1e-6);
#endif
}
END_TEST

START_TEST(test_add_rational)
{
#ifdef HAVE_GMP_H
  value val1, val2, sum;
  mpz_t expected;

  val1 = arc_mkrationall(&c, 1, 2);
  val2 = arc_mkrationall(&c, 1, 4);
  sum = __arc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REP(sum)._rational, 3, 4) == 0);

  val1 = sum;
  sum = __arc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_FIXNUM);
  fail_unless(FIX2INT(sum) == 1);

  val1 = arc_mkrationall(&c, 0, 1);
  mpq_set_str(REP(val1)._rational, "1606938044258990275541962092341162602522202993782792835301375/4", 10);
  val2 = arc_mkrationall(&c, 0, 1);
  mpq_set_str(REP(val2)._rational, "1/4", 10);
  sum = __arc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_BIGNUM);
  mpz_init(expected);
  mpz_set_str(expected, "401734511064747568885490523085290650630550748445698208825344", 10);
  fail_unless(mpz_cmp(expected, REP(sum)._bignum) == 0);
  mpz_clear(expected);
#endif

}
END_TEST

START_TEST(test_add_flonum2complex)
{
  value val1, val2, sum;

  val1 = arc_mkflonum(&c, 0.5);
  val2 = arc_mkcomplex(&c, 3, -4);
  sum = __arc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_COMPLEX);
  fail_unless(fabs(3.5 - REP(sum)._complex.re) < 1e-6);
  fail_unless(fabs(-4 - REP(sum)._complex.im) < 1e-6);

  val1 = arc_mkcomplex(&c, 3, -4);
  val2 = arc_mkflonum(&c, 0.5);
  sum = __arc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_COMPLEX);
  fail_unless(fabs(3.5 - REP(sum)._complex.re) < 1e-6);
  fail_unless(fabs(-4 - REP(sum)._complex.im) < 1e-6);

}
END_TEST

START_TEST(test_add_misc)
{
  value sum;

  error = 0;

  sum = __arc_add2(&c, CNIL, CNIL);
  fail_unless(error == 1);
  error = 0;

  sum = __arc_add2(&c, cons(&c, FIX2INT(1), CNIL), 
		    cons(&c, FIX2INT(2), CNIL));
}
END_TEST

START_TEST(test_mul_fixnum)
{
  value prod;
#ifdef HAVE_GMP_H
  mpz_t expected;
#endif

  error = 0;

  prod = __arc_mul2(&c, INT2FIX(8), INT2FIX(21));
  fail_unless(TYPE(prod) == T_FIXNUM);
  fail_unless(FIX2INT(prod) == 168);

  prod = __arc_mul2(&c, INT2FIX(-8), INT2FIX(-21));
  fail_unless(TYPE(prod) == T_FIXNUM);
  fail_unless(FIX2INT(prod) == 168);

  prod = __arc_mul2(&c, INT2FIX(-8), INT2FIX(21));
  fail_unless(TYPE(prod) == T_FIXNUM);
  fail_unless(FIX2INT(prod) == -168);

  prod = __arc_mul2(&c, INT2FIX(8), INT2FIX(-21));
  fail_unless(TYPE(prod) == T_FIXNUM);
  fail_unless(FIX2INT(prod) == -168);

#ifdef HAVE_GMP_H
  prod = __arc_mul2(&c, INT2FIX(2), INT2FIX(FIXNUM_MAX));
  fail_unless(TYPE(prod) == T_BIGNUM);
  mpz_init(expected);
  mpz_set_si(expected, 2*FIXNUM_MAX);
  fail_unless(mpz_cmp(expected, REP(prod)._bignum) == 0);

  prod = __arc_mul2(&c, INT2FIX(-2), INT2FIX(-FIXNUM_MAX));
  fail_unless(TYPE(prod) == T_BIGNUM);
  mpz_set_si(expected, 2*FIXNUM_MAX);
  fail_unless(mpz_cmp(expected, REP(prod)._bignum) == 0);

  prod = __arc_mul2(&c, INT2FIX(-2), INT2FIX(FIXNUM_MAX));
  fail_unless(TYPE(prod) == T_BIGNUM);
  mpz_set_si(expected, -2*FIXNUM_MAX);
  fail_unless(mpz_cmp(expected, REP(prod)._bignum) == 0);

  prod = __arc_mul2(&c, INT2FIX(2), INT2FIX(-FIXNUM_MAX));
  fail_unless(TYPE(prod) == T_BIGNUM);
  mpz_set_si(expected, -2*FIXNUM_MAX);
  fail_unless(mpz_cmp(expected, REP(prod)._bignum) == 0);
  mpz_clear(expected);
#else
  prod = __arc_mul2(&c, INT2FIX(2), INT2FIX(FIXNUM_MAX));
  fail_unless(TYPE(prod) == T_NIL);
  fail_unless(error == 1);
#endif

}
END_TEST

START_TEST(test_mul_bignum)
{
#ifdef HAVE_GMP_H
  value val1, val2, sum;
  mpz_t expected;

  val1 = arc_mkbignuml(&c, FIXNUM_MAX+1);
  val2 = arc_mkbignuml(&c, -(FIXNUM_MAX+2));
  sum = __arc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_FIXNUM);
  fail_unless(FIX2INT(sum) == -1);

  val1 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(val1)._bignum, "400000000000000000000000000000", 10);
  val2 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(val2)._bignum, "300000000000000000000000000000", 10);
  sum = __arc_mul2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_BIGNUM);
  mpz_init(expected);
  mpz_set_str(expected, "120000000000000000000000000000000000000000000000000000000000", 10);
  fail_unless(mpz_cmp(expected, REP(sum)._bignum) == 0);
  mpz_clear(expected);
#endif
}
END_TEST

START_TEST(test_mul_flonum)
{
  value val1, val2, prod;

  val1 = arc_mkflonum(&c, 1.20257);
  val2 = arc_mkflonum(&c, 0.57721);

  prod = __arc_mul2(&c, val1, val2);
  fail_unless(TYPE(prod) == T_FLONUM);
  fail_unless(fabs(0.694135 - REP(prod)._flonum) < 1e-6);
}
END_TEST

START_TEST(test_mul_complex)
{
  value val1, val2, prod;

  val1 = arc_mkcomplex(&c, 2.0, 1.0);
  val2 = arc_mkcomplex(&c, 3.0, 2.0);

  prod = __arc_mul2(&c, val1, val2);
  fail_unless(TYPE(prod) == T_COMPLEX);
  fail_unless(fabs(4.0 - REP(prod)._complex.re) < 1e-6);
  fail_unless(fabs(7.0 - REP(prod)._complex.im) < 1e-6);
}
END_TEST

START_TEST(test_mul_rational)
{
#ifdef HAVE_GMP_H
  value val1, val2, prod;
  mpz_t expected;

  val1 = arc_mkrationall(&c, 1, 2);
  val2 = arc_mkrationall(&c, 1, 4);
  prod = __arc_mul2(&c, val1, val2);
  fail_unless(TYPE(prod) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REP(prod)._rational, 1, 8) == 0);

  val1 = arc_mkrationall(&c, 3, 4);
  val2 = arc_mkrationall(&c, 4, 3);
  prod = __arc_mul2(&c, val1, val2);
  fail_unless(TYPE(prod) == T_FIXNUM);
  fail_unless(FIX2INT(prod) == 1);

  val1 = arc_mkrationall(&c, 3, 4);
  val2 = arc_mkrationall(&c, 4, 3);
  prod = __arc_mul2(&c, val1, val2);
  fail_unless(TYPE(prod) == T_FIXNUM);
  fail_unless(FIX2INT(prod) == 1);

  val1 = arc_mkrationall(&c, 0, 1);
  mpq_set_str(REP(val1)._rational, "115792089237316195423570985008687907853269984665640564039457584007913129639936/3", 10);
  val2 = arc_mkrationall(&c, 3, 4);
  prod = __arc_mul2(&c, val1, val2);
  fail_unless(TYPE(prod) == T_BIGNUM);
  mpz_init(expected);
  mpz_set_str(expected, "28948022309329048855892746252171976963317496166410141009864396001978282409984", 10);
  fail_unless(mpz_cmp(expected, REP(prod)._bignum) == 0);
  mpz_clear(expected);
#endif

}
END_TEST

START_TEST(test_mul_fixnum2bignum)
{
#ifdef HAVE_GMP_H
  value factorial;
  mpz_t expected;
  int i;

  factorial = INT2FIX(1);
  for (i=1; i<=100; i++)
    factorial = __arc_mul2(&c, INT2FIX(i), factorial);

  mpz_init(expected);
  mpz_set_str(expected, "93326215443944152681699238856266700490715968264381621468592963895217599993229915608941463976156518286253697920827223758251185210916864000000000000000000000000", 10);
  fail_unless(mpz_cmp(expected, REP(factorial)._bignum) == 0);
  mpz_clear(expected);
#endif
}
END_TEST

START_TEST(test_mul_misc)
{
  value prod;

  error = 0;

  prod = __arc_mul2(&c, CNIL, CNIL);
  fail_unless(error == 1);
  error = 0;

  prod = __arc_mul2(&c, cons(&c, FIX2INT(1), CNIL), 
		     cons(&c, FIX2INT(2), CNIL));
}
END_TEST

START_TEST(test_mul_fixnum2flonum)
{
  value val1, val2, prod;

  val1 = INT2FIX(2);
  val2 = arc_mkflonum(&c, 3.14159);

  prod = __arc_mul2(&c, val1, val2);
  fail_unless(TYPE(prod) == T_FLONUM);
  fail_unless(fabs(6.28318 - REP(prod)._flonum) < 1e-6);

  val1 = INT2FIX(3);
  prod = __arc_mul2(&c, prod, val1);
  fail_unless(TYPE(prod) == T_FLONUM);
  fail_unless(fabs(18.84954 - REP(prod)._flonum) < 1e-6);
}
END_TEST

START_TEST(test_mul_fixnum2rational)
{
#ifdef HAVE_GMP_H
  value v1, v2, prod;

  v1 = arc_mkrationall(&c, 1, 2);
  v2 = INT2FIX(3);
  prod = __arc_mul2(&c, v1, v2);
  fail_unless(TYPE(prod) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REP(prod)._rational, 3, 2) == 0);

  v1 = INT2FIX(3);
  v2 = arc_mkrationall(&c, 1, 2);
  prod = __arc_mul2(&c, v1, v2);
  fail_unless(TYPE(prod) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REP(prod)._rational, 3, 2) == 0);

  v1 = INT2FIX(2);
  v2 = arc_mkrationall(&c, 1, 2);
  prod = __arc_mul2(&c, v1, v2);
  fail_unless(TYPE(prod) == T_FIXNUM);
  fail_unless(FIX2INT(prod) == 1);
#endif
}
END_TEST

START_TEST(test_mul_fixnum2complex)
{
  value v1, v2, prod;

  v1 = arc_mkcomplex(&c, 1.0, 2.0);
  v2 = INT2FIX(3);
  prod = __arc_mul2(&c, v1, v2);
  fail_unless(TYPE(prod) == T_COMPLEX);
  fail_unless(fabs(3.0 - REP(prod)._complex.re) < 1e-6);
  fail_unless(fabs(6.0 - REP(prod)._complex.im) < 1e-6);

  v1 = INT2FIX(3);
  v1 = arc_mkcomplex(&c, 1.0, 2.0);
  prod = __arc_mul2(&c, v1, v2);
  fail_unless(fabs(3.0 - REP(prod)._complex.re) < 1e-6);
  fail_unless(fabs(6.0 - REP(prod)._complex.im) < 1e-6);
}
END_TEST

START_TEST(test_mul_bignum2flonum)
{
#ifdef HAVE_GMP_H
  value v1, v2, prod;

  v1 = arc_mkflonum(&c, 2.0);
  v2 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v2)._bignum, "100000000000000000000000000000", 10);
  prod = __arc_mul2(&c, v1, v2);
  fail_unless(TYPE(prod) == T_FLONUM);
  fail_unless(fabs(prod - 2e29) > 1e-6);

  v1 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v1)._bignum, "100000000000000000000000000000", 10);
  v2 = arc_mkflonum(&c, 2.0);
  prod = __arc_mul2(&c, v1, v2);
  fail_unless(TYPE(prod) == T_FLONUM);
  fail_unless(fabs(prod - 2e29) > 1e-6);
#endif
}
END_TEST

START_TEST(test_mul_bignum2rational)
{
#ifdef HAVE_GMP_H
  value v1, v2, prod;
  mpq_t expected;
  mpz_t zexpected;

  v1 = arc_mkrationall(&c, 1, 3);
  v2 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v2)._bignum, "100000000000000000000000000000", 10);
  prod = __arc_mul2(&c, v1, v2);
  fail_unless(TYPE(prod) == T_RATIONAL);
  mpq_init(expected);
  mpq_set_str(expected, "100000000000000000000000000000/3", 10);
  fail_unless(mpq_cmp(expected, REP(prod)._rational) == 0);
  mpq_clear(expected);

  v1 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v1)._bignum, "100000000000000000000000000000", 10);
  v2 = arc_mkrationall(&c, 1, 3);
  prod = __arc_mul2(&c, v1, v2);
  fail_unless(TYPE(prod) == T_RATIONAL);
  mpq_init(expected);
  mpq_set_str(expected, "100000000000000000000000000000/3", 10);
  fail_unless(mpq_cmp(expected, REP(prod)._rational) == 0);
  mpq_clear(expected);

  v1 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v1)._bignum, "100000000000000000000000000000", 10);
  v2 = arc_mkrationall(&c, 1, 2);
  prod = __arc_mul2(&c, v1, v2);
  fail_unless(TYPE(prod) == T_BIGNUM);
  mpz_init(zexpected);
  mpz_set_str(zexpected, "50000000000000000000000000000", 10);
  fail_unless(mpz_cmp(zexpected, REP(prod)._bignum) == 0);
  mpz_clear(zexpected);

  v1 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v1)._bignum, "100000000000000000000000000000", 10);
  v2 = arc_mkrationall(&c, 0, 1);
  mpq_set_str(REP(v2)._rational, "1/100000000000000000000000000000", 10);
  prod = __arc_mul2(&c, v1, v2);
  fail_unless(TYPE(prod) == T_FIXNUM);
  fail_unless(FIX2INT(prod) == 1);
#endif
}
END_TEST

START_TEST(test_mul_flonum2rational)
{
#ifdef HAVE_GMP_H
  value val1, val2, prod;

  val1 = arc_mkflonum(&c, 2.0);
  val2 = arc_mkrationall(&c, 1, 2);
  prod = __arc_mul2(&c, val1, val2);
  fail_unless(TYPE(prod) == T_FLONUM);
  fail_unless(fabs(1.0 - REP(prod)._flonum) < 1e-6);

  val1 = arc_mkrationall(&c, 1, 2);
  val2 = arc_mkflonum(&c, 2.0);
  prod = __arc_mul2(&c, val1, val2);
  fail_unless(TYPE(prod) == T_FLONUM);
  fail_unless(fabs(1.0 - REP(prod)._flonum) < 1e-6);
#endif
}
END_TEST

START_TEST(test_mul_flonum2complex)
{
  value val1, val2, prod;

  val1 = arc_mkflonum(&c, 0.5);
  val2 = arc_mkcomplex(&c, 3, -4);
  prod = __arc_mul2(&c, val1, val2);
  fail_unless(TYPE(prod) == T_COMPLEX);
  fail_unless(fabs(1.5 - REP(prod)._complex.re) < 1e-6);
  fail_unless(fabs(-2.0 - REP(prod)._complex.im) < 1e-6);

  val1 = arc_mkcomplex(&c, 3, -4);
  val2 = arc_mkflonum(&c, 0.5);
  prod = __arc_mul2(&c, val1, val2);
  fail_unless(TYPE(prod) == T_COMPLEX);
  fail_unless(fabs(1.5 - REP(prod)._complex.re) < 1e-6);
  fail_unless(fabs(-2.0 - REP(prod)._complex.im) < 1e-6);
}
END_TEST

START_TEST(test_mul_rational2complex)
{
#ifdef HAVE_GMP_H
  value val1, val2, prod;

  val1 = arc_mkcomplex(&c, 0.5, 0.25);
  val2 = arc_mkrationall(&c, 1, 2);
  prod = __arc_mul2(&c, val1, val2);
  fail_unless(TYPE(prod) == T_COMPLEX);
  fail_unless(fabs(0.25 - REP(prod)._complex.re) < 1e-6);
  fail_unless(fabs(0.125 - REP(prod)._complex.im) < 1e-6);

  val1 = arc_mkrationall(&c, 1, 2);
  val2 = arc_mkcomplex(&c, 0.5, 0.25);
  prod = __arc_mul2(&c, val1, val2);
  fail_unless(TYPE(prod) == T_COMPLEX);
  fail_unless(fabs(0.25 - REP(prod)._complex.re) < 1e-6);
  fail_unless(fabs(0.125 - REP(prod)._complex.im) < 1e-6);
#endif
}
END_TEST

START_TEST(test_div_fixnum)
{
  value quot;
#ifdef HAVE_GMP_H
  mpq_t expected;
#endif

  error = 0;

  quot = __arc_div2(&c, INT2FIX(168), INT2FIX(21));
  fail_unless(TYPE(quot) == T_FIXNUM);
  fail_unless(FIX2INT(quot) == 8);

  quot = __arc_div2(&c, INT2FIX(-168), INT2FIX(21));
  fail_unless(TYPE(quot) == T_FIXNUM);
  fail_unless(FIX2INT(quot) == -8);

  quot = __arc_div2(&c, INT2FIX(168), INT2FIX(-21));
  fail_unless(TYPE(quot) == T_FIXNUM);
  fail_unless(FIX2INT(quot) == -8);

  quot = __arc_div2(&c, INT2FIX(-168), INT2FIX(-21));
  fail_unless(TYPE(quot) == T_FIXNUM);
  fail_unless(FIX2INT(quot) == 8);

  quot = __arc_div2(&c, INT2FIX(168), INT2FIX(0));
  fail_unless(TYPE(quot) == T_NIL);
  fail_unless(quot == CNIL);
  fail_unless(error == 1);
  error = 0;

#ifdef HAVE_GMP_H
  quot = __arc_div2(&c, INT2FIX(1), INT2FIX(2));
  fail_unless(TYPE(quot) == T_RATIONAL);
  mpq_init(expected);
  mpq_set_str(expected, "1/2", 10);
  fail_unless(mpq_cmp(expected, REP(quot)._rational) == 0);
  mpq_clear(expected);

  /* This is the only case where a division of two fixnums will
     result in a bignum.  Happens because the fixnum range is
     unsymmetric. */
  quot = __arc_div2(&c, INT2FIX(FIXNUM_MIN), INT2FIX(-1));
  fail_unless(TYPE(quot) == T_BIGNUM);
  fail_unless(mpz_cmp_si(REP(quot)._bignum, -FIXNUM_MIN) == 0);
#endif
}
END_TEST

START_TEST(test_div_bignum)
{
#ifdef HAVE_GMP_H
  value val1, val2, quot;
  mpz_t expected;

  error = 0;

  /* Bignum result */
  val1 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(val1)._bignum, "40000000000000000000000000000000000000000000000", 10);
  val2 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(val2)._bignum, "20000000000000000000000", 10);
  quot = __arc_div2(&c, val1, val2);
  fail_unless(TYPE(quot) == T_BIGNUM);
  mpz_init(expected);
  mpz_set_str(expected, "2000000000000000000000000", 10);
  fail_unless(mpz_cmp(expected, REP(quot)._bignum) == 0);
  mpz_clear(expected);

  /* Fixnum result */
  val1 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(val1)._bignum, "40000000000000000000000000000000000000000000000", 10);
  val2 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(val2)._bignum, "20000000000000000000000000000000000000000000000", 10);
  quot = __arc_div2(&c, val1, val2);
  fail_unless(TYPE(quot) == T_FIXNUM);
  fail_unless(FIX2INT(quot) == 2);

  /* Rational result */
  val1 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(val1)._bignum, "40000000000000000000000000000000000000000000000", 10);
  val2 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(val2)._bignum, "30000000000000000000000000000000000000000000000", 10);
  quot = __arc_div2(&c, val1, val2);
  fail_unless(TYPE(quot) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REP(quot)._rational, 4, 3) == 0);

  /* Division by zero */
  error = 0;
  val1 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(val1)._bignum, "40000000000000000000000000000000000000000000000", 10);
  val2 = arc_mkbignuml(&c, 0);
  quot = __arc_div2(&c, val1, val2);
  fail_unless(TYPE(quot) == T_NIL);
  fail_unless(quot == CNIL);
  fail_unless(error == 1);
  error = 0;
#endif
}
END_TEST

START_TEST(test_div_flonum)
{
  value val1, val2, quot;

  error = 0;

  val1 = arc_mkflonum(&c, 1.20257);
  val2 = arc_mkflonum(&c, 0.57721);

  quot = __arc_div2(&c, val1, val2);
  fail_unless(TYPE(quot) == T_FLONUM);
  fail_unless(fabs(2.0834185 - REP(quot)._flonum) < 1e-6);

  val1 = arc_mkflonum(&c, 1.20257);
  val2 = arc_mkflonum(&c, 0.0);
  quot = __arc_div2(&c, val1, val2);
  fail_unless(TYPE(quot) == T_NIL);
  fail_unless(error == 1);
  error = 0;
}
END_TEST

START_TEST(test_div_rational)
{
#ifdef HAVE_GMP_H
  value val1, val2, quot;
  mpz_t expected;

  error = 0;

  val1 = arc_mkrationall(&c, 1, 2);
  val2 = arc_mkrationall(&c, 1, 3);
  quot = __arc_div2(&c, val1, val2);
  fail_unless(TYPE(quot) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REP(quot)._rational, 3, 2) == 0);

  val1 = arc_mkrationall(&c, 1, 2);
  val2 = arc_mkrationall(&c, 1, 4);
  quot = __arc_div2(&c, val1, val2);
  fail_unless(TYPE(quot) == T_FIXNUM);
  fail_unless(FIX2INT(quot) == 2);

  val1 = arc_mkrationall(&c, 0, 1);
  mpq_set_str(REP(val1)._rational, "115792089237316195423570985008687907853269984665640564039457584007913129639936/3", 10);
  val2 = arc_mkrationall(&c, 4, 3);
  quot = __arc_div2(&c, val1, val2);
  fail_unless(TYPE(quot) == T_BIGNUM);
  mpz_init(expected);
  mpz_set_str(expected, "28948022309329048855892746252171976963317496166410141009864396001978282409984", 10);
  fail_unless(mpz_cmp(expected, REP(quot)._bignum) == 0);
  mpz_clear(expected);

  val1 = arc_mkrationall(&c, 1, 1);
  val2 = arc_mkrationall(&c, 0, 1);
  quot = __arc_div2(&c, val1, val2);
  fail_unless(TYPE(quot) == T_NIL);
  fail_unless(error == 1);
  error = 0;
#endif

}
END_TEST

START_TEST(test_div_complex)
{
  value val1, val2, quot;

  error = 0;

  val1 = arc_mkcomplex(&c, 2.0, 1.0);
  val2 = arc_mkcomplex(&c, 3.0, 2.0);

  quot = __arc_div2(&c, val1, val2);
  fail_unless(TYPE(quot) == T_COMPLEX);
  fail_unless(fabs(0.61538462 - REP(quot)._complex.re) < 1e-6);
  fail_unless(fabs(-0.07692308 - REP(quot)._complex.im) < 1e-6);

  val1 = arc_mkcomplex(&c, 2.0, 1.0);
  val2 = arc_mkcomplex(&c, 0.0, 0.0);

  quot = __arc_div2(&c, val1, val2);
  fail_unless(TYPE(quot) == T_NIL);
  fail_unless(error == 1);
  error = 0;
}
END_TEST

START_TEST(test_div_fixnum2bignum)
{
#ifdef HAVE_GMP_H
  value val1, val2, quot;
  mpz_t expected;
  mpq_t qexpected;
  int i;

  error = 0;

  /* Bignum / Fixnum = Bignum */
  val1 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(val1)._bignum, "40000000000000000000000000000000000000000000000", 10);
  val2 = INT2FIX(2);
  quot = __arc_div2(&c, val1, val2);
  fail_unless(TYPE(quot) == T_BIGNUM);
  mpz_init(expected);
  mpz_set_str(expected, "20000000000000000000000000000000000000000000000", 10);
  fail_unless(mpz_cmp(expected, REP(quot)._bignum) == 0);
  mpz_clear(expected);

  /* Bignum / Fixnum = Fixnum (eventually) */
  val1 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(val1)._bignum, "40000000000000000000000000000000000000000000000", 10);
  val2 = INT2FIX(10);
  for (i=0; i<46; i++)
    val1 = __arc_div2(&c, val1, val2);
  fail_unless(TYPE(val1) == T_FIXNUM);
  fail_unless(FIX2INT(val1) == 4);

  /* Bignum / Fixnum = Rational */
  val1 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(val1)._bignum, "40000000000000000000000000000000000000000000000",
	      10);
  val2 = INT2FIX(3);
  quot = __arc_div2(&c, val1, val2);
  fail_unless(TYPE(quot) == T_RATIONAL);
  mpq_init(qexpected);
  mpq_set_str(qexpected, "40000000000000000000000000000000000000000000000/3",
	      10);
  fail_unless(mpq_cmp(qexpected, REP(quot)._rational) == 0);

  /* Fixnum / "Bignum" = Fixnum (somewhat contrived) */
  val1 = INT2FIX(100);
  val2 = arc_mkbignuml(&c, 10);
  quot = __arc_div2(&c, val1, val2);
  fail_unless(TYPE(quot) == T_FIXNUM);
  fail_unless(FIX2INT(quot) == 10);

  /* Fixnum / "Bignum" = Bignum (somewhat contrived).  See test_div_fixnum
     for a similar test */
  val1 = INT2FIX(FIXNUM_MIN);
  val2 = arc_mkbignuml(&c, -1);
  quot = __arc_div2(&c, val1, val2);
  fail_unless(TYPE(quot) == T_BIGNUM);
  fail_unless(mpz_cmp_si(REP(quot)._bignum, -FIXNUM_MIN) == 0);

  /* Fixnum / Bignum = Rational */
  val1 = INT2FIX(3);
  val2 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(val2)._bignum, "40000000000000000000000000000000000000000000000",
	      10);
  quot = __arc_div2(&c, val1, val2);
  fail_unless(TYPE(quot) == T_RATIONAL);
  mpq_init(qexpected);
  mpq_set_str(qexpected, "3/40000000000000000000000000000000000000000000000",
	      10);
  fail_unless(mpq_cmp(qexpected, REP(quot)._rational) == 0);

  /* Fixnum / Bignum division by zero */
  error = 0;
  val1 = INT2FIX(3);
  val2 = arc_mkbignuml(&c, 0);
  quot = __arc_div2(&c, val1, val2);
  fail_unless(TYPE(quot) == T_NIL);
  fail_unless(error == 1);
  error = 0;

#endif
}
END_TEST

START_TEST(test_div_fixnum2flonum)
{
  value val1, val2, quot;

  error = 0;

  val1 = INT2FIX(2);
  val2 = arc_mkflonum(&c, 3.14159);
  quot = __arc_div2(&c, val1, val2);
  fail_unless(TYPE(quot) == T_FLONUM);
  fail_unless(fabs(0.63662031 - REP(quot)._flonum) < 1e-6);

  val1 = INT2FIX(2);
  quot = __arc_div2(&c, quot, val1);
  fail_unless(TYPE(quot) == T_FLONUM);
  fail_unless(fabs(0.3183101 - REP(quot)._flonum) < 1e-6);

  /* Division by zero checks */
  error = 0;
  val1 = INT2FIX(2);
  val2 = arc_mkflonum(&c, 0.0);
  quot = __arc_div2(&c, val1, val2);
  fail_unless(TYPE(quot) == T_NIL);
  fail_unless(error == 1);
  error = 0;

  val1 = arc_mkflonum(&c, 1.0);
  val2 = INT2FIX(0);
  quot = __arc_div2(&c, val1, val2);
  fail_unless(TYPE(quot) == T_NIL);
  fail_unless(error == 1);
  error = 0;

}
END_TEST

START_TEST(test_div_fixnum2rational)
{
#ifdef HAVE_GMP_H
  value v1, v2, quot;

  error = 0;

  v1 = arc_mkrationall(&c, 1, 2);
  v2 = INT2FIX(3);
  quot = __arc_div2(&c, v1, v2);
  fail_unless(TYPE(quot) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REP(quot)._rational, 1, 6) == 0);

  v1 = INT2FIX(3);
  v2 = arc_mkrationall(&c, 5, 2);
  quot = __arc_div2(&c, v1, v2);
  fail_unless(TYPE(quot) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REP(quot)._rational, 6, 5) == 0);

  v1 = INT2FIX(3);
  v2 = arc_mkrationall(&c, 3, 2);
  quot = __arc_div2(&c, v1, v2);
  fail_unless(TYPE(quot) == T_FIXNUM);
  fail_unless(FIX2INT(quot) == 2);

  /* Division by zero */
  error = 0;
  v1 = arc_mkrationall(&c, 1, 2);
  v2 = INT2FIX(0);
  quot = __arc_div2(&c, v1, v2);
  fail_unless(TYPE(quot) == T_NIL);
  fail_unless(error == 1);
  error = 0;

  v1 = INT2FIX(1);
  v2 = arc_mkrationall(&c, 0, 1);
  quot = __arc_div2(&c, v1, v2);
  fail_unless(TYPE(quot) == T_NIL);
  fail_unless(error == 1);
  error = 0;

#endif
}
END_TEST

START_TEST(test_div_fixnum2complex)
{
  value v1, v2, quot;

  error = 0;

  v1 = arc_mkcomplex(&c, 1.0, 2.0);
  v2 = INT2FIX(4);
  quot = __arc_div2(&c, v1, v2);
  fail_unless(TYPE(quot) == T_COMPLEX);
  fail_unless(fabs(0.25 - REP(quot)._complex.re) < 1e-6);
  fail_unless(fabs(0.50 - REP(quot)._complex.im) < 1e-6);

  v1 = INT2FIX(2);
  v2 = arc_mkcomplex(&c, 1.0, 2.0);
  quot = __arc_div2(&c, v1, v2);
  fail_unless(fabs(0.4 - REP(quot)._complex.re) < 1e-6);
  fail_unless(fabs(-0.8 - REP(quot)._complex.im) < 1e-6);

  /* Division by zero */
  error = 0;
  v1 = INT2FIX(2);
  v2 = arc_mkcomplex(&c, 0.0, 0.0);
  quot = __arc_div2(&c, v1, v2);
  fail_unless(TYPE(quot) == T_NIL);
  fail_unless(error == 1);
  error = 0;

  v1 = arc_mkcomplex(&c, 0.0, 0.0);
  v2 = INT2FIX(0);
  quot = __arc_div2(&c, v1, v2);
  fail_unless(TYPE(quot) == T_NIL);
  fail_unless(error == 1);
  error = 0;

}
END_TEST

START_TEST(test_div_misc)
{
  value quot;

  error = 0;

  quot = __arc_div2(&c, CNIL, CNIL);
  fail_unless(error == 1);
  error = 0;

  quot = __arc_div2(&c, cons(&c, FIX2INT(1), CNIL), 
		     cons(&c, FIX2INT(2), CNIL));
  fail_unless(error == 1);
  error = 0;
}
END_TEST

START_TEST(test_mod)
{
  value m;
#ifdef HAVE_GMP_H
  value v1, v2;
  mpz_t expected;
#endif

  m = __arc_mod2(&c, INT2FIX(14), INT2FIX(12));
  fail_unless(TYPE(m) == T_FIXNUM);
  fail_unless(m == INT2FIX(2));

  m = __arc_mod2(&c, INT2FIX(-14), INT2FIX(12));
  fail_unless(TYPE(m) == T_FIXNUM);
  fail_unless(m == INT2FIX(10));

  m = __arc_mod2(&c, INT2FIX(14), INT2FIX(-12));
  fail_unless(TYPE(m) == T_FIXNUM);
  fail_unless(m == INT2FIX(-10));

  m = __arc_mod2(&c, INT2FIX(-14), INT2FIX(-12));
  fail_unless(TYPE(m) == T_FIXNUM);
  fail_unless(m == INT2FIX(-2));


  m = __arc_mod2(&c, INT2FIX(89), INT2FIX(17));
  fail_unless(TYPE(m) == T_FIXNUM);
  fail_unless(m == INT2FIX(4));

  m = __arc_mod2(&c, INT2FIX(-89), INT2FIX(17));
  fail_unless(TYPE(m) == T_FIXNUM);
  fail_unless(m == INT2FIX(13));

  m = __arc_mod2(&c, INT2FIX(89), INT2FIX(-17));
  fail_unless(TYPE(m) == T_FIXNUM);
  fail_unless(m == INT2FIX(-13));

  m = __arc_mod2(&c, INT2FIX(-89), INT2FIX(-17));
  fail_unless(TYPE(m) == T_FIXNUM);
  fail_unless(m == INT2FIX(-4));

#ifdef HAVE_GMP_H
  v1 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v1)._bignum, "47469399417712610569126236648663416022569604314900505405835385663651109539391", 10);
  v2 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v2)._bignum, "337934473528578712729202", 10);
  mpz_init(expected);
  mpz_set_str(expected, "292503905893903802678877", 10);
  m = __arc_mod2(&c, v1, v2);
  fail_unless(TYPE(m) == T_BIGNUM);
  fail_unless(mpz_cmp(expected, REP(m)._bignum) == 0);

  mpz_set_str(REP(v1)._bignum, "-47469399417712610569126236648663416022569604314900505405835385663651109539391", 10);
  mpz_set_str(REP(v2)._bignum, "337934473528578712729202", 10);
  mpz_set_str(expected, "45430567634674910050325", 10);
  m = __arc_mod2(&c, v1, v2);
  fail_unless(TYPE(m) == T_BIGNUM);
  fail_unless(mpz_cmp(expected, REP(m)._bignum) == 0);

  mpz_set_str(REP(v1)._bignum, "47469399417712610569126236648663416022569604314900505405835385663651109539391", 10);
  mpz_set_str(REP(v2)._bignum, "-337934473528578712729202", 10);
  mpz_init(expected);
  mpz_set_str(expected, "-45430567634674910050325", 10);
  m = __arc_mod2(&c, v1, v2);
  fail_unless(TYPE(m) == T_BIGNUM);
  fail_unless(mpz_cmp(expected, REP(m)._bignum) == 0);

  mpz_clear(expected);

  mpz_set_str(REP(v1)._bignum, "47469399417712610569126236648663416022569604314900505405835385663651109539391", 10);
  v2 = INT2FIX(948);
  m = __arc_mod2(&c, v1, v2);
  fail_unless(TYPE(m) == T_FIXNUM);
  fail_unless(m == INT2FIX(867));

  mpz_set_str(REP(v1)._bignum, "-47469399417712610569126236648663416022569604314900505405835385663651109539391", 10);
  v2 = INT2FIX(948);
  m = __arc_mod2(&c, v1, v2);
  fail_unless(TYPE(m) == T_FIXNUM);
  fail_unless(m == INT2FIX(81));

  mpz_set_str(REP(v1)._bignum, "47469399417712610569126236648663416022569604314900505405835385663651109539391", 10);
  v2 = INT2FIX(-948);
  m = __arc_mod2(&c, v1, v2);
  fail_unless(TYPE(m) == T_FIXNUM);
  fail_unless(m == INT2FIX(-81));

  mpz_set_str(REP(v1)._bignum, "-47469399417712610569126236648663416022569604314900505405835385663651109539391", 10);
  v2 = INT2FIX(-948);
  m = __arc_mod2(&c, v1, v2);
  fail_unless(TYPE(m) == T_FIXNUM);
  fail_unless(m == INT2FIX(-867));

#endif
}
END_TEST

START_TEST(test_sub_fixnum)
{
  int i;
  value v = INT2FIX(0);

  for (i=1; i<=100; i++)
    v = __arc_sub2(&c, v, INT2FIX(i));
  fail_unless(TYPE(v) == T_FIXNUM);
  fail_unless(FIX2INT(v) == -5050);
  
}
END_TEST

START_TEST(test_sub_bignum)
{
#ifdef HAVE_GMP_H
  value val1, val2, diff;
  mpz_t expected;

  val1 = arc_mkbignuml(&c, FIXNUM_MAX+1);
  val2 = arc_mkbignuml(&c, FIXNUM_MAX+2);
  diff = __arc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_FIXNUM);
  fail_unless(FIX2INT(diff) == -1);

  val1 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(val1)._bignum, "300000000000000000000000000000", 10);
  val2 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(val2)._bignum, "100000000000000000000000000000", 10);
  diff = __arc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_BIGNUM);
  mpz_init(expected);
  mpz_set_str(expected, "200000000000000000000000000000", 10);
  fail_unless(mpz_cmp(expected, REP(diff)._bignum) == 0);
  mpz_clear(expected);
#endif
}
END_TEST

START_TEST(test_sub_flonum)
{
  value val1, val2, diff;

  val1 = arc_mkflonum(&c, 2.71828);
  val2 = arc_mkflonum(&c, 3.14159);

  diff = __arc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_FLONUM);
  fail_unless(fabs(-0.42331 - REP(diff)._flonum) < 1e-6);
}
END_TEST

START_TEST(test_sub_rational)
{
#ifdef HAVE_GMP_H
  value val1, val2, diff;
  mpz_t expected;

  val1 = arc_mkrationall(&c, 1, 2);
  val2 = arc_mkrationall(&c, 1, 4);
  diff = __arc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REP(diff)._rational, 1, 4) == 0);

  val1 = diff;
  diff = __arc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_FIXNUM);
  fail_unless(FIX2INT(diff) == 0);

  val1 = arc_mkrationall(&c, 0, 1);
  mpq_set_str(REP(val1)._rational, "1606938044258990275541962092341162602522202993782792835301375/4", 10);
  val2 = arc_mkrationall(&c, 0, 1);
  mpq_set_str(REP(val2)._rational, "3/4", 10);
  diff = __arc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_BIGNUM);
  mpz_init(expected);
  mpz_set_str(expected, "401734511064747568885490523085290650630550748445698208825343", 10);
  fail_unless(mpz_cmp(expected, REP(diff)._bignum) == 0);
  mpz_clear(expected);
#endif

}
END_TEST

START_TEST(test_sub_complex)
{
  value val1, val2, diff;

  val1 = arc_mkcomplex(&c, 1, -2);
  val2 = arc_mkcomplex(&c, 3, -4);

  diff = __arc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_COMPLEX);
  fail_unless(fabs(-2 - REP(diff)._complex.re) < 1e-6);
  fail_unless(fabs(2 - REP(diff)._complex.im) < 1e-6);
}
END_TEST

START_TEST(test_sub_fixnum2bignum)
{
#ifdef HAVE_GMP_H
  value maxfixnum, one, negone, diff;

  maxfixnum = INT2FIX(-FIXNUM_MAX);
  one = INT2FIX(1);
  negone = INT2FIX(-1);
  diff = __arc_sub2(&c, maxfixnum, one);
  fail_unless(TYPE(diff) == T_BIGNUM);
  fail_unless(mpz_get_si(REP(diff)._bignum) == -FIXNUM_MAX - 1);

  diff = __arc_sub2(&c, negone, diff);
  fail_unless(TYPE(diff) == T_FIXNUM);
  fail_unless(FIX2INT(diff) == FIXNUM_MAX);
#endif
}
END_TEST

START_TEST(test_sub_fixnum2flonum)
{
  value val1, val2, diff;

  val1 = INT2FIX(1);
  val2 = arc_mkflonum(&c, 3.14159);

  diff = __arc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_FLONUM);
  fail_unless(fabs(-2.14159 - REP(diff)._flonum) < 1e-6);

  val1 = INT2FIX(-1);
  diff = __arc_sub2(&c, diff, val1);
  fail_unless(TYPE(diff) == T_FLONUM);
  fail_unless(fabs(-1.14159 - REP(diff)._flonum) < 1e-6);
}
END_TEST

START_TEST(test_sub_fixnum2rational)
{
#ifdef HAVE_GMP_H
  value v1, v2, diff;

  v1 = arc_mkrationall(&c, 1, 2);
  v2 = INT2FIX(1);
  diff = __arc_sub2(&c, v1, v2);
  fail_unless(TYPE(diff) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REP(diff)._rational, -1, 2) == 0);

  v1 = INT2FIX(1);
  v2 = arc_mkrationall(&c, 1, 2);
  diff = __arc_sub2(&c, v1, v2);
  fail_unless(TYPE(diff) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REP(diff)._rational, 1, 2) == 0);
#endif
}
END_TEST

START_TEST(test_sub_fixnum2complex)
{
  value val1, val2, diff;

  val1 = INT2FIX(1);
  val2 = arc_mkcomplex(&c, 1.1, 2.2);

  diff = __arc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_COMPLEX);
  fail_unless(fabs(-0.1 - REP(diff)._complex.re) < 1e-6);
  fail_unless(fabs(-2.2 - REP(diff)._complex.im) < 1e-6);

  val1 = INT2FIX(-1);
  diff = __arc_sub2(&c, diff, val1);
  fail_unless(TYPE(diff) == T_COMPLEX);
  fail_unless(fabs(0.9 - REP(diff)._complex.re) < 1e-6);
  fail_unless(fabs(-2.2 - REP(diff)._complex.im) < 1e-6);
}
END_TEST

START_TEST(test_sub_bignum2flonum)
{
#ifdef HAVE_GMP_H
  value v1, v2, diff;

  v1 = arc_mkflonum(&c, 0.0);
  v2 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v2)._bignum, "10000000000000000000000", 10);
  diff = __arc_sub2(&c, v1, v2);
  fail_unless(TYPE(diff) == T_FLONUM);
  fail_unless(fabs(REP(diff)._flonum + 1e22) < 1e-6);

  v1 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v1)._bignum, "10000000000000000000000", 10);
  v2 = arc_mkflonum(&c, 0.0);
  diff = __arc_sub2(&c, v1, v2);
  fail_unless(TYPE(diff) == T_FLONUM);
  fail_unless(fabs(REP(diff)._flonum - 1e22) < 1e-6);
#endif
}
END_TEST

START_TEST(test_sub_bignum2rational)
{
#ifdef HAVE_GMP_H
  value v1, v2, diff;
  mpq_t expected;

  v1 = arc_mkrationall(&c, 1, 2);
  v2 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v2)._bignum, "100000000000000000000000000000", 10);
  diff = __arc_sub2(&c, v1, v2);
  fail_unless(TYPE(diff) == T_RATIONAL);
  mpq_init(expected);
  mpq_set_str(expected, "-199999999999999999999999999999/2", 10);
  fail_unless(mpq_cmp(expected, REP(diff)._rational) == 0);
  mpq_clear(expected);

  v1 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v1)._bignum, "100000000000000000000000000000", 10);
  v2 = arc_mkrationall(&c, 1, 2);
  diff = __arc_sub2(&c, v1, v2);
  fail_unless(TYPE(diff) == T_RATIONAL);
  mpq_init(expected);
  mpq_set_str(expected, "199999999999999999999999999999/2", 10);
  fail_unless(mpq_cmp(expected, REP(diff)._rational) == 0);
  mpq_clear(expected);
#endif
}
END_TEST

START_TEST(test_sub_bignum2complex)
{
#ifdef HAVE_GMP_H
  value v1, v2, diff;

  v1 = arc_mkcomplex(&c, 0.0, 1.1);
  v2 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v2)._bignum, "10000000000000000000000", 10);
  diff = __arc_sub2(&c, v1, v2);
  fail_unless(TYPE(diff) == T_COMPLEX);
  fail_unless(fabs(REP(diff)._complex.re + 1e22) < 1e-6);
  fail_unless(fabs(REP(diff)._complex.im - 1.1) < 1e-6);

  v1 = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v1)._bignum, "10000000000000000000000", 10);
  v2 = arc_mkcomplex(&c, 0.0, 1.1);
  diff = __arc_sub2(&c, v1, v2);
  fail_unless(TYPE(diff) == T_COMPLEX);
  fail_unless(fabs(REP(diff)._complex.re - 1e22) < 1e-6);
  fail_unless(fabs(REP(diff)._complex.im + 1.1) < 1e-6);
#endif
}
END_TEST

START_TEST(test_sub_flonum2rational)
{
#ifdef HAVE_GMP_H
  value val1, val2, diff;

  val1 = arc_mkflonum(&c, 0.5);
  val2 = arc_mkrationall(&c, 1, 2);
  diff = __arc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_FLONUM);
  fail_unless(fabs(REP(diff)._flonum) < 1e-6);

  val1 = arc_mkrationall(&c, 1, 2);
  val2 = arc_mkflonum(&c, 0.5);
  diff = __arc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_FLONUM);
  fail_unless(fabs(REP(diff)._flonum) < 1e-6);
#endif
}
END_TEST

START_TEST(test_sub_flonum2complex)
{
  value val1, val2, diff;

  val1 = arc_mkflonum(&c, 0.5);
  val2 = arc_mkcomplex(&c, 3, -4);
  diff = __arc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_COMPLEX);
  fail_unless(fabs(-2.5 - REP(diff)._complex.re) < 1e-6);
  fail_unless(fabs(4 - REP(diff)._complex.im) < 1e-6);

  val1 = arc_mkcomplex(&c, 3, -4);
  val2 = arc_mkflonum(&c, 0.5);
  diff = __arc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_COMPLEX);
  fail_unless(fabs(2.5 - REP(diff)._complex.re) < 1e-6);
  fail_unless(fabs(-4 - REP(diff)._complex.im) < 1e-6);

}
END_TEST

START_TEST(test_sub_rational2complex)
{
#ifdef HAVE_GMP_H
  value val1, val2, diff;

  val1 = arc_mkcomplex(&c, 0.5, 0.5);
  val2 = arc_mkrationall(&c, 1, 2);
  diff = __arc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_COMPLEX);
  fail_unless(fabs(REP(diff)._complex.re) < 1e-6);
  fail_unless(fabs(0.5 - REP(diff)._complex.im) < 1e-6);

  val1 = arc_mkrationall(&c, 1, 2);
  val2 = arc_mkcomplex(&c, 0.5, 0.5);
  diff = __arc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_COMPLEX);
  fail_unless(fabs(REP(diff)._complex.re) < 1e-6);
  fail_unless(fabs(-0.5 - REP(diff)._complex.im) < 1e-6);
#endif
}
END_TEST

START_TEST(test_sub_misc)
{
  value diff;

  error = 0;

  diff = __arc_sub2(&c, CNIL, CNIL);
  fail_unless(error == 1);
  error = 0;

  diff = __arc_sub2(&c, cons(&c, FIX2INT(1), CNIL), 
		    cons(&c, FIX2INT(2), CNIL));
  fail_unless(error == 1);
}
END_TEST

START_TEST(test_neg)
{
  value v, neg;
#ifdef HAVE_GMP_H
  mpz_t expected;
#endif

  error = 0;

  v = INT2FIX(1);
  neg = __arc_neg(&c, v);
  fail_unless(TYPE(neg) == T_FIXNUM);
  fail_unless(FIX2INT(neg) == -1);

  v = arc_mkflonum(&c, -1.234);
  neg = __arc_neg(&c, v);
  fail_unless(TYPE(neg) == T_FLONUM);
  fail_unless(fabs(REP(neg)._flonum - 1.234) < 1e-6);

  v = arc_mkcomplex(&c, -1.234, 5.678);
  neg = __arc_neg(&c, v);
  fail_unless(TYPE(neg) == T_COMPLEX);
  fail_unless(fabs(REP(neg)._complex.re - 1.234) < 1e-6);
  fail_unless(fabs(REP(neg)._complex.im + 5.678) < 1e-6);

#ifdef HAVE_GMP_H
  v = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v)._bignum, "100000000000000000000000000000", 10);
  neg = __arc_neg(&c, v);
  mpz_init(expected);
  mpz_set_str(expected, "-100000000000000000000000000000", 10);
  fail_unless(TYPE(neg) == T_BIGNUM);
  fail_unless(mpz_cmp(expected, REP(neg)._bignum) == 0);
  mpz_clear(expected);

  v = arc_mkrationall(&c, 1, 2);
  neg = __arc_neg(&c, v);
  fail_unless(TYPE(neg) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REP(neg)._rational, -1, 2) == 0);
#endif

  neg = __arc_neg(&c, CNIL);
  fail_unless(error == 1);
  fail_unless(neg == CNIL);
}
END_TEST

START_TEST(test_cmp_fixnum)
{
  value cmp;

  cmp = arc_numcmp(&c, INT2FIX(8), INT2FIX(21));
  fail_unless(TYPE(cmp) == T_FIXNUM);
  fail_unless(FIX2INT(cmp) == -1);

  cmp = arc_numcmp(&c, INT2FIX(21), INT2FIX(8));
  fail_unless(TYPE(cmp) == T_FIXNUM);
  fail_unless(FIX2INT(cmp) == 1);

  cmp = arc_numcmp(&c, INT2FIX(8), INT2FIX(8));
  fail_unless(TYPE(cmp) == T_FIXNUM);
  fail_unless(FIX2INT(cmp) == 0);

  cmp = arc_numcmp(&c, INT2FIX(-21), INT2FIX(-8));
  fail_unless(TYPE(cmp) == T_FIXNUM);
  fail_unless(FIX2INT(cmp) == -1);

  cmp = arc_numcmp(&c, INT2FIX(-8), INT2FIX(-21));
  fail_unless(TYPE(cmp) == T_FIXNUM);
  fail_unless(FIX2INT(cmp) == 1);

  cmp = arc_numcmp(&c, INT2FIX(-8), INT2FIX(-8));
  fail_unless(TYPE(cmp) == T_FIXNUM);
  fail_unless(FIX2INT(cmp) == 0);

  cmp = arc_numcmp(&c, INT2FIX(-21), INT2FIX(8));
  fail_unless(TYPE(cmp) == T_FIXNUM);
  fail_unless(FIX2INT(cmp) == -1);

  cmp = arc_numcmp(&c, INT2FIX(8), INT2FIX(-21));
  fail_unless(TYPE(cmp) == T_FIXNUM);
  fail_unless(FIX2INT(cmp) == 1);
}
END_TEST

START_TEST(test_coerce_flonum)
{
  double d;
  value v;


#ifdef HAVE_GMP_H
  v = arc_mkbignuml(&c, 1);
  mpz_set_str(REP(v)._bignum, "100000000000000000000000000000", 10);
  d = arc_coerce_flonum(&c, v);
  fail_unless(fabs(1e29 - d) < 1e-6);
#endif

  v = arc_mkflonum(&c, 3.14159);
  d = arc_coerce_flonum(&c, v);
  fail_unless(fabs(3.14159 - d) < 1e-6);
  v = INT2FIX(0xdeadbee);
  d = arc_coerce_flonum(&c, v);
  fail_unless(fabs(233495534.0 - d) < 1e-6);
  v = c.get_cell(&c);
  BTYPE(v) = T_CONS;
  d = arc_coerce_flonum(&c, v);
  fail_unless(error == 1);
  error = 0;
}
END_TEST

START_TEST(test_coerce_fixnum)
{
  value v, v2;

  error = 0;

  /* Identity */
  v = INT2FIX(1);
  v2 = arc_coerce_fixnum(&c, v);
  fail_unless(TYPE(v2) == T_FIXNUM);
  fail_unless(FIX2INT(v2) == 1);

  /* Truncates */
  v = arc_mkflonum(&c, 3.14159);
  v2 = arc_coerce_fixnum(&c, v);
  fail_unless(TYPE(v2) == T_FIXNUM);
  fail_unless(FIX2INT(v2) == 3);

  /* Fails conversion (too big) */
  v = arc_mkflonum(&c, 1e100);
  v2 = arc_coerce_fixnum(&c, v);
  fail_unless(v2 == CNIL);
  fail_unless(TYPE(v2) == T_NIL);

  /* Should also fail conversion */
  v = arc_mkflonum(&c, ((double)(FIXNUM_MAX))*2);
  v2 = arc_coerce_fixnum(&c, v);
  fail_unless(v2 == CNIL);
  fail_unless(TYPE(v2) == T_NIL);

#ifdef HAVE_GMP_H
  /* Small bignum should be converted */
  v = arc_mkbignuml(&c, 1000);
  v2 = arc_coerce_fixnum(&c, v);
  fail_unless(TYPE(v2) == T_FIXNUM);
  fail_unless(FIX2INT(v2) == 1000);

  /* too big to convert to fixnum */
  v = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v)._bignum, "100000000000000000000000000000", 10);
  v2 = arc_coerce_fixnum(&c, v);
  fail_unless(v2 == CNIL);
  fail_unless(TYPE(v2) == T_NIL);
#endif
}
END_TEST

START_TEST(test_coerce_bignum)
{
#ifdef HAVE_GMP_H
  value v;
  mpz_t v2;

  error = 0;
  mpz_init(v2);
  v = arc_mkbignuml(&c, 1000);
  arc_coerce_bignum(&c, v, &v2);
  fail_unless(mpz_cmp(REP(v)._bignum, v2) == 0);

  v = arc_mkflonum(&c, 3.14159);
  arc_coerce_bignum(&c, v, &v2);
  fail_unless(fabs(mpz_get_d(v2) - 3.0) < 1e-6);

  v = INT2FIX(32);
  arc_coerce_bignum(&c, v, &v2);
  fail_unless(mpz_get_si(v2) == 32);

  v = cons(&c, 1,2);
  arc_coerce_bignum(&c, v, &v2);
  fail_unless(error == 1);
  error = 0;

  v = arc_mkcomplex(&c, 3.14159, 2.71828);
  arc_coerce_bignum(&c, v, &v2);
  fail_unless(error == 1);
  error = 0;

  mpz_clear(v2);
#endif
}
END_TEST

START_TEST(test_coerce_rational)
{
#ifdef HAVE_GMP_H
  value v;
  mpq_t v2;

  mpq_init(v2);

  v = arc_mkrationall(&c, 1, 2);
  arc_coerce_rational(&c, v, &v2);
  fail_unless(mpq_cmp(REP(v)._rational, v2) == 0);

  error = 0;
  v = arc_mkbignuml(&c, 1000);
  arc_coerce_rational(&c, v, &v2);
  fail_unless(mpq_cmp_si(v2, 1000, 1) == 0);

  v = arc_mkflonum(&c, 3.14159);
  arc_coerce_rational(&c, v, &v2);
  fail_unless(fabs(mpq_get_d(v2) - 3.14159) < 1e-6);

  v = INT2FIX(32);
  arc_coerce_rational(&c, v, &v2);
  fail_unless(mpq_cmp_si(v2, 32, 1) == 0);

  v = cons(&c, 1,2);
  arc_coerce_rational(&c, v, &v2);
  fail_unless(error == 1);
  error = 0;

  v = arc_mkcomplex(&c, 3.14159, 2.71828);
  arc_coerce_rational(&c, v, &v2);
  fail_unless(error == 1);
  error = 0;
#endif
}
END_TEST

START_TEST(test_coerce_complex)
{
  double re, im;
  value v;

#ifdef HAVE_GMP_H
  v = arc_mkbignuml(&c, 1);
  mpz_set_str(REP(v)._bignum, "100000000000000000000000000000", 10);
  arc_coerce_complex(&c, v, &re, &im);
  fail_unless(fabs(1e29 - re) < 1e-6);
  fail_unless(fabs(im) < 1e-6);
#endif

  v = arc_mkcomplex(&c, 3.14159, 2.71828);
  arc_coerce_complex(&c, v, &re, &im);
  fail_unless(fabs(3.14159 - re) < 1e-6);
  fail_unless(fabs(2.71828 - im) < 1e-6);

  v = arc_mkflonum(&c, 3.14159);
  arc_coerce_complex(&c, v, &re, &im);
  fail_unless(fabs(3.14159 - re) < 1e-6);
  fail_unless(fabs(im) < 1e-6);

  v = INT2FIX(0xdeadbee);
  arc_coerce_complex(&c, v, &re, &im);
  fail_unless(fabs(233495534.0 - re) < 1e-6);
  fail_unless(fabs(im) < 1e-6);

  v = c.get_cell(&c);
  BTYPE(v) = T_CONS;
  arc_coerce_complex(&c, v, &re, &im);
  fail_unless(error == 1);
  error = 0;
}
END_TEST

START_TEST(test_string2num)
{
  value str, num;
#ifdef HAVE_GMP_H
  mpz_t expected;
  mpq_t qexpected;
#endif

  str = arc_mkstringc(&c, "0");
  num = arc_string2num(&c, str);
  fail_unless(TYPE(num) == T_FIXNUM);
  fail_unless(FIX2INT(num) == 0);

  str = arc_mkstringc(&c, "1234");
  num = arc_string2num(&c, str);
  fail_unless(TYPE(num) == T_FIXNUM);
  fail_unless(FIX2INT(num) == 1234);

  str = arc_mkstringc(&c, "0x1234");
  num = arc_string2num(&c, str);
  fail_unless(TYPE(num) == T_FIXNUM);
  fail_unless(FIX2INT(num) == 0x1234);

  str = arc_mkstringc(&c, "01234");
  num = arc_string2num(&c, str);
  fail_unless(TYPE(num) == T_FIXNUM);
  fail_unless(FIX2INT(num) == 01234);

  str = arc_mkstringc(&c, "0b11011010");
  num = arc_string2num(&c, str);
  fail_unless(TYPE(num) == T_FIXNUM);
  fail_unless(FIX2INT(num) == 218);

  str = arc_mkstringc(&c, "16r1234");
  num = arc_string2num(&c, str);
  fail_unless(TYPE(num) == T_FIXNUM);
  fail_unless(FIX2INT(num) == 0x1234);

  str = arc_mkstringc(&c, "36rWxY");
  num = arc_string2num(&c, str);
  fail_unless(TYPE(num) == T_FIXNUM);
  fail_unless(FIX2INT(num) == 42694);

  str = arc_mkstringc(&c, "1.234");
  num = arc_string2num(&c, str);
  fail_unless(TYPE(num) == T_FLONUM);
  fail_unless(fabs(1.234 - REP(num)._flonum) < 1e-6);

  str = arc_mkstringc(&c, "1.2E3");
  num = arc_string2num(&c, str);
  fail_unless(TYPE(num) == T_FLONUM);
  fail_unless(fabs(1.2e3 - REP(num)._flonum) < 1e-6);

  str = arc_mkstringc(&c, "-1.234");
  num = arc_string2num(&c, str);
  fail_unless(TYPE(num) == T_FLONUM);
  fail_unless(fabs(-1.234 - REP(num)._flonum) < 1e-6);

  str = arc_mkstringc(&c, "-1.2e-3");
  num = arc_string2num(&c, str);
  fail_unless(TYPE(num) == T_FLONUM);
  fail_unless(fabs(-1.2e-3 - REP(num)._flonum) < 1e-6);

  str = arc_mkstringc(&c, "-1.2i");
  num = arc_string2num(&c, str);
  fail_unless(TYPE(num) == T_COMPLEX);
  fail_unless(fabs(0.0 - REP(num)._complex.re) < 1e-6);
  fail_unless(fabs(-1.2 - REP(num)._complex.im) < 1e-6);

  str = arc_mkstringc(&c, "-1.2j");
  num = arc_string2num(&c, str);
  fail_unless(TYPE(num) == T_COMPLEX);
  fail_unless(fabs(0.0 - REP(num)._complex.re) < 1e-6);
  fail_unless(fabs(-1.2 - REP(num)._complex.im) < 1e-6);

  str = arc_mkstringc(&c, "-1.2I");
  num = arc_string2num(&c, str);
  fail_unless(TYPE(num) == T_COMPLEX);
  fail_unless(fabs(0.0 - REP(num)._complex.re) < 1e-6);
  fail_unless(fabs(-1.2 - REP(num)._complex.im) < 1e-6);

  str = arc_mkstringc(&c, "-1.2J");
  num = arc_string2num(&c, str);
  fail_unless(TYPE(num) == T_COMPLEX);
  fail_unless(fabs(0.0 - REP(num)._complex.re) < 1e-6);
  fail_unless(fabs(-1.2 - REP(num)._complex.im) < 1e-6);

  str = arc_mkstringc(&c, "2.1-1.2i");
  num = arc_string2num(&c, str);
  fail_unless(TYPE(num) == T_COMPLEX);
  fail_unless(fabs(2.1 - REP(num)._complex.re) < 1e-6);
  fail_unless(fabs(-1.2 - REP(num)._complex.im) < 1e-6);

  str = arc_mkstringc(&c, "-1i");
  num = arc_string2num(&c, str);
  fail_unless(TYPE(num) == T_COMPLEX);
  fail_unless(fabs(0.0 - REP(num)._complex.re) < 1e-6);
  fail_unless(fabs(-1.0 - REP(num)._complex.im) < 1e-6);

  str = arc_mkstringc(&c, "2+3i");
  num = arc_string2num(&c, str);
  fail_unless(TYPE(num) == T_COMPLEX);
  fail_unless(fabs(2.0 - REP(num)._complex.re) < 1e-6);
  fail_unless(fabs(3.0 - REP(num)._complex.im) < 1e-6);

#ifdef HAVE_GMP_H
  str = arc_mkstringc(&c, "36rzyxwvutsrqponmlkjihgfedcba9876543210");
  num = arc_string2num(&c, str);
  fail_unless(TYPE(num) == T_BIGNUM);
  mpz_init(expected);
  mpz_set_str(expected, "zyxwvutsrqponmlkjihgfedcba9876543210", 36);
  fail_unless(mpz_cmp(expected, REP(num)._bignum) == 0);
  mpz_clear(expected);

  str = arc_mkstringc(&c, "1/2");
  num = arc_string2num(&c, str);
  fail_unless(TYPE(num) == T_RATIONAL);
  mpq_init(qexpected);
  mpq_set_str(qexpected, "1/2", 10);
  fail_unless(mpq_cmp(qexpected, REP(num)._rational) == 0);
  mpq_clear(qexpected);
#endif

  str = arc_mkstringc(&c, "+");
  num = arc_string2num(&c, str);
  fail_unless(num == CNIL);

  str = arc_mkstringc(&c, "_");
  num = arc_string2num(&c, str);
  fail_unless(num == CNIL);
}
END_TEST

START_TEST(test_intfix_conv)
{
  int i;

  for (i=1; i<100; i++)
    fail_unless(FIX2INT(INT2FIX(-i)) == -i);
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Arithmetic");
  TCase *tc_ops = tcase_create("Operators");
  TCase *tc_conv = tcase_create("Conversions");
  SRunner *sr;

  tcase_add_test(tc_ops, test_mul_fixnum);
  tcase_add_test(tc_ops, test_mul_bignum);
  tcase_add_test(tc_ops, test_mul_flonum);
  tcase_add_test(tc_ops, test_mul_rational);
  tcase_add_test(tc_ops, test_mul_complex);

  tcase_add_test(tc_ops, test_mul_fixnum2bignum);
  tcase_add_test(tc_ops, test_mul_fixnum2flonum);
  tcase_add_test(tc_ops, test_mul_fixnum2rational);
  tcase_add_test(tc_ops, test_mul_fixnum2complex);

  tcase_add_test(tc_ops, test_mul_bignum2flonum);
  tcase_add_test(tc_ops, test_mul_bignum2rational);
  tcase_add_test(tc_ops, test_mul_bignum2complex);

  tcase_add_test(tc_ops, test_mul_flonum2rational);
  tcase_add_test(tc_ops, test_mul_flonum2complex);

  tcase_add_test(tc_ops, test_mul_rational2complex);

  tcase_add_test(tc_ops, test_mul_misc);


  tcase_add_test(tc_ops, test_div_fixnum);
  tcase_add_test(tc_ops, test_div_bignum);
  tcase_add_test(tc_ops, test_div_flonum);
  tcase_add_test(tc_ops, test_div_rational);
  tcase_add_test(tc_ops, test_div_complex);

  tcase_add_test(tc_ops, test_div_fixnum2bignum);
  tcase_add_test(tc_ops, test_div_fixnum2flonum);
  tcase_add_test(tc_ops, test_div_fixnum2rational);
  tcase_add_test(tc_ops, test_div_fixnum2complex);

  tcase_add_test(tc_ops, test_div_misc);


  tcase_add_test(tc_ops, test_mod);


  tcase_add_test(tc_ops, test_add_fixnum);
  tcase_add_test(tc_ops, test_add_bignum);
  tcase_add_test(tc_ops, test_add_flonum);
  tcase_add_test(tc_ops, test_add_rational);
  tcase_add_test(tc_ops, test_add_complex);

  tcase_add_test(tc_ops, test_add_fixnum2bignum);
  tcase_add_test(tc_ops, test_add_fixnum2flonum);
  tcase_add_test(tc_ops, test_add_fixnum2rational);
  tcase_add_test(tc_ops, test_add_fixnum2complex);

  tcase_add_test(tc_ops, test_add_bignum2flonum);
  tcase_add_test(tc_ops, test_add_bignum2rational);
  tcase_add_test(tc_ops, test_add_bignum2complex);

  tcase_add_test(tc_ops, test_add_flonum2rational);
  tcase_add_test(tc_ops, test_add_flonum2complex);

  tcase_add_test(tc_ops, test_add_rational2complex);

  tcase_add_test(tc_ops, test_add_misc);


  tcase_add_test(tc_ops, test_sub_fixnum);
  tcase_add_test(tc_ops, test_sub_bignum);
  tcase_add_test(tc_ops, test_sub_flonum);
  tcase_add_test(tc_ops, test_sub_rational);
  tcase_add_test(tc_ops, test_sub_complex);

  tcase_add_test(tc_ops, test_sub_fixnum2bignum);
  tcase_add_test(tc_ops, test_sub_fixnum2flonum);
  tcase_add_test(tc_ops, test_sub_fixnum2rational);
  tcase_add_test(tc_ops, test_sub_fixnum2complex);

  tcase_add_test(tc_ops, test_sub_bignum2flonum);
  tcase_add_test(tc_ops, test_sub_bignum2rational);
  tcase_add_test(tc_ops, test_sub_bignum2complex);

  tcase_add_test(tc_ops, test_sub_flonum2rational);
  tcase_add_test(tc_ops, test_sub_flonum2complex);

  tcase_add_test(tc_ops, test_sub_rational2complex);

  tcase_add_test(tc_ops, test_sub_misc);


  tcase_add_test(tc_ops, test_neg);


  tcase_add_test(tc_ops, test_cmp_fixnum);

  tcase_add_test(tc_conv, test_coerce_fixnum);
  tcase_add_test(tc_conv, test_coerce_flonum);
  tcase_add_test(tc_conv, test_coerce_bignum);
  tcase_add_test(tc_conv, test_coerce_rational);
  tcase_add_test(tc_conv, test_coerce_complex);
  tcase_add_test(tc_conv, test_string2num);
  tcase_add_test(tc_conv, test_intfix_conv);

  arc_set_memmgr(&c);
  c.signal_error = signal_error_test;

  suite_add_tcase(s, tc_ops);
  suite_add_tcase(s, tc_conv);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

