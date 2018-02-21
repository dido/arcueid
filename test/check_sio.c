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

#include <stdlib.h>
#include <string.h>
#include <check.h>
#include <stdio.h>
#include "../src/arcueid.h"
#include "../src/vmengine.h"

arc cc;
arc *c;

START_TEST(test_sio_readb)
{
  value thr, sio;
  arc_thread *t;

  thr = __arc_thread_new(c, 1);
  t = (arc_thread *)thr;

  sio = arc_instring(c, arc_string_new_cstr(c, "abc"));

  CPUSH(thr, sio);
  t->argc = 1;
  arc_thr_setacc(c, thr, arc_aff_new(c, arc_readb));
  __arc_thr_trampoline(c, thr, TR_FNAPP);
  ck_assert_int_eq(FIX2INT(arc_thr_acc(c, thr)), 0x61);

  CPUSH(thr, sio);
  t->argc = 1;
  arc_thr_setacc(c, thr, arc_aff_new(c, arc_readb));
  __arc_thr_trampoline(c, thr, TR_FNAPP);
  ck_assert_int_eq(FIX2INT(arc_thr_acc(c, thr)), 0x62);

  CPUSH(thr, sio);
  t->argc = 1;
  arc_thr_setacc(c, thr, arc_aff_new(c, arc_readb));
  __arc_thr_trampoline(c, thr, TR_FNAPP);
  ck_assert_int_eq(FIX2INT(arc_thr_acc(c, thr)), 0x63);

  CPUSH(thr, sio);
  t->argc = 1;
  arc_thr_setacc(c, thr, arc_aff_new(c, arc_readb));
  __arc_thr_trampoline(c, thr, TR_FNAPP);
  ck_assert(NILP(arc_thr_acc(c, thr)));

  CPUSH(thr, sio);
  t->argc = 1;
  arc_thr_setacc(c, thr, arc_aff_new(c, arc_readb));
  __arc_thr_trampoline(c, thr, TR_FNAPP);
  ck_assert(NILP(arc_thr_acc(c, thr)));
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Arcueid String I/O");
  TCase *tc_sio = tcase_create("Arcueid String I/O");
  SRunner *sr;

  c = &cc;
  arc_init(c);
  arc_types_init(c);

  tcase_add_test(tc_sio, test_sio_readb);

  suite_add_tcase(s, tc_sio);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}


