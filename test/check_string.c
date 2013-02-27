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
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include <math.h>
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

arc cc;
arc *c;

/* Do nothing: root marking is done by individual tests */
static void markroots(arc *c)
{
}

START_TEST(test_make_strings)
{
  value newstring;
  int i;
  Rune runes[] = { 0x9060, 0x91ce, 0x5fd7, 0x8cb4 };
  char *str;

  newstring = arc_mkstringc(c, "遠野志貴");
  fail_unless(arc_strlen(c, newstring) == 4);
  for (i=0; i<4; i++)
    fail_unless(arc_strindex(c, newstring, i) == runes[i]);

  arc_strsetindex(c, newstring, 2, 0x79cb);
  arc_strsetindex(c, newstring, 3, 0x8449);
  str = alloca(FIX2INT(arc_strutflen(c, newstring)));
  arc_str2cstr(c, newstring, str);
  fail_unless(strcmp(str, "遠野秋葉") == 0);
}
END_TEST

START_TEST(test_compare_strings)
{
  value str1, str2, str3;

  str1 = arc_mkstringc(c, "abc");
  str2 = arc_mkstringc(c, "bcd");
  str3 = arc_mkstringc(c, "abc");
  fail_unless(arc_strcmp(c, str1, str1) == 0);
  fail_unless(arc_strcmp(c, str2, str2) == 0);
  fail_unless(arc_strcmp(c, str1, str2) < 0);
  fail_unless(arc_strcmp(c, str2, str1) > 0);
  fail_unless(arc_strcmp(c, str1, str3) == 0);
  fail_unless(arc_is(c, str1, str1) == CTRUE);
  fail_unless(arc_is(c, str1, str3) == CTRUE);
  fail_unless(arc_iso(c, str1, str1, CNIL, CNIL) == CTRUE);
  fail_unless(arc_iso(c, str1, str3, CNIL, CNIL) == CTRUE);
  fail_unless(arc_is(c, str1, str2) == CNIL);
  fail_unless(arc_iso(c, str1, str2, CNIL, CNIL) == CNIL);
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Strings");
  TCase *tc_str = tcase_create("Strings");
  SRunner *sr;

  c = &cc;
  arc_init_memmgr(c);
  arc_init_datatypes(c);
  c->markroots = markroots;

  tcase_add_test(tc_str, test_make_strings);
  tcase_add_test(tc_str, test_compare_strings);

  suite_add_tcase(s, tc_str);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
