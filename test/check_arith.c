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
#include "../src/carc.h"
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

value get_cell_test_add_fixnum2bignum(struct carc *c)
{
  static struct cell cellone;

  return((value)&cellone);
}

START_TEST(test_add_fixnum2bignum)
{
#ifdef HAVE_GMP_H
  struct cell c1, c2;
  value value1, value2, maxfixnum, one, sum;
  carc c;
  double d;

  c.get_cell = get_cell_test_add_fixnum2bignum;

  value1 = (value)&c1;
  value2 = (value)&c2;
  maxfixnum = INT2FIX(FIXNUM_MAX);
  one = INT2FIX(1);
  car(value1) = maxfixnum;
  cdr(value1) = value2;
  car(value2) = one;
  cdr(value2) = CNIL;
  sum = carc_arith_op(&c, '+', value1);
  fail_unless(TYPE(sum) == T_BIGNUM);
  d = mpq_get_d(REP(sum)._bignum);
  fail_unless(fabs((d - 1.) - (double)FIXNUM_MAX) < 1e-6);
#endif
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Arithmetic");
  TCase *tc_ops = tcase_create("Operators");
  SRunner *sr;

  tcase_add_test(tc_ops, test_add_fixnum);
  tcase_add_test(tc_ops, test_add_fixnum2bignum);

  suite_add_tcase(s, tc_ops);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
