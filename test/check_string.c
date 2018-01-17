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
#include "../config.h"

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
#ifndef alloca
# define alloca __builtin_alloca
#endif
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# include <stddef.h>
void *alloca (size_t);
#endif

START_TEST(test_make_strings)
{
  arc cc;
  arc *c = &cc;
  value newstring;
  int i;
  Rune runes[] = { 0x9060, 0x91ce, 0x5fd7, 0x8cb4 };
  char *str;

  arc_init(c);

  newstring = arc_string_new_cstr(c, "遠野志貴");
  ck_assert_int_eq(arc_strlen(c, newstring), 4);

  arc_strsetindex(c, newstring, 2, 0x79cb);
  arc_strsetindex(c, newstring, 3, 0x8449);
  str = alloca(arc_strutflen(c, newstring));
  arc_str2cstr(c, newstring, str);
  ck_assert(strcmp(str, "遠野秋葉") == 0);

  newstring = arc_string_new(c, 3, 0x16a0);
  ck_assert_int_eq(arc_strlen(c, newstring), 3);
  ck_assert_int_eq(arc_strindex(newstring, 0), 0x16a0);
  ck_assert_int_eq(arc_strindex(newstring, 1), 0x16a0);
  ck_assert_int_eq(arc_strindex(newstring, 2), 0x16a0);

  newstring = arc_string_new_str(c, 4, Runes);
  ck_assert_int_eq(arc_strlen(c, newstring), 3);
  arc_str2cstr(c, newstring, str);
  ck_assert(strcmp(str, "遠野志貴") == 0);
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Strings");
  TCase *tc_string = tcase_create("Strings");
  SRunner *sr;

  tcase_add_test(tc_string, test_make_strings);

  suite_add_tcase(s, tc_hash);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_VERBOSE);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
