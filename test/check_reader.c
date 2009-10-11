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
#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include "../src/carc.h"

carc c;

START_TEST(test_atom)
{
  value str, sexpr;
  int index;

  str = carc_mkstringc(&c, "0");
  fail_if(carc_read(&c, str, &index, &sexpr) == CNIL);
  fail_unless(TYPE(sexpr) == T_FIXNUM);
  fail_unless(FIX2INT(sexpr) == 0);

  str = carc_mkstringc(&c, "foo");
  fail_if(carc_read(&c, str, &index, &sexpr) == CNIL);
  fail_unless(TYPE(sexpr) == T_SYMBOL);
  fail_unless(carc_is(&c, str, carc_sym2name(&c, sexpr)) == CTRUE);
}
END_TEST

START_TEST(test_list)
{
  value str, sexpr;
  int index;

  index = 0;
  str = carc_mkstringc(&c, "(foo 1 2 3)");
  fail_if(carc_read(&c, str, &index, &sexpr) == CNIL);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(TYPE(car(sexpr)) == T_SYMBOL);
  fail_unless(carc_is(&c, carc_mkstringc(&c, "foo"),
		      carc_sym2name(&c, car(sexpr))) == CTRUE);
  fail_unless(TYPE(car(cdr(sexpr))) == T_FIXNUM);
  fail_unless(FIX2INT((car(cdr(sexpr)))) == 1);
  fail_unless(TYPE(car(cdr(cdr(sexpr)))) == T_FIXNUM);
  fail_unless(FIX2INT((car(cdr(cdr(sexpr))))) == 2);
  fail_unless(TYPE(car(cdr(cdr(cdr(sexpr))))) == T_FIXNUM);
  fail_unless(FIX2INT((car(cdr(cdr(cdr(sexpr)))))) == 3);

  index = 0;
  str = carc_mkstringc(&c, "(foo (bar 4) 5)");
  fail_if(carc_read(&c, str, &index, &sexpr) == CNIL);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(TYPE(car(sexpr)) == T_SYMBOL);
  fail_unless(carc_is(&c, carc_mkstringc(&c, "foo"),
		      carc_sym2name(&c, car(sexpr))) == CTRUE);
  fail_unless(TYPE(car(cdr(sexpr))) == T_CONS);
  fail_unless(TYPE(car(car(cdr(sexpr)))) == T_SYMBOL);
  fail_unless(carc_is(&c, carc_mkstringc(&c, "bar"),
		      carc_sym2name(&c, car(car(cdr(sexpr))))) == CTRUE);
  fail_unless(TYPE(car(cdr(car(cdr(sexpr))))) == T_FIXNUM);
  fail_unless(FIX2INT(car(cdr(car(cdr(sexpr))))) == 4);
  fail_unless(TYPE(car(cdr(cdr(sexpr)))) == T_FIXNUM);
  fail_unless(FIX2INT(car(cdr(cdr(sexpr)))) == 5);
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Reader");
  TCase *tc_reader = tcase_create("Reader");
  SRunner *sr;

  carc_set_memmgr(&c);
  carc_init_reader(&c);
  tcase_add_test(tc_reader, test_atom);
  tcase_add_test(tc_reader, test_list);

  suite_add_tcase(s, tc_reader);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
