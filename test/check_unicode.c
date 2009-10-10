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

START_TEST(test_utf8decode)
{
  Rune r;

  fail_unless(chartorune(&r, "\x24") == 1);
  fail_unless(r == 0x0024);

  fail_unless(chartorune(&r, "\xc2\xa2") == 2);
  fail_unless(r == 0x00a2);

  fail_unless(chartorune(&r, "\xd7\x90") == 2);
  fail_unless(r == 0x05d0);

  fail_unless(chartorune(&r, "\xe2\x82\xac") == 3);
  fail_unless(r == 0x20ac);

  fail_unless(chartorune(&r, "\xf0\x92\x80\xb1") == 4);
  fail_unless(r == 0x12031);

  /* Boundary condition test cases */
  /* First possible sequence of a certain length */
  fail_unless(chartorune(&r, "\x00") == 1); /* one byte */
  fail_unless(r == 0x00000000);

  fail_unless(chartorune(&r, "\xc2\x80") == 2); /* two bytes */
  fail_unless(r == 0x00000080);

  fail_unless(chartorune(&r, "\xe0\xa0\x80") == 3); /* three bytes */
  fail_unless(r == 0x00000800);

  fail_unless(chartorune(&r, "\xf0\x90\x80\x80") == 4); /* four bytes */
  fail_unless(r == 0x00010000);

  fail_unless(chartorune(&r, "\xf8\x88\x80\x80\x80") == 5); /* five bytes */
  fail_unless(r == 0x00200000);

  fail_unless(chartorune(&r, "\xfc\x84\x80\x80\x80\x80") == 6); /* six bytes */
  fail_unless(r == 0x04000000);

  /* Last possible sequence of a certain length */
  fail_unless(chartorune(&r, "\x7f") == 1); /* one byte */
  fail_unless(r == 0x0000007f);

  fail_unless(chartorune(&r, "\xdf\xbf") == 2); /* two bytes */
  fail_unless(r == 0x000007ff);

  fail_unless(chartorune(&r, "\xef\xbf\xbf") == 3); /* three bytes */
  fail_unless(r == 0x0000ffff);

  fail_unless(chartorune(&r, "\xf7\xbf\xbf\xbf") == 4); /* four bytes */
  fail_unless(r == 0x001fffff);

  fail_unless(chartorune(&r, "\xfb\xbf\xbf\xbf\xbf") == 5); /* five bytes */
  fail_unless(r == 0x03ffffff);

  fail_unless(chartorune(&r, "\xfd\xbf\xbf\xbf\xbf\xbf") == 6); /* six bytes */
  fail_unless(r == 0x7fffffff);

  /* Other boundary conditions */
  fail_unless(chartorune(&r, "\xed\x9f\xbf") == 3);
  fail_unless(r == 0x0000d7ff);

  fail_unless(chartorune(&r, "\xee\x80\x80") == 3);
  fail_unless(r == 0x0000e000);

  fail_unless(chartorune(&r, "\xef\xbf\xbd") == 3);
  fail_unless(r == 0x0000fffd);

  fail_unless(chartorune(&r, "\xf4\x8f\xbf\xbf") == 4);
  fail_unless(r == 0x0010ffff);

  fail_unless(chartorune(&r, "\xf4\x90\x80\x80") == 4);
  fail_unless(r == 0x00110000);

  /* Malformed sequences */

  /* Impossible bytes */
  fail_unless(chartorune(&r, "\xfe") == 1);
  fail_unless(r == 0x00000080);

  fail_unless(chartorune(&r, "\xff") == 1);
  fail_unless(r == 0x00000080);

  /* overlong UTF-8 sequences (invalid) */
  fail_unless(chartorune(&r, "\xc0\xaf") == 1);
  fail_unless(r == 0x80);

  fail_unless(chartorune(&r, "\xe0\x80\xaf") == 1);
  fail_unless(r == 0x80);

  fail_unless(chartorune(&r, "\xf0\x80\x80\xaf") == 1);
  fail_unless(r == 0x80);

  fail_unless(chartorune(&r, "\xf8\x80\x80\x80\xaf") == 1);
  fail_unless(r == 0x80);

  fail_unless(chartorune(&r, "\xfc\x80\x80\x80\x80\xaf") == 1);
  fail_unless(r == 0x80);

  /* maximum overlong sequences */
  fail_unless(chartorune(&r, "\xc1\xbf") == 1);
  fail_unless(r == 0x80);

  fail_unless(chartorune(&r, "\xe0\x9f\xbf") == 1);
  fail_unless(r == 0x80);

  fail_unless(chartorune(&r, "\xf0\x8f\xbf\xbf") == 1);
  fail_unless(r == 0x80);

  fail_unless(chartorune(&r, "\xf8\x87\xbf\xbf\xbf") == 1);
  fail_unless(r == 0x80);

  fail_unless(chartorune(&r, "\xfc\x83\xbf\xbf\xbf\xbf") == 1);
  fail_unless(r == 0x80);

  /* Overlong representations of NUL */
  fail_unless(chartorune(&r, "\xc0\x80") == 1);
  fail_unless(r == 0x80);

  fail_unless(chartorune(&r, "\xe0\x80\x80") == 1);
  fail_unless(r == 0x80);

  fail_unless(chartorune(&r, "\xf0\x80\x80\x80") == 1);
  fail_unless(r == 0x80);

  fail_unless(chartorune(&r, "\xf8\x80\x80\x80\x80") == 1);
  fail_unless(r == 0x80);

  fail_unless(chartorune(&r, "\xfc\x80\x80\x80\x80\x80") == 1);
  fail_unless(r == 0x80);

  fail_unless(chartorune(&r, "\xed\xa0\x80") == 1);
  fail_unless(r == 0x80);

  fail_unless(chartorune(&r, "\xed\xad\xbf") == 1);
  fail_unless(r == 0x80);

  fail_unless(chartorune(&r, "\xed\xae\x80") == 1);
  fail_unless(r == 0x80);

  fail_unless(chartorune(&r, "\xed\xaf\xbf") == 1);
  fail_unless(r == 0x80);

  fail_unless(chartorune(&r, "\xed\xb0\x80") == 1);
  fail_unless(r == 0x80);

  fail_unless(chartorune(&r, "\xed\xbe\x80") == 1);
  fail_unless(r == 0x80);

  fail_unless(chartorune(&r, "\xed\xbf\xbf") == 1);
  fail_unless(r == 0x80);

}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Unicode");
  TCase *tc_utf8 = tcase_create("UTF-8 conversion");
  TCase *tc_uctype = tcase_create("Unicode character classification");
  SRunner *sr;

  tcase_add_test(tc_utf8, test_utf8decode);
  tcase_add_test(tc_uctype, test_ucisspace);

  suite_add_tcase(s, tc_utf8);
  suite_add_tcase(s, tc_uctype);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
