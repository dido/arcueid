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
  fail_unless(mpz_get_si(mpq_denref(REP(sum)._bignum)) == 1);
  fail_unless(mpz_get_si(mpq_numref(REP(sum)._bignum)) == FIXNUM_MAX + 1);

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
  mpq_t expected;

  c.get_cell = get_cell_test;
  val1 = carc_mkbignuml(&c, FIXNUM_MAX+1);
  val2 = carc_mkbignuml(&c, -(FIXNUM_MAX+2));
  sum = __carc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_FIXNUM);
  fail_unless(FIX2INT(sum) == -1);

  val1 = carc_mkbignuml(&c, 0);
  mpq_set_str(REP(val1)._bignum, "100000000000000000000000000000", 10);
  val2 = carc_mkbignuml(&c, 0);
  mpq_set_str(REP(val2)._bignum, "200000000000000000000000000000", 10);
  sum = __carc_add2(&c, val1, val2);
  fail_unless(TYPE(sum) == T_BIGNUM);
  mpq_init(expected);
  mpq_set_str(expected, "300000000000000000000000000000", 10);
  fail_unless(mpq_equal(expected, REP(sum)._bignum));
#endif
}
END_TEST

START_TEST(test_add_misc)
{
  carc c;
  value v, sum;

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
  mpq_t expected;
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
  mpq_init(expected);
  mpq_set_si(expected, 2*FIXNUM_MAX, 1);
  fail_unless(mpq_equal(expected, REP(prod)._bignum));

  prod = __carc_mul2(&c, INT2FIX(-2), INT2FIX(-FIXNUM_MAX));
  fail_unless(TYPE(prod) == T_BIGNUM);
  mpq_init(expected);
  mpq_set_si(expected, 2*FIXNUM_MAX, 1);
  fail_unless(mpq_equal(expected, REP(prod)._bignum));

  prod = __carc_mul2(&c, INT2FIX(-2), INT2FIX(FIXNUM_MAX));
  fail_unless(TYPE(prod) == T_BIGNUM);
  mpq_init(expected);
  mpq_set_si(expected, -2*FIXNUM_MAX, 1);
  fail_unless(mpq_equal(expected, REP(prod)._bignum));

  prod = __carc_mul2(&c, INT2FIX(2), INT2FIX(-FIXNUM_MAX));
  fail_unless(TYPE(prod) == T_BIGNUM);
  mpq_init(expected);
  mpq_set_si(expected, -2*FIXNUM_MAX, 1);
  fail_unless(mpq_equal(expected, REP(prod)._bignum));
#else
  prod = __carc_mul2(&c, INT2FIX(2), INT2FIX(FIXNUM_MAX));
  fail_unless(TYPE(prod) == T_NIL);
  fail_unless(error == 1);
#endif

}
END_TEST

START_TEST(test_neg)
{
  carc c;
  value v, neg;
#ifdef HAVE_GMP_H
  mpq_t expected;
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

#ifdef HAVE_GMP_H
  v = carc_mkbignuml(&c, 0);
  mpq_set_str(REP(v)._bignum, "100000000000000000000000000000", 10);
  neg = __carc_neg(&c, v);
  mpq_init(expected);
  mpq_set_str(expected, "-100000000000000000000000000000", 10);
  fail_unless(TYPE(neg) == T_BIGNUM);
  fail_unless(mpq_equal(expected, REP(neg)._bignum));
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
  mpq_set_str(REP(v)._bignum, "100000000000000000000000000000", 10);
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
  mpq_set_str(REP(v)._bignum, "100000000000000000000000000000", 10);
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
  mpq_t v2;

  c.get_cell = get_cell_test;
  c.signal_error = signal_error_test;
  error = 0;
  mpq_init(v2);
  v = carc_mkbignuml(&c, 1000);
  carc_coerce_bignum(&c, v, &v2);
  fail_unless(mpq_equal(REP(v)._bignum, v2));

  v = carc_mkflonum(&c, 3.14159);
  carc_coerce_bignum(&c, v, &v2);
  fail_unless(fabs(mpq_get_d(v2) - 3.14159) < 1e-6);

  v = INT2FIX(32);
  carc_coerce_bignum(&c, v, &v2);
  fail_unless(mpz_get_si(mpq_numref(v2)) == 32);
  fail_unless(mpz_get_si(mpq_denref(v2)) == 1);

  v = cons(&c, 1,2);
  carc_coerce_bignum(&c, v, &v2);
  fail_unless(error == 1);
  error = 0;
#endif
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Arithmetic");
  TCase *tc_ops = tcase_create("Operators");
  TCase *tc_conv = tcase_create("Conversions");
  SRunner *sr;

  tcase_add_test(tc_ops, test_add_fixnum);
  tcase_add_test(tc_ops, test_add_bignum);
  tcase_add_test(tc_ops, test_add_fixnum2bignum);
  tcase_add_test(tc_ops, test_add_fixnum2flonum);
  tcase_add_test(tc_ops, test_add_misc);
  tcase_add_test(tc_ops, test_neg);
  tcase_add_test(tc_ops, test_mul_fixnum);

  tcase_add_test(tc_conv, test_coerce_fixnum);
  tcase_add_test(tc_conv, test_coerce_flonum);
  tcase_add_test(tc_conv, test_coerce_bignum);

  suite_add_tcase(s, tc_ops);
  suite_add_tcase(s, tc_conv);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
