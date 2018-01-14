/* Copyright (C) 2018 Rafael R. Sevilla

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

START_TEST(test_rune)
{
  arc cc;
  arc *c = &cc;
  value r, r2;

  arc_init(c);
  __arc_rune_t.init(c);

  r = arc_rune_new(c, 0x16a0);

  ck_assert_int_eq(arc_rune(r), 0x16a0);
  r2 = arc_rune_new(c, 0x16a0);

  ck_assert(r == r2);
  ck_assert_int_eq(arc_rune(r2), 0x16a0);

  r2 = arc_rune_new(c, 0x16a6);
  ck_assert_int_eq(arc_rune(r2), 0x16a6);
  ck_assert(r != r2);
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Runes");
  TCase *tc_rune = tcase_create("Runes");
  SRunner *sr;

  tcase_add_test(tc_rune, test_rune);
  suite_add_tcase(s, tc_rune);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

