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
#include <string.h>
#include <check.h>
#include <math.h>
#include <stdio.h>
#include "../src/carc.h"
#include "../src/arith.h"
#include "../config.h"

START_TEST(test_add_fixnum)
{
  int i;
  carc c;
  value v = INT2FIX(0);

  for (i=1; i<=100; i++)
    v = __carc_add2(&c, v, INT2FIX(i));
  fail_unless(TYPE(v) == T_FIXNUM);
  fail_unless(FIX2INT(v) == 5050);
  
}
END_TEST

/* This is a very basic memory allocation routine that essentially
   "fakes" it out. */
value get_cell_test(struct carc *c)
{
  static struct cell cells[1024];
  static int cellptr = 0;

  return((value)(cells + ((cellptr++) % 1024)));
}

START_TEST(test_add_fixnum2bignum)
{
#ifdef HAVE_GMP_H
  value maxfixnum, one, negone, sum;
  carc c;

  c.get_cell = get_cell_test;

  maxfixnum = INT2FIX(FIXNUM_MAX);
  one = INT2FIX(1);
  negone = INT2FIX(-1);
  sum = __carc_add2(&c, maxfixnum, one);
  fail_unless(TYPE(sum) == T_BIGNUM);
  fail_unless(mpz_get_si(REP(sum)._bignum) == FIXNUM_MAX + 1);

  sum = __carc_add2(&c, negone, sum);
  fail_unless(TYPE(sum) == T_FIXNUM);
  fail_unless(FIX2INT(sum) == FIXNUM_MAX);
#endif
}
END_TEST

START_TEST(test_add_fixnum2flonum)
{
  value val1, val2, sum;
  carc c;

  c.get_cell = get_cell_test;

  val1 = INT2FIX(1);
  val2 = carc_mkflonum(&c, 3.14159);

  sum = __carc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_FLONUM);
  fail_unless(fabs(4.14159 - REP(sum)._flonum) < 1e-6);

  val1 = INT2FIX(-1);
  sum = __carc_add2(&c, sum, val1);
  fail_unless(TYPE(sum) == T_FLONUM);
  fail_unless(fabs(3.14159 - REP(sum)._flonum) < 1e-6);
}
END_TEST

START_TEST(test_add_fixnum2complex)
{
  value val1, val2, sum;
  carc c;

  c.get_cell = get_cell_test;

  val1 = INT2FIX(1);
  val2 = carc_mkcomplex(&c, 1.1, 2.2);

  sum = __carc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_COMPLEX);
  fail_unless(fabs(2.1 - REP(sum)._complex.re) < 1e-6);
  fail_unless(fabs(2.2 - REP(sum)._complex.im) < 1e-6);

  val1 = INT2FIX(-1);
  sum = __carc_add2(&c, sum, val1);
  fail_unless(TYPE(sum) == T_COMPLEX);
  fail_unless(fabs(1.1 - REP(sum)._complex.re) < 1e-6);
  fail_unless(fabs(2.2 - REP(sum)._complex.im) < 1e-6);
}
END_TEST

START_TEST(test_add_fixnum2rational)
{
#ifdef HAVE_GMP_H
  value v1, v2, sum;
  carc c;

  c.get_cell = get_cell_test;

  v1 = carc_mkrationall(&c, 1, 2);
  v2 = INT2FIX(1);
  sum = __carc_add2(&c, v1, v2);
  fail_unless(TYPE(sum) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REP(sum)._rational, 3, 2) == 0);

  v1 = INT2FIX(1);
  v2 = carc_mkrationall(&c, 1, 2);
  sum = __carc_add2(&c, v1, v2);
  fail_unless(TYPE(sum) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REP(sum)._rational, 3, 2) == 0);
#endif
}
END_TEST

START_TEST(test_add_flonum)
{
  value val1, val2, sum;
  carc c;

  c.get_cell = get_cell_test;

  val1 = carc_mkflonum(&c, 2.71828);
  val2 = carc_mkflonum(&c, 3.14159);

  sum = __carc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_FLONUM);
  fail_unless(fabs(5.85987 - REP(sum)._flonum) < 1e-6);
}
END_TEST

START_TEST(test_add_complex)
{
  value val1, val2, sum;
  carc c;

  c.get_cell = get_cell_test;

  val1 = carc_mkcomplex(&c, 1, -2);
  val2 = carc_mkcomplex(&c, 3, -4);

  sum = __carc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_COMPLEX);
  fail_unless(fabs(4 - REP(sum)._complex.re) < 1e-6);
  fail_unless(fabs(-6 - REP(sum)._complex.im) < 1e-6);
}
END_TEST

START_TEST(test_add_rational2complex)
{
#ifdef HAVE_GMP_H
  value val1, val2, sum;
  carc c;

  c.get_cell = get_cell_test;

  val1 = carc_mkcomplex(&c, 0.5, 0.5);
  val2 = carc_mkrationall(&c, 1, 2);
  sum = __carc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_COMPLEX);
  fail_unless(fabs(1.0 - REP(sum)._complex.re) < 1e-6);
  fail_unless(fabs(0.5 - REP(sum)._complex.im) < 1e-6);

  val1 = carc_mkrationall(&c, 1, 2);
  val2 = carc_mkcomplex(&c, 0.5, 0.5);
  sum = __carc_add2(&c, val1, val2);
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
  carc c;

  c.get_cell = get_cell_test;

  val1 = carc_mkflonum(&c, 0.5);
  val2 = carc_mkrationall(&c, 1, 2);
  sum = __carc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_FLONUM);
  fail_unless(fabs(1.0 - REP(sum)._flonum) < 1e-6);

  val1 = carc_mkrationall(&c, 1, 2);
  val2 = carc_mkflonum(&c, 0.5);
  sum = __carc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_FLONUM);
  fail_unless(fabs(1.0 - REP(sum)._flonum) < 1e-6);
#endif
}
END_TEST

static int error = 0;

static void signal_error_test(struct carc *c, const char *fmt, ...)
{
  error = 1;
}

START_TEST(test_add_bignum)
{
#ifdef HAVE_GMP_H
  value val1, val2, sum;
  carc c;
  mpz_t expected;

  c.get_cell = get_cell_test;
  val1 = carc_mkbignuml(&c, FIXNUM_MAX+1);
  val2 = carc_mkbignuml(&c, -(FIXNUM_MAX+2));
  sum = __carc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_FIXNUM);
  fail_unless(FIX2INT(sum) == -1);

  val1 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(val1)._bignum, "100000000000000000000000000000", 10);
  val2 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(val2)._bignum, "200000000000000000000000000000", 10);
  sum = __carc_add2(&c, val1, val2);
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
  carc c;

  c.get_cell = get_cell_test;

  v1 = carc_mkflonum(&c, 0.0);
  v2 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(v2)._bignum, "10000000000000000000000", 10);
  sum = __carc_add2(&c, v1, v2);
  fail_unless(TYPE(sum) == T_FLONUM);
  fail_unless(fabs(REP(sum)._flonum - 1e22) < 1e-6);

  v1 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(v1)._bignum, "10000000000000000000000", 10);
  v2 = carc_mkflonum(&c, 0.0);
  sum = __carc_add2(&c, v1, v2);
  fail_unless(TYPE(sum) == T_FLONUM);
  fail_unless(fabs(REP(sum)._flonum - 1e22) < 1e-6);
#endif
}
END_TEST

START_TEST(test_add_bignum2complex)
{
#ifdef HAVE_GMP_H
  value v1, v2, sum;
  carc c;

  c.get_cell = get_cell_test;

  v1 = carc_mkcomplex(&c, 0.0, 1.1);
  v2 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(v2)._bignum, "10000000000000000000000", 10);
  sum = __carc_add2(&c, v1, v2);
  fail_unless(TYPE(sum) == T_COMPLEX);
  fail_unless(fabs(REP(sum)._complex.re - 1e22) < 1e-6);
  fail_unless(fabs(REP(sum)._complex.im - 1.1) < 1e-6);

  v1 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(v1)._bignum, "10000000000000000000000", 10);
  v2 = carc_mkcomplex(&c, 0.0, 1.1);
  sum = __carc_add2(&c, v1, v2);
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
  carc c;
  mpq_t expected;

  c.get_cell = get_cell_test;

  v1 = carc_mkrationall(&c, 1, 2);
  v2 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(v2)._bignum, "100000000000000000000000000000", 10);
  sum = __carc_add2(&c, v1, v2);
  fail_unless(TYPE(sum) == T_RATIONAL);
  mpq_init(expected);
  mpq_set_str(expected, "200000000000000000000000000001/2", 10);
  fail_unless(mpq_cmp(expected, REP(sum)._rational) == 0);
  mpq_clear(expected);

  v1 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(v1)._bignum, "100000000000000000000000000000", 10);
  v2 = carc_mkrationall(&c, 1, 2);
  sum = __carc_add2(&c, v1, v2);
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
  carc c;

  c.get_cell = get_cell_test;

  v1 = carc_mkcomplex(&c, 1.0, 1.0);
  v2 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(v2)._bignum, "10000000000000000000000", 10);
  prod = __carc_mul2(&c, v1, v2);
  fail_unless(TYPE(prod) == T_COMPLEX);
  fail_unless(fabs(REP(prod)._complex.re - 1e22) < 1e-6);
  fail_unless(fabs(REP(prod)._complex.im - 1e22) < 1e-6);

  v1 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(v1)._bignum, "10000000000000000000000", 10);
  v2 = carc_mkcomplex(&c, 1.0, 1.0);
  prod = __carc_mul2(&c, v1, v2);
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
  carc c;
  mpz_t expected;

  c.get_cell = get_cell_test;
  val1 = carc_mkrationall(&c, 1, 2);
  val2 = carc_mkrationall(&c, 1, 4);
  sum = __carc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REP(sum)._rational, 3, 4) == 0);

  val1 = sum;
  sum = __carc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_FIXNUM);
  fail_unless(FIX2INT(sum) == 1);

  val1 = carc_mkrationall(&c, 0, 1);
  mpq_set_str(REP(val1)._rational, "1606938044258990275541962092341162602522202993782792835301375/4", 10);
  val2 = carc_mkrationall(&c, 0, 1);
  mpq_set_str(REP(val2)._rational, "1/4", 10);
  sum = __carc_add2(&c, val1, val2);
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
  carc c;

  c.get_cell = get_cell_test;

  val1 = carc_mkflonum(&c, 0.5);
  val2 = carc_mkcomplex(&c, 3, -4);
  sum = __carc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_COMPLEX);
  fail_unless(fabs(3.5 - REP(sum)._complex.re) < 1e-6);
  fail_unless(fabs(-4 - REP(sum)._complex.im) < 1e-6);

  val1 = carc_mkcomplex(&c, 3, -4);
  val2 = carc_mkflonum(&c, 0.5);
  sum = __carc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_COMPLEX);
  fail_unless(fabs(3.5 - REP(sum)._complex.re) < 1e-6);
  fail_unless(fabs(-4 - REP(sum)._complex.im) < 1e-6);

}
END_TEST

START_TEST(test_add_misc)
{
  carc c;
  value sum;

  c.get_cell = get_cell_test;
  c.signal_error = signal_error_test;
  error = 0;

  sum = __carc_add2(&c, CNIL, CNIL);
  fail_unless(error == 1);
  error = 0;

  sum = __carc_add2(&c, cons(&c, FIX2INT(1), CNIL), 
		    cons(&c, FIX2INT(2), CNIL));
}
END_TEST

START_TEST(test_mul_fixnum)
{
  carc c;
  value prod;
#ifdef HAVE_GMP_H
  mpz_t expected;
#endif

  c.get_cell = get_cell_test;
  c.signal_error = signal_error_test;
  error = 0;

  prod = __carc_mul2(&c, INT2FIX(8), INT2FIX(21));
  fail_unless(TYPE(prod) == T_FIXNUM);
  fail_unless(FIX2INT(prod) == 168);

  prod = __carc_mul2(&c, INT2FIX(-8), INT2FIX(-21));
  fail_unless(TYPE(prod) == T_FIXNUM);
  fail_unless(FIX2INT(prod) == 168);

  prod = __carc_mul2(&c, INT2FIX(-8), INT2FIX(21));
  fail_unless(TYPE(prod) == T_FIXNUM);
  fail_unless(FIX2INT(prod) == -168);

  prod = __carc_mul2(&c, INT2FIX(8), INT2FIX(-21));
  fail_unless(TYPE(prod) == T_FIXNUM);
  fail_unless(FIX2INT(prod) == -168);

#ifdef HAVE_GMP_H
  prod = __carc_mul2(&c, INT2FIX(2), INT2FIX(FIXNUM_MAX));
  fail_unless(TYPE(prod) == T_BIGNUM);
  mpz_init(expected);
  mpz_set_si(expected, 2*FIXNUM_MAX);
  fail_unless(mpz_cmp(expected, REP(prod)._bignum) == 0);

  prod = __carc_mul2(&c, INT2FIX(-2), INT2FIX(-FIXNUM_MAX));
  fail_unless(TYPE(prod) == T_BIGNUM);
  mpz_set_si(expected, 2*FIXNUM_MAX);
  fail_unless(mpz_cmp(expected, REP(prod)._bignum) == 0);

  prod = __carc_mul2(&c, INT2FIX(-2), INT2FIX(FIXNUM_MAX));
  fail_unless(TYPE(prod) == T_BIGNUM);
  mpz_set_si(expected, -2*FIXNUM_MAX);
  fail_unless(mpz_cmp(expected, REP(prod)._bignum) == 0);

  prod = __carc_mul2(&c, INT2FIX(2), INT2FIX(-FIXNUM_MAX));
  fail_unless(TYPE(prod) == T_BIGNUM);
  mpz_set_si(expected, -2*FIXNUM_MAX);
  fail_unless(mpz_cmp(expected, REP(prod)._bignum) == 0);
  mpz_clear(expected);
#else
  prod = __carc_mul2(&c, INT2FIX(2), INT2FIX(FIXNUM_MAX));
  fail_unless(TYPE(prod) == T_NIL);
  fail_unless(error == 1);
#endif

}
END_TEST

START_TEST(test_mul_bignum)
{
#ifdef HAVE_GMP_H
  value val1, val2, sum;
  carc c;
  mpz_t expected;

  c.get_cell = get_cell_test;
  val1 = carc_mkbignuml(&c, FIXNUM_MAX+1);
  val2 = carc_mkbignuml(&c, -(FIXNUM_MAX+2));
  sum = __carc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_FIXNUM);
  fail_unless(FIX2INT(sum) == -1);

  val1 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(val1)._bignum, "400000000000000000000000000000", 10);
  val2 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(val2)._bignum, "300000000000000000000000000000", 10);
  sum = __carc_mul2(&c, val1, val2);
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
  carc c;

  c.get_cell = get_cell_test;

  val1 = carc_mkflonum(&c, 1.20257);
  val2 = carc_mkflonum(&c, 0.57721);

  prod = __carc_mul2(&c, val1, val2);
  fail_unless(TYPE(prod) == T_FLONUM);
  fail_unless(fabs(0.694135 - REP(prod)._flonum) < 1e-6);
}
END_TEST

START_TEST(test_mul_complex)
{
  value val1, val2, prod;
  carc c;

  c.get_cell = get_cell_test;

  val1 = carc_mkcomplex(&c, 2.0, 1.0);
  val2 = carc_mkcomplex(&c, 3.0, 2.0);

  prod = __carc_mul2(&c, val1, val2);
  fail_unless(TYPE(prod) == T_COMPLEX);
  fail_unless(fabs(4.0 - REP(prod)._complex.re) < 1e-6);
  fail_unless(fabs(7.0 - REP(prod)._complex.im) < 1e-6);
}
END_TEST

START_TEST(test_mul_rational)
{
#ifdef HAVE_GMP_H
  value val1, val2, prod;
  carc c;
  mpz_t expected;

  c.get_cell = get_cell_test;
  val1 = carc_mkrationall(&c, 1, 2);
  val2 = carc_mkrationall(&c, 1, 4);
  prod = __carc_mul2(&c, val1, val2);
  fail_unless(TYPE(prod) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REP(prod)._rational, 1, 8) == 0);

  val1 = carc_mkrationall(&c, 3, 4);
  val2 = carc_mkrationall(&c, 4, 3);
  prod = __carc_mul2(&c, val1, val2);
  fail_unless(TYPE(prod) == T_FIXNUM);
  fail_unless(FIX2INT(prod) == 1);

  val1 = carc_mkrationall(&c, 3, 4);
  val2 = carc_mkrationall(&c, 4, 3);
  prod = __carc_mul2(&c, val1, val2);
  fail_unless(TYPE(prod) == T_FIXNUM);
  fail_unless(FIX2INT(prod) == 1);

  val1 = carc_mkrationall(&c, 0, 1);
  mpq_set_str(REP(val1)._rational, "115792089237316195423570985008687907853269984665640564039457584007913129639936/3", 10);
  val2 = carc_mkrationall(&c, 3, 4);
  prod = __carc_mul2(&c, val1, val2);
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
  carc c;
  mpz_t expected;
  int i;

  c.get_cell = get_cell_test;
  factorial = INT2FIX(1);
  for (i=1; i<=100; i++)
    factorial = __carc_mul2(&c, INT2FIX(i), factorial);

  mpz_init(expected);
  mpz_set_str(expected, "93326215443944152681699238856266700490715968264381621468592963895217599993229915608941463976156518286253697920827223758251185210916864000000000000000000000000", 10);
  fail_unless(mpz_cmp(expected, REP(factorial)._bignum) == 0);
  mpz_clear(expected);
#endif
}
END_TEST

START_TEST(test_mul_misc)
{
  carc c;
  value prod;

  c.get_cell = get_cell_test;
  c.signal_error = signal_error_test;
  error = 0;

  prod = __carc_mul2(&c, CNIL, CNIL);
  fail_unless(error == 1);
  error = 0;

  prod = __carc_mul2(&c, cons(&c, FIX2INT(1), CNIL), 
		     cons(&c, FIX2INT(2), CNIL));
}
END_TEST

START_TEST(test_mul_fixnum2flonum)
{
  value val1, val2, prod;
  carc c;

  c.get_cell = get_cell_test;

  val1 = INT2FIX(2);
  val2 = carc_mkflonum(&c, 3.14159);

  prod = __carc_mul2(&c, val1, val2);
  fail_unless(TYPE(prod) == T_FLONUM);
  fail_unless(fabs(6.28318 - REP(prod)._flonum) < 1e-6);

  val1 = INT2FIX(3);
  prod = __carc_mul2(&c, prod, val1);
  fail_unless(TYPE(prod) == T_FLONUM);
  fail_unless(fabs(18.84954 - REP(prod)._flonum) < 1e-6);
}
END_TEST

START_TEST(test_mul_fixnum2rational)
{
#ifdef HAVE_GMP_H
  value v1, v2, prod;
  carc c;

  c.get_cell = get_cell_test;

  v1 = carc_mkrationall(&c, 1, 2);
  v2 = INT2FIX(3);
  prod = __carc_mul2(&c, v1, v2);
  fail_unless(TYPE(prod) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REP(prod)._rational, 3, 2) == 0);

  v1 = INT2FIX(3);
  v2 = carc_mkrationall(&c, 1, 2);
  prod = __carc_mul2(&c, v1, v2);
  fail_unless(TYPE(prod) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REP(prod)._rational, 3, 2) == 0);

  v1 = INT2FIX(2);
  v2 = carc_mkrationall(&c, 1, 2);
  prod = __carc_mul2(&c, v1, v2);
  fail_unless(TYPE(prod) == T_FIXNUM);
  fail_unless(FIX2INT(prod) == 1);
#endif
}
END_TEST

START_TEST(test_mul_fixnum2complex)
{
  value v1, v2, prod;
  carc c;

  c.get_cell = get_cell_test;

  v1 = carc_mkcomplex(&c, 1.0, 2.0);
  v2 = INT2FIX(3);
  prod = __carc_mul2(&c, v1, v2);
  fail_unless(TYPE(prod) == T_COMPLEX);
  fail_unless(fabs(3.0 - REP(prod)._complex.re) < 1e-6);
  fail_unless(fabs(6.0 - REP(prod)._complex.im) < 1e-6);

  v1 = INT2FIX(3);
  v1 = carc_mkcomplex(&c, 1.0, 2.0);
  prod = __carc_mul2(&c, v1, v2);
  fail_unless(fabs(3.0 - REP(prod)._complex.re) < 1e-6);
  fail_unless(fabs(6.0 - REP(prod)._complex.im) < 1e-6);
}
END_TEST

START_TEST(test_mul_bignum2flonum)
{
#ifdef HAVE_GMP_H
  value v1, v2, prod;
  carc c;

  c.get_cell = get_cell_test;

  v1 = carc_mkflonum(&c, 2.0);
  v2 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(v2)._bignum, "100000000000000000000000000000", 10);
  prod = __carc_mul2(&c, v1, v2);
  fail_unless(TYPE(prod) == T_FLONUM);
  fail_unless(fabs(prod - 2e29) > 1e-6);

  v1 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(v1)._bignum, "100000000000000000000000000000", 10);
  v2 = carc_mkflonum(&c, 2.0);
  prod = __carc_mul2(&c, v1, v2);
  fail_unless(TYPE(prod) == T_FLONUM);
  fail_unless(fabs(prod - 2e29) > 1e-6);
#endif
}
END_TEST

START_TEST(test_mul_bignum2rational)
{
#ifdef HAVE_GMP_H
  value v1, v2, prod;
  carc c;
  mpq_t expected;
  mpz_t zexpected;

  c.get_cell = get_cell_test;

  v1 = carc_mkrationall(&c, 1, 3);
  v2 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(v2)._bignum, "100000000000000000000000000000", 10);
  prod = __carc_mul2(&c, v1, v2);
  fail_unless(TYPE(prod) == T_RATIONAL);
  mpq_init(expected);
  mpq_set_str(expected, "100000000000000000000000000000/3", 10);
  fail_unless(mpq_cmp(expected, REP(prod)._rational) == 0);
  mpq_clear(expected);

  v1 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(v1)._bignum, "100000000000000000000000000000", 10);
  v2 = carc_mkrationall(&c, 1, 3);
  prod = __carc_mul2(&c, v1, v2);
  fail_unless(TYPE(prod) == T_RATIONAL);
  mpq_init(expected);
  mpq_set_str(expected, "100000000000000000000000000000/3", 10);
  fail_unless(mpq_cmp(expected, REP(prod)._rational) == 0);
  mpq_clear(expected);

  v1 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(v1)._bignum, "100000000000000000000000000000", 10);
  v2 = carc_mkrationall(&c, 1, 2);
  prod = __carc_mul2(&c, v1, v2);
  fail_unless(TYPE(prod) == T_BIGNUM);
  mpz_init(zexpected);
  mpz_set_str(zexpected, "50000000000000000000000000000", 10);
  fail_unless(mpz_cmp(zexpected, REP(prod)._bignum) == 0);
  mpz_clear(zexpected);

  v1 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(v1)._bignum, "100000000000000000000000000000", 10);
  v2 = carc_mkrationall(&c, 0, 1);
  mpq_set_str(REP(v2)._rational, "1/100000000000000000000000000000", 10);
  prod = __carc_mul2(&c, v1, v2);
  fail_unless(TYPE(prod) == T_FIXNUM);
  fail_unless(FIX2INT(prod) == 1);
#endif
}
END_TEST

START_TEST(test_mul_flonum2rational)
{
#ifdef HAVE_GMP_H
  value val1, val2, prod;
  carc c;

  c.get_cell = get_cell_test;

  val1 = carc_mkflonum(&c, 2.0);
  val2 = carc_mkrationall(&c, 1, 2);
  prod = __carc_mul2(&c, val1, val2);
  fail_unless(TYPE(prod) == T_FLONUM);
  fail_unless(fabs(1.0 - REP(prod)._flonum) < 1e-6);

  val1 = carc_mkrationall(&c, 1, 2);
  val2 = carc_mkflonum(&c, 2.0);
  prod = __carc_mul2(&c, val1, val2);
  fail_unless(TYPE(prod) == T_FLONUM);
  fail_unless(fabs(1.0 - REP(prod)._flonum) < 1e-6);
#endif
}
END_TEST

START_TEST(test_mul_flonum2complex)
{
  value val1, val2, prod;
  carc c;

  c.get_cell = get_cell_test;

  val1 = carc_mkflonum(&c, 0.5);
  val2 = carc_mkcomplex(&c, 3, -4);
  prod = __carc_mul2(&c, val1, val2);
  fail_unless(TYPE(prod) == T_COMPLEX);
  fail_unless(fabs(1.5 - REP(prod)._complex.re) < 1e-6);
  fail_unless(fabs(-2.0 - REP(prod)._complex.im) < 1e-6);

  val1 = carc_mkcomplex(&c, 3, -4);
  val2 = carc_mkflonum(&c, 0.5);
  prod = __carc_mul2(&c, val1, val2);
  fail_unless(TYPE(prod) == T_COMPLEX);
  fail_unless(fabs(1.5 - REP(prod)._complex.re) < 1e-6);
  fail_unless(fabs(-2.0 - REP(prod)._complex.im) < 1e-6);
}
END_TEST

START_TEST(test_mul_rational2complex)
{
#ifdef HAVE_GMP_H
  value val1, val2, prod;
  carc c;

  c.get_cell = get_cell_test;

  val1 = carc_mkcomplex(&c, 0.5, 0.25);
  val2 = carc_mkrationall(&c, 1, 2);
  prod = __carc_mul2(&c, val1, val2);
  fail_unless(TYPE(prod) == T_COMPLEX);
  fail_unless(fabs(0.25 - REP(prod)._complex.re) < 1e-6);
  fail_unless(fabs(0.125 - REP(prod)._complex.im) < 1e-6);

  val1 = carc_mkrationall(&c, 1, 2);
  val2 = carc_mkcomplex(&c, 0.5, 0.25);
  prod = __carc_mul2(&c, val1, val2);
  fail_unless(TYPE(prod) == T_COMPLEX);
  fail_unless(fabs(0.25 - REP(prod)._complex.re) < 1e-6);
  fail_unless(fabs(0.125 - REP(prod)._complex.im) < 1e-6);
#endif
}
END_TEST

START_TEST(test_sub_fixnum)
{
  int i;
  carc c;
  value v = INT2FIX(0);

  for (i=1; i<=100; i++)
    v = __carc_sub2(&c, v, INT2FIX(i));
  fail_unless(TYPE(v) == T_FIXNUM);
  fail_unless(FIX2INT(v) == -5050);
  
}
END_TEST

START_TEST(test_sub_bignum)
{
#ifdef HAVE_GMP_H
  value val1, val2, diff;
  carc c;
  mpz_t expected;

  c.get_cell = get_cell_test;
  val1 = carc_mkbignuml(&c, FIXNUM_MAX+1);
  val2 = carc_mkbignuml(&c, FIXNUM_MAX+2);
  diff = __carc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_FIXNUM);
  fail_unless(FIX2INT(diff) == -1);

  val1 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(val1)._bignum, "300000000000000000000000000000", 10);
  val2 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(val2)._bignum, "100000000000000000000000000000", 10);
  diff = __carc_sub2(&c, val1, val2);
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
  carc c;

  c.get_cell = get_cell_test;

  val1 = carc_mkflonum(&c, 2.71828);
  val2 = carc_mkflonum(&c, 3.14159);

  diff = __carc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_FLONUM);
  fail_unless(fabs(-0.42331 - REP(diff)._flonum) < 1e-6);
}
END_TEST

START_TEST(test_sub_rational)
{
#ifdef HAVE_GMP_H
  value val1, val2, diff;
  carc c;
  mpz_t expected;

  c.get_cell = get_cell_test;
  val1 = carc_mkrationall(&c, 1, 2);
  val2 = carc_mkrationall(&c, 1, 4);
  diff = __carc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REP(diff)._rational, 1, 4) == 0);

  val1 = diff;
  diff = __carc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_FIXNUM);
  fail_unless(FIX2INT(diff) == 0);

  val1 = carc_mkrationall(&c, 0, 1);
  mpq_set_str(REP(val1)._rational, "1606938044258990275541962092341162602522202993782792835301375/4", 10);
  val2 = carc_mkrationall(&c, 0, 1);
  mpq_set_str(REP(val2)._rational, "3/4", 10);
  diff = __carc_sub2(&c, val1, val2);
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
  carc c;

  c.get_cell = get_cell_test;

  val1 = carc_mkcomplex(&c, 1, -2);
  val2 = carc_mkcomplex(&c, 3, -4);

  diff = __carc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_COMPLEX);
  fail_unless(fabs(-2 - REP(diff)._complex.re) < 1e-6);
  fail_unless(fabs(2 - REP(diff)._complex.im) < 1e-6);
}
END_TEST

START_TEST(test_sub_fixnum2bignum)
{
#ifdef HAVE_GMP_H
  value maxfixnum, one, negone, diff;
  carc c;

  c.get_cell = get_cell_test;

  maxfixnum = INT2FIX(-FIXNUM_MAX);
  one = INT2FIX(1);
  negone = INT2FIX(-1);
  diff = __carc_sub2(&c, maxfixnum, one);
  fail_unless(TYPE(diff) == T_BIGNUM);
  fail_unless(mpz_get_si(REP(diff)._bignum) == -FIXNUM_MAX - 1);

  diff = __carc_sub2(&c, negone, diff);
  fail_unless(TYPE(diff) == T_FIXNUM);
  fail_unless(FIX2INT(diff) == FIXNUM_MAX);
#endif
}
END_TEST

START_TEST(test_sub_fixnum2flonum)
{
  value val1, val2, diff;
  carc c;

  c.get_cell = get_cell_test;

  val1 = INT2FIX(1);
  val2 = carc_mkflonum(&c, 3.14159);

  diff = __carc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_FLONUM);
  fail_unless(fabs(-2.14159 - REP(diff)._flonum) < 1e-6);

  val1 = INT2FIX(-1);
  diff = __carc_sub2(&c, diff, val1);
  fail_unless(TYPE(diff) == T_FLONUM);
  fail_unless(fabs(-1.14159 - REP(diff)._flonum) < 1e-6);
}
END_TEST

START_TEST(test_sub_fixnum2rational)
{
#ifdef HAVE_GMP_H
  value v1, v2, diff;
  carc c;

  c.get_cell = get_cell_test;

  v1 = carc_mkrationall(&c, 1, 2);
  v2 = INT2FIX(1);
  diff = __carc_sub2(&c, v1, v2);
  fail_unless(TYPE(diff) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REP(diff)._rational, -1, 2) == 0);

  v1 = INT2FIX(1);
  v2 = carc_mkrationall(&c, 1, 2);
  diff = __carc_sub2(&c, v1, v2);
  fail_unless(TYPE(diff) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REP(diff)._rational, 1, 2) == 0);
#endif
}
END_TEST

START_TEST(test_sub_fixnum2complex)
{
  value val1, val2, diff;
  carc c;

  c.get_cell = get_cell_test;

  val1 = INT2FIX(1);
  val2 = carc_mkcomplex(&c, 1.1, 2.2);

  diff = __carc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_COMPLEX);
  fail_unless(fabs(-0.1 - REP(diff)._complex.re) < 1e-6);
  fail_unless(fabs(-2.2 - REP(diff)._complex.im) < 1e-6);

  val1 = INT2FIX(-1);
  diff = __carc_sub2(&c, diff, val1);
  fail_unless(TYPE(diff) == T_COMPLEX);
  fail_unless(fabs(0.9 - REP(diff)._complex.re) < 1e-6);
  fail_unless(fabs(-2.2 - REP(diff)._complex.im) < 1e-6);
}
END_TEST

START_TEST(test_sub_bignum2flonum)
{
#ifdef HAVE_GMP_H
  value v1, v2, diff;
  carc c;

  c.get_cell = get_cell_test;

  v1 = carc_mkflonum(&c, 0.0);
  v2 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(v2)._bignum, "10000000000000000000000", 10);
  diff = __carc_sub2(&c, v1, v2);
  fail_unless(TYPE(diff) == T_FLONUM);
  fail_unless(fabs(REP(diff)._flonum + 1e22) < 1e-6);

  v1 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(v1)._bignum, "10000000000000000000000", 10);
  v2 = carc_mkflonum(&c, 0.0);
  diff = __carc_sub2(&c, v1, v2);
  fail_unless(TYPE(diff) == T_FLONUM);
  fail_unless(fabs(REP(diff)._flonum - 1e22) < 1e-6);
#endif
}
END_TEST

START_TEST(test_sub_bignum2rational)
{
#ifdef HAVE_GMP_H
  value v1, v2, diff;
  carc c;
  mpq_t expected;

  c.get_cell = get_cell_test;

  v1 = carc_mkrationall(&c, 1, 2);
  v2 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(v2)._bignum, "100000000000000000000000000000", 10);
  diff = __carc_sub2(&c, v1, v2);
  fail_unless(TYPE(diff) == T_RATIONAL);
  mpq_init(expected);
  mpq_set_str(expected, "-199999999999999999999999999999/2", 10);
  fail_unless(mpq_cmp(expected, REP(diff)._rational) == 0);
  mpq_clear(expected);

  v1 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(v1)._bignum, "100000000000000000000000000000", 10);
  v2 = carc_mkrationall(&c, 1, 2);
  diff = __carc_sub2(&c, v1, v2);
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
  carc c;

  c.get_cell = get_cell_test;

  v1 = carc_mkcomplex(&c, 0.0, 1.1);
  v2 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(v2)._bignum, "10000000000000000000000", 10);
  diff = __carc_sub2(&c, v1, v2);
  fail_unless(TYPE(diff) == T_COMPLEX);
  fail_unless(fabs(REP(diff)._complex.re + 1e22) < 1e-6);
  fail_unless(fabs(REP(diff)._complex.im - 1.1) < 1e-6);

  v1 = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(v1)._bignum, "10000000000000000000000", 10);
  v2 = carc_mkcomplex(&c, 0.0, 1.1);
  diff = __carc_sub2(&c, v1, v2);
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
  carc c;

  c.get_cell = get_cell_test;

  val1 = carc_mkflonum(&c, 0.5);
  val2 = carc_mkrationall(&c, 1, 2);
  diff = __carc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_FLONUM);
  fail_unless(fabs(REP(diff)._flonum) < 1e-6);

  val1 = carc_mkrationall(&c, 1, 2);
  val2 = carc_mkflonum(&c, 0.5);
  diff = __carc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_FLONUM);
  fail_unless(fabs(REP(diff)._flonum) < 1e-6);
#endif
}
END_TEST

START_TEST(test_sub_flonum2complex)
{
  value val1, val2, diff;
  carc c;

  c.get_cell = get_cell_test;

  val1 = carc_mkflonum(&c, 0.5);
  val2 = carc_mkcomplex(&c, 3, -4);
  diff = __carc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_COMPLEX);
  fail_unless(fabs(-2.5 - REP(diff)._complex.re) < 1e-6);
  fail_unless(fabs(4 - REP(diff)._complex.im) < 1e-6);

  val1 = carc_mkcomplex(&c, 3, -4);
  val2 = carc_mkflonum(&c, 0.5);
  diff = __carc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_COMPLEX);
  fail_unless(fabs(2.5 - REP(diff)._complex.re) < 1e-6);
  fail_unless(fabs(-4 - REP(diff)._complex.im) < 1e-6);

}
END_TEST

START_TEST(test_sub_rational2complex)
{
#ifdef HAVE_GMP_H
  value val1, val2, diff;
  carc c;

  c.get_cell = get_cell_test;

  val1 = carc_mkcomplex(&c, 0.5, 0.5);
  val2 = carc_mkrationall(&c, 1, 2);
  diff = __carc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_COMPLEX);
  fail_unless(fabs(REP(diff)._complex.re) < 1e-6);
  fail_unless(fabs(0.5 - REP(diff)._complex.im) < 1e-6);

  val1 = carc_mkrationall(&c, 1, 2);
  val2 = carc_mkcomplex(&c, 0.5, 0.5);
  diff = __carc_sub2(&c, val1, val2);
  fail_unless(TYPE(diff) == T_COMPLEX);
  fail_unless(fabs(REP(diff)._complex.re) < 1e-6);
  fail_unless(fabs(-0.5 - REP(diff)._complex.im) < 1e-6);
#endif
}
END_TEST

START_TEST(test_sub_misc)
{
  carc c;
  value diff;

  c.get_cell = get_cell_test;
  c.signal_error = signal_error_test;
  error = 0;

  diff = __carc_sub2(&c, CNIL, CNIL);
  fail_unless(error == 1);
  error = 0;

  diff = __carc_sub2(&c, cons(&c, FIX2INT(1), CNIL), 
		    cons(&c, FIX2INT(2), CNIL));
  fail_unless(error == 1);
}
END_TEST

START_TEST(test_neg)
{
  carc c;
  value v, neg;
#ifdef HAVE_GMP_H
  mpz_t expected;
#endif

  c.get_cell = get_cell_test;
  c.signal_error = signal_error_test;
  error = 0;

  v = INT2FIX(1);
  neg = __carc_neg(&c, v);
  fail_unless(TYPE(neg) == T_FIXNUM);
  fail_unless(FIX2INT(neg) == -1);

  v = carc_mkflonum(&c, -1.234);
  neg = __carc_neg(&c, v);
  fail_unless(TYPE(neg) == T_FLONUM);
  fail_unless(fabs(REP(neg)._flonum - 1.234) < 1e-6);

  v = carc_mkcomplex(&c, -1.234, 5.678);
  neg = __carc_neg(&c, v);
  fail_unless(TYPE(neg) == T_COMPLEX);
  fail_unless(fabs(REP(neg)._complex.re - 1.234) < 1e-6);
  fail_unless(fabs(REP(neg)._complex.im + 5.678) < 1e-6);

#ifdef HAVE_GMP_H
  v = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(v)._bignum, "100000000000000000000000000000", 10);
  neg = __carc_neg(&c, v);
  mpz_init(expected);
  mpz_set_str(expected, "-100000000000000000000000000000", 10);
  fail_unless(TYPE(neg) == T_BIGNUM);
  fail_unless(mpz_cmp(expected, REP(neg)._bignum) == 0);
  mpz_clear(expected);

  v = carc_mkrationall(&c, 1, 2);
  neg = __carc_neg(&c, v);
  fail_unless(TYPE(neg) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REP(neg)._rational, -1, 2) == 0);
#endif

  neg = __carc_neg(&c, CNIL);
  fail_unless(error == 1);
  fail_unless(neg == CNIL);
}
END_TEST

START_TEST(test_coerce_flonum)
{
  carc c;
  double d;
  value v;

  c.get_cell = get_cell_test;
  c.signal_error = signal_error_test;

#ifdef HAVE_GMP_H
  v = carc_mkbignuml(&c, 1);
  mpz_set_str(REP(v)._bignum, "100000000000000000000000000000", 10);
  d = carc_coerce_flonum(&c, v);
  fail_unless(fabs(1e29 - d) < 1e-6);
#endif

  v = carc_mkflonum(&c, 3.14159);
  d = carc_coerce_flonum(&c, v);
  fail_unless(fabs(3.14159 - d) < 1e-6);
  v = INT2FIX(0xdeadbeef);
  d = carc_coerce_flonum(&c, v);
  fail_unless(fabs(3735928559.0 - d) < 1e-6);
  v = c.get_cell(&c);
  BTYPE(v) = T_CONS;
  d = carc_coerce_flonum(&c, v);
  fail_unless(error == 1);
  error = 0;
}
END_TEST

START_TEST(test_coerce_fixnum)
{
  carc c;
  value v, v2;

  c.get_cell = get_cell_test;
  c.signal_error = signal_error_test;
  error = 0;

  /* Identity */
  v = INT2FIX(1);
  v2 = carc_coerce_fixnum(&c, v);
  fail_unless(TYPE(v2) == T_FIXNUM);
  fail_unless(FIX2INT(v2) == 1);

  /* Truncates */
  v = carc_mkflonum(&c, 3.14159);
  v2 = carc_coerce_fixnum(&c, v);
  fail_unless(TYPE(v2) == T_FIXNUM);
  fail_unless(FIX2INT(v2) == 3);

  /* Fails conversion (too big) */
  v = carc_mkflonum(&c, 1e100);
  v2 = carc_coerce_fixnum(&c, v);
  fail_unless(v2 == CNIL);
  fail_unless(TYPE(v2) == T_NIL);

  /* Should also fail conversion */
  v = carc_mkflonum(&c, ((double)(FIXNUM_MAX))*2);
  v2 = carc_coerce_fixnum(&c, v);
  fail_unless(v2 == CNIL);
  fail_unless(TYPE(v2) == T_NIL);

#ifdef HAVE_GMP_H
  /* Small bignum should be converted */
  v = carc_mkbignuml(&c, 1000);
  v2 = carc_coerce_fixnum(&c, v);
  fail_unless(TYPE(v2) == T_FIXNUM);
  fail_unless(FIX2INT(v2) == 1000);

  /* too big to convert to fixnum */
  v = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(v)._bignum, "100000000000000000000000000000", 10);
  v2 = carc_coerce_fixnum(&c, v);
  fail_unless(v2 == CNIL);
  fail_unless(TYPE(v2) == T_NIL);
#endif
}
END_TEST

START_TEST(test_coerce_bignum)
{
#ifdef HAVE_GMP_H
  carc c;
  value v;
  mpz_t v2;

  c.get_cell = get_cell_test;
  c.signal_error = signal_error_test;
  error = 0;
  mpz_init(v2);
  v = carc_mkbignuml(&c, 1000);
  carc_coerce_bignum(&c, v, &v2);
  fail_unless(mpz_cmp(REP(v)._bignum, v2) == 0);

  v = carc_mkflonum(&c, 3.14159);
  carc_coerce_bignum(&c, v, &v2);
  fail_unless(fabs(mpz_get_d(v2) - 3.0) < 1e-6);

  v = INT2FIX(32);
  carc_coerce_bignum(&c, v, &v2);
  fail_unless(mpz_get_si(v2) == 32);

  v = cons(&c, 1,2);
  carc_coerce_bignum(&c, v, &v2);
  fail_unless(error == 1);
  error = 0;

  v = carc_mkcomplex(&c, 3.14159, 2.71828);
  carc_coerce_bignum(&c, v, &v2);
  fail_unless(error == 1);
  error = 0;

  mpz_clear(v2);
#endif
}
END_TEST

START_TEST(test_coerce_rational)
{
#ifdef HAVE_GMP_H
  carc c;
  value v;
  mpq_t v2;

  c.get_cell = get_cell_test;
  c.signal_error = signal_error_test;

  mpq_init(v2);

  v = carc_mkrationall(&c, 1, 2);
  carc_coerce_rational(&c, v, &v2);
  fail_unless(mpq_cmp(REP(v)._rational, v2) == 0);

  error = 0;
  v = carc_mkbignuml(&c, 1000);
  carc_coerce_rational(&c, v, &v2);
  fail_unless(mpq_cmp_si(v2, 1000, 1) == 0);

  v = carc_mkflonum(&c, 3.14159);
  carc_coerce_rational(&c, v, &v2);
  fail_unless(fabs(mpq_get_d(v2) - 3.14159) < 1e-6);

  v = INT2FIX(32);
  carc_coerce_rational(&c, v, &v2);
  fail_unless(mpq_cmp_si(v2, 32, 1) == 0);

  v = cons(&c, 1,2);
  carc_coerce_rational(&c, v, &v2);
  fail_unless(error == 1);
  error = 0;

  v = carc_mkcomplex(&c, 3.14159, 2.71828);
  carc_coerce_rational(&c, v, &v2);
  fail_unless(error == 1);
  error = 0;
#endif
}
END_TEST

START_TEST(test_coerce_complex)
{
  carc c;
  double re, im;
  value v;

  c.get_cell = get_cell_test;
  c.signal_error = signal_error_test;

#ifdef HAVE_GMP_H
  v = carc_mkbignuml(&c, 1);
  mpz_set_str(REP(v)._bignum, "100000000000000000000000000000", 10);
  carc_coerce_complex(&c, v, &re, &im);
  fail_unless(fabs(1e29 - re) < 1e-6);
  fail_unless(fabs(im) < 1e-6);
#endif

  v = carc_mkcomplex(&c, 3.14159, 2.71828);
  carc_coerce_complex(&c, v, &re, &im);
  fail_unless(fabs(3.14159 - re) < 1e-6);
  fail_unless(fabs(2.71828 - im) < 1e-6);

  v = carc_mkflonum(&c, 3.14159);
  carc_coerce_complex(&c, v, &re, &im);
  fail_unless(fabs(3.14159 - re) < 1e-6);
  fail_unless(fabs(im) < 1e-6);

  v = INT2FIX(0xdeadbeef);
  carc_coerce_complex(&c, v, &re, &im);
  fail_unless(fabs(3735928559.0 - re) < 1e-6);
  fail_unless(fabs(im) < 1e-6);

  v = c.get_cell(&c);
  BTYPE(v) = T_CONS;
  carc_coerce_complex(&c, v, &re, &im);
  fail_unless(error == 1);
  error = 0;
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

  tcase_add_test(tc_conv, test_coerce_fixnum);
  tcase_add_test(tc_conv, test_coerce_flonum);
  tcase_add_test(tc_conv, test_coerce_bignum);
  tcase_add_test(tc_conv, test_coerce_rational);
  tcase_add_test(tc_conv, test_coerce_complex);

  suite_add_tcase(s, tc_ops);
  suite_add_tcase(s, tc_conv);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

