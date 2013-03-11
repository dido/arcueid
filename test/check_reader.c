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

START_TEST(test_read_symbol)
{
  value thr, sio;

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "foo"), CNIL);
  TVALR(thr) = arc_mkaff(c, arc_sread, arc_mkstringc(c, "sread"));
  TARGC(thr) = 2;
  CPUSH(thr, sio);
  CPUSH(thr, CNIL);
  __arc_thr_trampoline(c, thr);
  fail_unless(TYPE(TVALR(thr)) == T_SYMBOL);
  fail_unless(TVALR(thr) == arc_intern_cstr(c, "foo"));
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Arcueid's Reader");
  TCase *tc_reader = tcase_create("Arcueid's Reader");
  SRunner *sr;

  c = &cc;
  arc_init(c);

  tcase_add_test(tc_reader, test_read_symbol);

  suite_add_tcase(s, tc_reader);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

