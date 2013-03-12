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

#define CPUSH_(val) CPUSH(thr, val)

#define XCALL(fname, ...) do {			\
    TVALR(thr) = arc_mkaff(c, fname, CNIL);	\
    TARGC(thr) = NARGS(__VA_ARGS__);		\
    FOR_EACH(CPUSH_, __VA_ARGS__);		\
    __arc_thr_trampoline(c, thr);		\
  } while (0)

START_TEST(test_read_symbol)
{
  value thr, sio;

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "foo"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  fail_unless(TYPE(TVALR(thr)) == T_SYMBOL);
  fail_unless(TVALR(thr) == arc_intern_cstr(c, "foo"));
}
END_TEST

START_TEST(test_read_list)
{
  value thr, sio;

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "(a b c)"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  fail_unless(TYPE(TVALR(thr)) == T_CONS);
  fail_unless(car(TVALR(thr)) == arc_intern_cstr(c, "a"));
  fail_unless(cadr(TVALR(thr)) == arc_intern_cstr(c, "b"));
  fail_unless(car(cddr(TVALR(thr))) == arc_intern_cstr(c, "c"));
}
END_TEST

START_TEST(test_read_imp_list)
{
  value thr, sio;

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "(a b . c)"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  fail_unless(TYPE(TVALR(thr)) == T_CONS);
  fail_unless(car(TVALR(thr)) == arc_intern_cstr(c, "a"));
  fail_unless(cadr(TVALR(thr)) == arc_intern_cstr(c, "b"));
  fail_unless(cddr(TVALR(thr)) == arc_intern_cstr(c, "c"));
}
END_TEST

START_TEST(test_read_bracket_fn)
{
  value thr, sio;

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "[cons _ nil]"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  fail_unless(TYPE(TVALR(thr)) == T_CONS);
  fail_unless(car(TVALR(thr)) == arc_intern_cstr(c, "fn"));
  fail_unless(TYPE(cdr(TVALR(thr))) == T_CONS);
  fail_unless(TYPE(cadr(TVALR(thr))) == T_CONS);
  fail_unless(car(cadr(TVALR(thr))) == arc_intern_cstr(c, "_"));
  fail_unless(NIL_P(cdr(cadr(TVALR(thr)))));
  fail_unless(TYPE(cddr(TVALR(thr))) == T_CONS);
  fail_unless(TYPE(car(cddr(TVALR(thr)))) == T_CONS);
  fail_unless(car(car(cddr(TVALR(thr)))) == arc_intern_cstr(c, "cons"));
  fail_unless(NIL_P(car(cddr(car(cddr(TVALR(thr)))))));
  fail_unless(NIL_P(cdr((cddr(TVALR(thr))))));
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
  tcase_add_test(tc_reader, test_read_list);
  tcase_add_test(tc_reader, test_read_imp_list);
  tcase_add_test(tc_reader, test_read_bracket_fn);

  suite_add_tcase(s, tc_reader);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

