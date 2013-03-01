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

AFFDEF0(simple_aff)
{
  AFBEGIN;
  ARETURN(INT2FIX(31337));
  AFEND;
}
AFFEND

AFFDEF(subtractor, a, b)
{
  AFBEGIN;
  ARETURN(INT2FIX(FIX2INT(AV(a)) - FIX2INT(AV(b))));
  AFEND;
}
AFFEND

AFFDEF(doubler, a, b)
{
  AVAR(subf);
  AFBEGIN;

  AV(subf) = arc_mkaff(c, subtractor, arc_mkstringc(c, "subtractor"));
  AFCALL(AV(subf), AV(a), AV(b));
  ARETURN(INT2FIX(2 * FIX2INT(AFCRV)));
  AFEND;
}
AFFEND

START_TEST(test_aff_simple)
{
  value thr;

  thr = arc_mkthread(c);
  TVALR(thr) = arc_mkaff(c, simple_aff, arc_mkstringc(c, "simple_aff"));
  __arc_thr_trampoline(c, thr);
  fail_unless(TVALR(thr) == INT2FIX(31337));
}
END_TEST

START_TEST(test_aff_subtractor)
{
  value thr;

  thr = arc_mkthread(c);
  TVALR(thr) = arc_mkaff(c, subtractor, arc_mkstringc(c, "subtractor"));
  CPUSH(thr, INT2FIX(3));
  CPUSH(thr, INT2FIX(2));
  __arc_thr_trampoline(c, thr);
  fail_unless(TVALR(thr) == INT2FIX(1));
}
END_TEST

START_TEST(test_aff_doubler)
{
  value thr;

  thr = arc_mkthread(c);
  TVALR(thr) = arc_mkaff(c, doubler, arc_mkstringc(c, "doubler"));
  CPUSH(thr, INT2FIX(5));
  CPUSH(thr, INT2FIX(2));
  __arc_thr_trampoline(c, thr);
  fail_unless(TVALR(thr) == INT2FIX(6));
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Arcueid Foreign Functions");
  TCase *tc_aff = tcase_create("Arcueid Foreign Functions");
  SRunner *sr;

  c = &cc;
  arc_init(c);

  tcase_add_test(tc_aff, test_aff_simple);
  tcase_add_test(tc_aff, test_aff_subtractor);
  tcase_add_test(tc_aff, test_aff_doubler);

  suite_add_tcase(s, tc_aff);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

