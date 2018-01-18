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

START_TEST(test_sym)
{
  arc cc;
  arc *c = &cc;
  value s1, s2;
  value str1, str2;

  arc_init(c);
  __arc_sym_t.init(c);

  s1 = arc_intern_cstr(c, "foo");
  s2 = arc_intern_cstr(c, "bar");
  ck_assert(s1 != s2);
  ck_assert(s1 == arc_intern_cstr(c, "foo"));
  ck_assert(s2 == arc_intern_cstr(c, "bar"));
  str1 = arc_string_new_cstr(c, "baz");
  str2 = arc_string_new_cstr(c, "baz");
  ck_assert(str1 != str2);
  ck_assert(arc_intern(c, str1) == arc_intern(c, str2));
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Symbols");
  TCase *tc_sym = tcase_create("Symbols");
  SRunner *sr;

  tcase_add_test(tc_sym, test_sym);

  suite_add_tcase(s, tc_sym);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_VERBOSE);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
