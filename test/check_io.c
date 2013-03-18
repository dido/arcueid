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

START_TEST(test_sio_readb)
{
  value thr, sio;

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "abc"), CNIL);
  TVALR(thr) = arc_mkaff(c, arc_readb, CNIL);
  TARGC(thr) = 1;
  CPUSH(thr, sio);
  __arc_thr_trampoline(c, thr, TR_FNAPP);
  fail_unless(TVALR(thr) == INT2FIX(97));

  TVALR(thr) = arc_mkaff(c, arc_readb, CNIL);
  TARGC(thr) = 1;
  CPUSH(thr, sio);
  __arc_thr_trampoline(c, thr, TR_FNAPP);
  fail_unless(TVALR(thr) == INT2FIX(98));

  TVALR(thr) = arc_mkaff(c, arc_readb, CNIL);
  TARGC(thr) = 1;
  CPUSH(thr, sio);
  __arc_thr_trampoline(c, thr, TR_FNAPP);
  fail_unless(TVALR(thr) == INT2FIX(99));

  TVALR(thr) = arc_mkaff(c, arc_readb, CNIL);
  TARGC(thr) = 1;
  CPUSH(thr, sio);
  __arc_thr_trampoline(c, thr, TR_FNAPP);
  fail_unless(NIL_P(TVALR(thr)));
}
END_TEST

START_TEST(test_sio_readc)
{
  value thr, sio;

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "以呂波"), CNIL);
  TVALR(thr) = arc_mkaff(c, arc_readc, CNIL);
  TARGC(thr) = 1;
  CPUSH(thr, sio);
  __arc_thr_trampoline(c, thr, TR_FNAPP);
  fail_unless(TYPE(TVALR(thr)) == T_CHAR);
  fail_unless(arc_char2rune(c, TVALR(thr)) == 0x4ee5);

  TVALR(thr) = arc_mkaff(c, arc_readc, CNIL);
  TARGC(thr) = 1;
  CPUSH(thr, sio);
  __arc_thr_trampoline(c, thr, TR_FNAPP);
  fail_unless(TYPE(TVALR(thr)) == T_CHAR);
  fail_unless(arc_char2rune(c, TVALR(thr)) == 0x5442);

  TVALR(thr) = arc_mkaff(c, arc_readc, CNIL);
  TARGC(thr) = 1;
  CPUSH(thr, sio);
  __arc_thr_trampoline(c, thr, TR_FNAPP);
  fail_unless(TYPE(TVALR(thr)) == T_CHAR);
  fail_unless(arc_char2rune(c, TVALR(thr)) == 0x6ce2);
  TVALR(thr) = arc_mkaff(c, arc_readc, CNIL);
  TARGC(thr) = 1;
  CPUSH(thr, sio);
  __arc_thr_trampoline(c, thr, TR_FNAPP);
  fail_unless(NIL_P(TVALR(thr)));

}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Arcueid I/O");
  TCase *tc_io = tcase_create("Arcueid I/O");
  SRunner *sr;

  c = &cc;
  arc_init(c);

  tcase_add_test(tc_io, test_sio_readb);
  tcase_add_test(tc_io, test_sio_readc);

  suite_add_tcase(s, tc_io);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

