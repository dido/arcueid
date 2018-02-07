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
#include <check.h>
#include "../src/arcueid.h"
#include "../src/vmengine.h"

AFFDEF(simple_aff)
{
  AFBEGIN;
  ARETURN(INT2FIX(31337));
  AFEND;
}
AFFEND

AFFDEF(subtractor)
{
  AARG(a, b);
  AFBEGIN;
  ARETURN(INT2FIX(FIX2INT(AV(a)) - FIX2INT(AV(b))));
  AFEND;
}
AFFEND

AFFDEF(doubler)
{
  AARG(a, b);
  AVAR(subf);
  AFBEGIN;

  WV(subf, arc_aff_new(c, subtractor));
  AFCALL(AV(subf), AV(a), AV(b));
  ARETURN(INT2FIX(2 * FIX2INT(AFCRV)));
  AFEND;
}
AFFEND

START_TEST(test_aff_simple)
{
  arc cc;
  arc *c = &cc;
  value thr;

  arc_init(c);
  thr = __arc_thread_new(c, 1);
  arc_thr_setacc(c, thr, arc_aff_new(c, simple_aff));
  __arc_thr_trampoline(c, thr, TR_FNAPP);
  ck_assert_int_eq(FIX2INT(arc_thr_acc(c, thr)), 31337);
}
END_TEST

START_TEST(test_subtractor)
{
  arc cc;
  arc *c = &cc;
  value thr;
  arc_thread *t;

  arc_init(c);
  thr = __arc_thread_new(c, 1);
  t = (arc_thread *)thr;
  arc_thr_setacc(c, thr, arc_aff_new(c, subtractor));
  CPUSH(thr, INT2FIX(3));
  CPUSH(thr, INT2FIX(2));
  t->argc = 2;
  __arc_thr_trampoline(c, thr, TR_FNAPP);
  ck_assert_int_eq(FIX2INT(arc_thr_acc(c, thr)), 1);
}
END_TEST

START_TEST(test_doubler)
{
  arc cc;
  arc *c = &cc;
  value thr;
  arc_thread *t;

  arc_init(c);
  thr = __arc_thread_new(c, 1);
  t = (arc_thread *)thr;
  arc_thr_setacc(c, thr, arc_aff_new(c, doubler));
  CPUSH(thr, INT2FIX(5));
  CPUSH(thr, INT2FIX(2));
  t->argc = 2;
  __arc_thr_trampoline(c, thr, TR_FNAPP);
  ck_assert_int_eq(FIX2INT(arc_thr_acc(c, thr)), 6);
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Arcueid Foreign Functions");
  TCase *tc_aff = tcase_create("Arcueid Foreign Functions");
  SRunner *sr;

  tcase_add_test(tc_aff, test_aff_simple);
  tcase_add_test(tc_aff, test_subtractor);
  tcase_add_test(tc_aff, test_doubler);

  suite_add_tcase(s, tc_aff);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
