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
#include "../src/utf.h"

START_TEST(test_ucisspace)
{
  fail_unless(ucisspace(0x09));
  fail_unless(ucisspace(0x0a));
  fail_unless(ucisspace(0x0b));
  fail_unless(ucisspace(0x0c));
  fail_unless(ucisspace(0x0d));
  fail_unless(ucisspace(0x20));
  fail_unless(ucisspace(0xa0));
  fail_unless(ucisspace(0x1680));
  fail_unless(ucisspace(0x180e));
  fail_unless(ucisspace(0x2000));
  fail_unless(ucisspace(0x2001));
  fail_unless(ucisspace(0x2002));
  fail_unless(ucisspace(0x2003));
  fail_unless(ucisspace(0x2004));
  fail_unless(ucisspace(0x2005));
  fail_unless(ucisspace(0x2006));
  fail_unless(ucisspace(0x2007));
  fail_unless(ucisspace(0x2008));
  fail_unless(ucisspace(0x2009));
  fail_unless(ucisspace(0x200a));
  fail_unless(ucisspace(0x2028));
  fail_unless(ucisspace(0x2029));
  fail_unless(ucisspace(0x202f));
  fail_unless(ucisspace(0x205f));
  fail_unless(ucisspace(0x3000));
  fail_if(ucisspace(0x30));
  fail_if(ucisspace(0x07));
  fail_if(ucisspace(0x65));
  fail_if(ucisspace(0x9f8d));
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Unicode");
  TCase *tc_utf8 = tcase_create("UTF-8 conversion");
  TCase *tc_uctype = tcase_create("Unicode character classification");
  SRunner *sr;

  tcase_add_test(tc_uctype, test_ucisspace);

  suite_add_tcase(s, tc_utf8);
  suite_add_tcase(s, tc_uctype);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
