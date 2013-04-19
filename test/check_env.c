/* 
  Copyright (C) 2013 Rafael R. Sevilla

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
#include <check.h>
#include "../src/arcueid.h"
#include "../src/vmengine.h"

arc cc;
arc *c;

START_TEST(test_env_simple)
{
  value thr;

  thr = arc_mkthread(c);
  CPUSH(thr, INT2FIX(1));
  CPUSH(thr, INT2FIX(2));
  CPUSH(thr, INT2FIX(3));
  __arc_mkenv(c, thr, 3, 3);
  fail_unless(__arc_getenv(c, thr, 0, 0) == INT2FIX(1));
  fail_unless(__arc_getenv(c, thr, 0, 1) == INT2FIX(2));
  fail_unless(__arc_getenv(c, thr, 0, 2) == INT2FIX(3));
  fail_unless(__arc_getenv(c, thr, 0, 3) == CUNBOUND);
  fail_unless(__arc_getenv(c, thr, 0, 4) == CUNBOUND);
  fail_unless(__arc_getenv(c, thr, 0, 5) == CUNBOUND);

  __arc_putenv(c, thr, 0, 3, INT2FIX(4));
  __arc_putenv(c, thr, 0, 4, INT2FIX(5));
  __arc_putenv(c, thr, 0, 5, INT2FIX(6));


  CPUSH(thr, INT2FIX(7));
  CPUSH(thr, INT2FIX(8));
  CPUSH(thr, INT2FIX(9));
  CPUSH(thr, INT2FIX(10));
  __arc_mkenv(c, thr, 4, 0);
  fail_unless(__arc_getenv(c, thr, 0, 0) == INT2FIX(7));
  fail_unless(__arc_getenv(c, thr, 0, 1) == INT2FIX(8));
  fail_unless(__arc_getenv(c, thr, 0, 2) == INT2FIX(9));
  fail_unless(__arc_getenv(c, thr, 0, 3) == INT2FIX(10));

  fail_unless(__arc_getenv(c, thr, 1, 0) == INT2FIX(1));
  fail_unless(__arc_getenv(c, thr, 1, 1) == INT2FIX(2));
  fail_unless(__arc_getenv(c, thr, 1, 2) == INT2FIX(3));
  fail_unless(__arc_getenv(c, thr, 1, 3) == INT2FIX(4));
  fail_unless(__arc_getenv(c, thr, 1, 4) == INT2FIX(5));
  fail_unless(__arc_getenv(c, thr, 1, 5) == INT2FIX(6));

  CPUSH(thr, INT2FIX(11));
  CPUSH(thr, INT2FIX(12));
  CPUSH(thr, INT2FIX(13));
  CPUSH(thr, INT2FIX(14));
  CPUSH(thr, INT2FIX(15));
  __arc_mkenv(c, thr, 5, 0);
  fail_unless(__arc_getenv(c, thr, 0, 0) == INT2FIX(11));
  fail_unless(__arc_getenv(c, thr, 0, 1) == INT2FIX(12));
  fail_unless(__arc_getenv(c, thr, 0, 2) == INT2FIX(13));
  fail_unless(__arc_getenv(c, thr, 0, 3) == INT2FIX(14));
  fail_unless(__arc_getenv(c, thr, 0, 4) == INT2FIX(15));

  fail_unless(__arc_getenv(c, thr, 1, 0) == INT2FIX(7));
  fail_unless(__arc_getenv(c, thr, 1, 1) == INT2FIX(8));
  fail_unless(__arc_getenv(c, thr, 1, 2) == INT2FIX(9));
  fail_unless(__arc_getenv(c, thr, 1, 3) == INT2FIX(10));

  fail_unless(__arc_getenv(c, thr, 2, 0) == INT2FIX(1));
  fail_unless(__arc_getenv(c, thr, 2, 1) == INT2FIX(2));
  fail_unless(__arc_getenv(c, thr, 2, 2) == INT2FIX(3));
  fail_unless(__arc_getenv(c, thr, 2, 3) == INT2FIX(4));
  fail_unless(__arc_getenv(c, thr, 2, 4) == INT2FIX(5));
  fail_unless(__arc_getenv(c, thr, 2, 5) == INT2FIX(6));
}
END_TEST

START_TEST(test_menv)
{
  value thr;

  /* New environment is just as big as the old environment */
  thr = arc_mkthread(c);
  CPUSH(thr, INT2FIX(1));
  CPUSH(thr, INT2FIX(2));
  CPUSH(thr, INT2FIX(3));
  __arc_mkenv(c, thr, 3, 0);
  fail_unless(__arc_getenv(c, thr, 0, 0) == INT2FIX(1));
  fail_unless(__arc_getenv(c, thr, 0, 1) == INT2FIX(2));
  fail_unless(__arc_getenv(c, thr, 0, 2) == INT2FIX(3));

  CPUSH(thr, INT2FIX(4));
  CPUSH(thr, INT2FIX(5));
  CPUSH(thr, INT2FIX(6));
  __arc_menv(c, thr, 3);
  __arc_mkenv(c, thr, 3, 0);
  fail_unless(__arc_getenv(c, thr, 0, 0) == INT2FIX(4));
  fail_unless(__arc_getenv(c, thr, 0, 1) == INT2FIX(5));
  fail_unless(__arc_getenv(c, thr, 0, 2) == INT2FIX(6));

  /* New environment is smaller than the old environment */
  thr = arc_mkthread(c);
  CPUSH(thr, INT2FIX(1));
  CPUSH(thr, INT2FIX(2));
  CPUSH(thr, INT2FIX(3));
  __arc_mkenv(c, thr, 3, 0);
  fail_unless(__arc_getenv(c, thr, 0, 0) == INT2FIX(1));
  fail_unless(__arc_getenv(c, thr, 0, 1) == INT2FIX(2));
  fail_unless(__arc_getenv(c, thr, 0, 2) == INT2FIX(3));
  CPUSH(thr, INT2FIX(7));
  CPUSH(thr, INT2FIX(8));
  __arc_menv(c, thr, 2);
  __arc_mkenv(c, thr, 2, 0);
  fail_unless(__arc_getenv(c, thr, 0, 0) == INT2FIX(7));
  fail_unless(__arc_getenv(c, thr, 0, 1) == INT2FIX(8));

  /* New environment is larger than the old environment */
  thr = arc_mkthread(c);
  CPUSH(thr, INT2FIX(1));
  CPUSH(thr, INT2FIX(2));
  CPUSH(thr, INT2FIX(3));
  __arc_mkenv(c, thr, 3, 0);
  fail_unless(__arc_getenv(c, thr, 0, 0) == INT2FIX(1));
  fail_unless(__arc_getenv(c, thr, 0, 1) == INT2FIX(2));
  fail_unless(__arc_getenv(c, thr, 0, 2) == INT2FIX(3));
  CPUSH(thr, INT2FIX(9));
  CPUSH(thr, INT2FIX(10));
  CPUSH(thr, INT2FIX(11));
  CPUSH(thr, INT2FIX(12));
  __arc_menv(c, thr, 4);
  __arc_mkenv(c, thr, 4, 0);
  fail_unless(__arc_getenv(c, thr, 0, 0) == INT2FIX(9));
  fail_unless(__arc_getenv(c, thr, 0, 1) == INT2FIX(10));
  fail_unless(__arc_getenv(c, thr, 0, 2) == INT2FIX(11));
  fail_unless(__arc_getenv(c, thr, 0, 3) == INT2FIX(12));
}
END_TEST

START_TEST(test_heap_env)
{
  value thr;

  thr = arc_mkthread(c);
  CPUSH(thr, INT2FIX(1));
  CPUSH(thr, INT2FIX(2));
  CPUSH(thr, INT2FIX(3));
  __arc_mkenv(c, thr, 3, 0);

  CPUSH(thr, INT2FIX(4));
  CPUSH(thr, INT2FIX(5));
  CPUSH(thr, INT2FIX(6));
  __arc_mkenv(c, thr, 3, 0);

  CPUSH(thr, INT2FIX(7));
  CPUSH(thr, INT2FIX(8));
  CPUSH(thr, INT2FIX(9));
  __arc_mkenv(c, thr, 3, 0);

  TENVR(thr) = __arc_env2heap(c, thr, TENVR(thr));

  fail_unless(__arc_getenv(c, thr, 0, 0) == INT2FIX(7));
  fail_unless(__arc_getenv(c, thr, 0, 1) == INT2FIX(8));
  fail_unless(__arc_getenv(c, thr, 0, 2) == INT2FIX(9));

  fail_unless(__arc_getenv(c, thr, 1, 0) == INT2FIX(4));
  fail_unless(__arc_getenv(c, thr, 1, 1) == INT2FIX(5));
  fail_unless(__arc_getenv(c, thr, 1, 2) == INT2FIX(6));

  fail_unless(__arc_getenv(c, thr, 2, 0) == INT2FIX(1));
  fail_unless(__arc_getenv(c, thr, 2, 1) == INT2FIX(2));
  fail_unless(__arc_getenv(c, thr, 2, 2) == INT2FIX(3));

}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Environments");
  TCase *tc_env = tcase_create("Environments");
  SRunner *sr;

  c = &cc;
  arc_init(c);

  tcase_add_test(tc_env, test_env_simple);
  tcase_add_test(tc_env, test_menv);
  tcase_add_test(tc_env, test_heap_env);

  suite_add_tcase(s, tc_env);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);

}
