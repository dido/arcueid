/* Copyright (C) 2017, 2018 Rafael R. Sevilla

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
#include <stdlib.h>
#include <check.h>
#include <stdint.h>
#include <stdio.h>
#include "../src/arcueid.h"
#include "../src/vmengine.h"

START_TEST(test_env_simple)
{
  arc cc;
  arc *c = &cc;
  value thr;

  arc_init(c);
  thr = __arc_thread_new(c, 1);
  CPUSH(thr, INT2FIX(1));
  CPUSH(thr, INT2FIX(2));
  CPUSH(thr, INT2FIX(3));
  __arc_env_new(c, thr, 3, 3);
  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 0, 0)), 1);
  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 0, 1)), 2);
  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 0, 2)), 3);
  ck_assert(__arc_getenv(c, thr, 0, 3) == CUNBOUND);
  ck_assert(__arc_getenv(c, thr, 0, 4) == CUNBOUND);
  ck_assert(__arc_getenv(c, thr, 0, 5) == CUNBOUND);

  ck_assert_int_eq(FIX2INT(__arc_getenv0(c, thr, 0)), 1);
  ck_assert_int_eq(FIX2INT(__arc_getenv0(c, thr, 1)), 2);
  ck_assert_int_eq(FIX2INT(__arc_getenv0(c, thr, 2)), 3);
  ck_assert(__arc_getenv0(c, thr, 3) == CUNBOUND);
  ck_assert(__arc_getenv0(c, thr, 4) == CUNBOUND);
  ck_assert(__arc_getenv0(c, thr, 5) == CUNBOUND);

  __arc_putenv(c, thr, 0, 3, INT2FIX(4));
  __arc_putenv0(c, thr, 4, INT2FIX(5));
  __arc_putenv0(c, thr, 5, INT2FIX(6));

  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 0, 3)), 4);
  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 0, 4)), 5);
  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 0, 5)), 6);

  CPUSH(thr, INT2FIX(7));
  CPUSH(thr, INT2FIX(8));
  CPUSH(thr, INT2FIX(9));
  CPUSH(thr, INT2FIX(10));
  __arc_env_new(c, thr, 4, 0);

  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 0, 0)), 7);
  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 0, 1)), 8);
  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 0, 2)), 9);
  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 0, 3)), 10);

  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 1, 0)), 1);
  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 1, 1)), 2);
  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 1, 2)), 3);
  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 1, 3)), 4);
  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 1, 4)), 5);
  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 1, 5)), 6);

  CPUSH(thr, INT2FIX(11));
  CPUSH(thr, INT2FIX(12));
  CPUSH(thr, INT2FIX(13));
  CPUSH(thr, INT2FIX(14));
  CPUSH(thr, INT2FIX(15));
  __arc_env_new(c, thr, 5, 0);

  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 0, 0)), 11);
  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 0, 1)), 12);
  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 0, 2)), 13);
  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 0, 3)), 14);
  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 0, 4)), 15);

  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 1, 0)), 7);
  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 1, 1)), 8);
  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 1, 2)), 9);
  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 1, 3)), 10);

  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 2, 0)), 1);
  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 2, 1)), 2);
  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 2, 2)), 3);
  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 2, 3)), 4);
  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 2, 4)), 5);
  ck_assert_int_eq(FIX2INT(__arc_getenv(c, thr, 2, 5)), 6);
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Environments");
  TCase *tc_env = tcase_create("Environments");
  SRunner *sr;

  tcase_add_test(tc_env, test_env_simple);

  suite_add_tcase(s, tc_env);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
