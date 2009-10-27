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
#include "../config.h"

carc c;

START_TEST(test_pp_atom)
{
  value ppval;
  int i, found;

  ppval = carc_prettyprint(&c, CNIL);
  fail_unless(TYPE(ppval) == T_STRING);
  fail_unless(carc_strlen(&c, ppval) == 3);
  fail_unless(carc_strindex(&c, ppval, 0) == 'n');
  fail_unless(carc_strindex(&c, ppval, 1) == 'i');
  fail_unless(carc_strindex(&c, ppval, 2) == 'l');

  ppval = carc_prettyprint(&c, CTRUE);
  fail_unless(TYPE(ppval) == T_STRING);
  fail_unless(carc_strlen(&c, ppval) == 1);
  fail_unless(carc_strindex(&c, ppval, 0) == 't');

  ppval = carc_prettyprint(&c, INT2FIX(1234));
  fail_unless(TYPE(ppval) == T_STRING);
  fail_unless(carc_strlen(&c, ppval) == 4);
  fail_unless(carc_strindex(&c, ppval, 0) == '1');
  fail_unless(carc_strindex(&c, ppval, 1) == '2');
  fail_unless(carc_strindex(&c, ppval, 2) == '3');
  fail_unless(carc_strindex(&c, ppval, 3) == '4');

  ppval = carc_prettyprint(&c, INT2FIX(-1234));
  fail_unless(TYPE(ppval) == T_STRING);
  fail_unless(carc_strlen(&c, ppval) == 5);
  fail_unless(carc_strindex(&c, ppval, 0) == '-');
  fail_unless(carc_strindex(&c, ppval, 1) == '1');
  fail_unless(carc_strindex(&c, ppval, 2) == '2');
  fail_unless(carc_strindex(&c, ppval, 3) == '3');
  fail_unless(carc_strindex(&c, ppval, 4) == '4');

  ppval = carc_prettyprint(&c, carc_mkflonum(&c, -1.234));
  fail_unless(TYPE(ppval) == T_STRING);
  fail_unless(carc_strindex(&c, ppval, 0) == '-');
  fail_unless(carc_strindex(&c, ppval, 1) == '1');
  fail_unless(carc_strindex(&c, ppval, 2) == '.');
  fail_unless(carc_strindex(&c, ppval, 3) == '2');
  fail_unless(carc_strindex(&c, ppval, 4) == '3');
  fail_unless(carc_strindex(&c, ppval, 5) == '4');

  ppval = carc_prettyprint(&c, carc_mkcomplex(&c, -1.234, 5.678));
  fail_unless(TYPE(ppval) == T_STRING);
  fail_unless(carc_strindex(&c, ppval, 0) == '-');
  fail_unless(carc_strindex(&c, ppval, 1) == '1');
  fail_unless(carc_strindex(&c, ppval, 2) == '.');
  fail_unless(carc_strindex(&c, ppval, 3) == '2');
  fail_unless(carc_strindex(&c, ppval, 4) == '3');
  fail_unless(carc_strindex(&c, ppval, 5) == '4');

  found = 0;
  for (i=0; i<carc_strlen(&c, ppval); i++) {
    if (carc_strindex(&c, ppval, i) == '+') {
      found = 1;
      break;
    }
  }
  fail_unless(found);
  fail_unless(carc_strindex(&c, ppval, i) == '+');
  fail_unless(carc_strindex(&c, ppval, i+1) == '5');
  fail_unless(carc_strindex(&c, ppval, i+2) == '.');
  fail_unless(carc_strindex(&c, ppval, i+3) == '6');
  fail_unless(carc_strindex(&c, ppval, i+4) == '7');
  fail_unless(carc_strindex(&c, ppval, i+5) == '8');

  fail_unless(carc_strindex(&c, ppval, carc_strlen(&c, ppval)-1) == 'i');

  ppval = carc_prettyprint(&c, carc_mkcomplex(&c, -1.234, -5.678));
  fail_unless(TYPE(ppval) == T_STRING);
  fail_unless(carc_strindex(&c, ppval, 0) == '-');
  fail_unless(carc_strindex(&c, ppval, 1) == '1');
  fail_unless(carc_strindex(&c, ppval, 2) == '.');
  fail_unless(carc_strindex(&c, ppval, 3) == '2');
  fail_unless(carc_strindex(&c, ppval, 4) == '3');
  fail_unless(carc_strindex(&c, ppval, 5) == '4');

  found = 0;
  for (i=1; i<carc_strlen(&c, ppval); i++) {
    if (carc_strindex(&c, ppval, i) == '-') {
      found = 1;
      break;
    }
  }
  fail_unless(found);
  fail_unless(carc_strindex(&c, ppval, i) == '-');
  fail_unless(carc_strindex(&c, ppval, i+1) == '5');
  fail_unless(carc_strindex(&c, ppval, i+2) == '.');
  fail_unless(carc_strindex(&c, ppval, i+3) == '6');
  fail_unless(carc_strindex(&c, ppval, i+4) == '7');
  fail_unless(carc_strindex(&c, ppval, i+5) == '8');

  fail_unless(carc_strindex(&c, ppval, carc_strlen(&c, ppval)-1) == 'i');

#ifdef HAVE_GMP_H
  {
    value n;
    char digits[] = "93326215443944152681699238856266700490715968264381621468592963895217599993229915608941463976156518286253697920827223758251185210916864000000000000000000000000";
    char rat[] = "104348/33215";

    n = carc_mkbignuml(&c, 0);
    mpz_set_str(REP(n)._bignum, digits, 10);
    ppval = carc_prettyprint(&c, n);
    fail_unless(TYPE(ppval) == T_STRING);
    for (i=0; digits[i]; i++)
      fail_unless(carc_strindex(&c, ppval, i) == (Rune)digits[i]);

    n = carc_mkrationall(&c, 104348, 33215);
    ppval = carc_prettyprint(&c, n);
    fail_unless(TYPE(ppval) == T_STRING);
    for (i=0; rat[i]; i++)
      fail_unless(carc_strindex(&c, ppval, i) == (Rune)rat[i]);
  }
#endif
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Pretty Printer");
  TCase *tc_pp = tcase_create("Pretty Printer");
  SRunner *sr;

  carc_set_memmgr(&c);
  tcase_add_test(tc_pp, test_pp_atom);

  suite_add_tcase(s, tc_pp);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
