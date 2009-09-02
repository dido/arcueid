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
  struct cell conses[4];
  value head, val;
  int i;
  carc c;

  /* Create a list of numbers */
  val = head = (value)&conses[0];
  for (i=0; i<4; i++) {
    car(val) = INT2FIX(i+1);
    cdr(val) = (i == 3) ? CNIL : ((value)&conses[i+1]);
    val = cdr(val);
  }
  val = carc_arith_op(&c, '+', head);
  fail_unless(FIX2INT(val) == 10);
}
END_TEST

value get_cell_test(struct carc *c)
{
  static struct cell cells[1024];
  static struct cell *cellptr = cells;

  return((value)cellptr++);
}

START_TEST(test_add_fixnum2bignum)
{
#ifdef HAVE_GMP_H
  struct cell c1, c2;
  value value1, value2, maxfixnum, one, negone, sum;
  carc c;

  c.get_cell = get_cell_test;

  value1 = (value)&c1;
  value2 = (value)&c2;
  maxfixnum = INT2FIX(FIXNUM_MAX);
  one = INT2FIX(1);
  negone = INT2FIX(-1);
  car(value1) = maxfixnum;
  cdr(value1) = value2;
  car(value2) = one;
  cdr(value2) = CNIL;
  sum = carc_arith_op(&c, '+', value1);
  fail_unless(TYPE(sum) == T_BIGNUM);
  fail_unless(mpz_get_si(mpq_denref(REP(sum)._bignum)) == 1);
  fail_unless(mpz_get_si(mpq_numref(REP(sum)._bignum)) == FIXNUM_MAX + 1);

  car(value1) = sum;
  cdr(value1) = value2;
  car(value2) = negone;
  cdr(value2) = CNIL;
  sum = carc_arith_op(&c, '+', value1);
  fail_unless(TYPE(sum) == T_FIXNUM);
  fail_unless(FIX2INT(sum) == FIXNUM_MAX);
#endif
}
END_TEST

START_TEST(test_add_fixnum2flonum)
{
  value list, val1, val2, sum;
  carc c;

  c.get_cell = get_cell_test;

  val1 = INT2FIX(1);
  val2 = carc_mkflonum(&c, 3.14159);
  list = get_cell_test(&c);

  car(list) = val1;
  cdr(list) = get_cell_test(&c);
  car(cdr(list)) = val2;
  cdr(cdr(list)) = CNIL;
  sum = carc_arith_op(&c, '+', list);
  fail_unless(TYPE(sum) == T_FLONUM);
  fail_unless(fabs(4.14159 - REP(sum)._flonum) < 1e-6);

  val1 = INT2FIX(-1);
  car(list) = sum;
  car(cdr(list)) = val1;
  sum = carc_arith_op(&c, '+', list);
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
  value list, val1, val2, sum;
  carc c;
  mpq_t expected;

  c.get_cell = get_cell_test;
  val1 = carc_mkbignuml(&c, FIXNUM_MAX+1);
  val2 = carc_mkbignuml(&c, -(FIXNUM_MAX+2));
  list = get_cell_test(&c);
  car(list) = val1;
  cdr(list) = get_cell_test(&c);
  car(cdr(list)) = val2;
  cdr(cdr(list)) = CNIL;
  sum = carc_arith_op(&c, '+', list);
  fail_unless(TYPE(sum) == T_FIXNUM);
  fail_unless(FIX2INT(sum) == -1);

  val1 = carc_mkbignuml(&c, 0);
  mpq_set_str(REP(val1)._bignum, "100000000000000000000000000000", 10);
  val2 = carc_mkbignuml(&c, 0);
  mpq_set_str(REP(val2)._bignum, "200000000000000000000000000000", 10);

  car(list) = val1;
  cdr(list) = get_cell_test(&c);
  car(cdr(list)) = val2;
  cdr(cdr(list)) = CNIL;
  sum = carc_arith_op(&c, '+', list);
  fail_unless(TYPE(sum) == T_BIGNUM);
  mpq_init(expected);
  mpq_set_str(expected, "300000000000000000000000000000", 10);
  fail_if(!mpq_equal(expected, REP(sum)._bignum));
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

  v = cons(&c, cons(&c, CNIL, CNIL), cons(&c, CNIL, CNIL));
  sum = carc_arith_op(&c, '+', v);
  fail_unless(error == 1);
  error = 0;

  sum = carc_arith_op(&c, '$', v);
  fail_unless(error == 1);
  error = 0;

}
END_TEST

START_TEST(test_neg)
{
  carc c;
  value v, neg;
  mpq_t expected;

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

  v = carc_mkbignuml(&c, 0);
  mpq_set_str(REP(v)._bignum, "100000000000000000000000000000", 10);
  neg = __carc_neg(&c, v);
  mpq_init(expected);
  mpq_set_str(expected, "-100000000000000000000000000000", 10);
  fail_unless(TYPE(neg) == T_BIGNUM);
  fail_unless(mpq_equal(expected, REP(neg)._bignum));
}
END_TEST

START_TEST(test_coerce_flonum)
{
  carc c;
  double d;
  value v;

  c.get_cell = get_cell_test;
  c.signal_error = signal_error_test;
  v = carc_mkbignuml(&c, 1);
  mpq_set_str(REP(v)._bignum, "100000000000000000000000000000", 10);
  d = carc_coerce_flonum(&c, v);
  fail_unless(fabs(1e29 - d) < 1e-6);
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
