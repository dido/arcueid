/* 
  Copyright (C) 2017,2018 Rafael R. Sevilla

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
  along with this program; if not, see <http://www.gnu.org/licenses/>.
*/
#include "../config.h"
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include <math.h>
#include <stdio.h>
#include "../src/arcueid.h"

START_TEST(test_cons)
{
  arc cc;
  arc *c = &cc;
  value r;

  arc_init(c);
  r = cons(c, cons(c, INT2FIX(1), CNIL),
	   cons(c, INT2FIX(2), cons(c, INT2FIX(3), CNIL)));
  ck_assert((r & 0x0f) == 0);
  ck_assert(((car(r)) & 0x0f) == 0);
  ck_assert(((car(car(r))) & 0x0f) != 0);
  ck_assert(arc_type(r) == &__arc_cons_t);
  ck_assert(arc_type(car(r)) == &__arc_cons_t);
  ck_assert(arc_type(car(car(r))) == &__arc_fixnum_t);
  ck_assert_int_eq(FIX2INT(car(car(r))), 1);
  ck_assert(arc_type(car(car(r))) == &__arc_fixnum_t);
  ck_assert(NILP(cdr(car(r))));

  ck_assert(arc_type(cdr(r)) == &__arc_cons_t);
  ck_assert(arc_type(car(cdr(r))) == &__arc_fixnum_t);
  ck_assert_int_eq(FIX2INT(car(cdr(r))), 2);
  ck_assert(arc_type(cdr(cdr(r))) == &__arc_cons_t);
  ck_assert(arc_type(car(cdr(cdr(r)))) == &__arc_fixnum_t);
  ck_assert_int_eq(FIX2INT(car(cdr(cdr(r)))), 3);
  ck_assert(arc_type(cdr(cdr(cdr(r)))) == &__arc_nil_t);

  scar(c, r, INT2FIX(4));
  ck_assert(arc_type(car(r)) == &__arc_fixnum_t);
  ck_assert_int_eq(FIX2INT(car(r)), 4);

  scdr(c, r, INT2FIX(5));
  ck_assert(arc_type(cdr(r)) == &__arc_fixnum_t);
  ck_assert_int_eq(FIX2INT(cdr(r)), 5);
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Conses");
  TCase *tc_cons = tcase_create("Conses");
  SRunner *sr;

  tcase_add_test(tc_cons, test_cons);
  suite_add_tcase(s, tc_cons);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
