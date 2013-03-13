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
#include "../src/builtins.h"

arc cc;
arc *c;

#define CPUSH_(val) CPUSH(thr, val)

#define XCALL(fname, ...) do {			\
    TVALR(thr) = arc_mkaff(c, fname, CNIL);	\
    TARGC(thr) = NARGS(__VA_ARGS__);		\
    FOR_EACH(CPUSH_, __VA_ARGS__);		\
    __arc_thr_trampoline(c, thr);		\
  } while (0)

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

START_TEST(test_read_quote)
{
  value thr, sio, sexpr, qexpr;

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "'(+ a b)"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  sexpr = TVALR(thr);
  fail_unless(TYPE(car(sexpr)) == T_SYMBOL);
  fail_unless(car(sexpr) == ARC_BUILTIN(c, S_QUOTE));
  qexpr = cadr(sexpr);
  fail_unless(TYPE(qexpr) == T_CONS);
  fail_unless(TYPE(car(qexpr)) == T_SYMBOL);
  fail_unless(car(qexpr) == arc_intern_cstr(c, "+"));
  fail_unless(TYPE(car(cdr(qexpr))) == T_SYMBOL);
  fail_unless(car(cdr(qexpr)) == arc_intern_cstr(c, "a"));
  fail_unless(TYPE(car(cdr(cdr(qexpr)))) == T_SYMBOL);
  fail_unless(car(cdr(cdr(qexpr))) == arc_intern_cstr(c, "b"));
}
END_TEST

START_TEST(test_read_qquote)
{
  value thr, sio, sexpr, qexpr;

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "`(* c d)"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  sexpr = TVALR(thr);
  fail_unless(TYPE(car(sexpr)) == T_SYMBOL);
  fail_unless(car(sexpr) == ARC_BUILTIN(c, S_QQUOTE));
  qexpr = cadr(sexpr);
  fail_unless(TYPE(qexpr) == T_CONS);
  fail_unless(TYPE(car(qexpr)) == T_SYMBOL);
  fail_unless(car(qexpr) == arc_intern_cstr(c, "*"));
  fail_unless(TYPE(car(cdr(qexpr))) == T_SYMBOL);
  fail_unless(car(cdr(qexpr)) == arc_intern_cstr(c, "c"));
  fail_unless(TYPE(car(cdr(cdr(qexpr)))) == T_SYMBOL);
  fail_unless(car(cdr(cdr(qexpr))) == arc_intern_cstr(c, "d"));
}
END_TEST

START_TEST(test_read_comma)
{
  value thr, sio, sexpr;

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, ",a"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  sexpr = TVALR(thr);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(TYPE(car(sexpr)) == T_SYMBOL);
  fail_unless(car(sexpr) == ARC_BUILTIN(c, S_UNQUOTE));
  fail_unless(car(cdr(sexpr)) == arc_intern_cstr(c, "a"));

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, ",@b"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  sexpr = TVALR(thr);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(TYPE(car(sexpr)) == T_SYMBOL);
  fail_unless(car(sexpr) == ARC_BUILTIN(c, S_UNQUOTESP));
  fail_unless(car(cdr(sexpr)) == arc_intern_cstr(c, "b"));
}
END_TEST

START_TEST(test_read_string)
{
  value thr, sio, sexpr;
  char *teststr;

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "\"foo\""), CNIL);
  XCALL(arc_sread, sio, CNIL);
  sexpr = TVALR(thr);
  fail_unless(TYPE(sexpr) == T_STRING);
  fail_unless(arc_is2(c, arc_mkstringc(c, "foo"), sexpr) == CTRUE);

  /* escapes */
  teststr = "\b\t\n\v\f\r\\\'\"\a";
  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "\"\\b\\t\\n\\v\\f\\r\\\\\'\\\"\\a\""), CNIL);
  XCALL(arc_sread, sio, CNIL);
  sexpr = TVALR(thr);
  fail_unless(TYPE(sexpr) == T_STRING);
  fail_unless(arc_is2(c, arc_mkstringc(c, teststr), sexpr) == CTRUE);

  /* Unicode */
  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "\"\\U086dF\351\276\215\""), CNIL);
  XCALL(arc_sread, sio, CNIL);
  sexpr = TVALR(thr);
  fail_unless(TYPE(sexpr) == T_STRING);
  fail_unless(arc_strindex(c, sexpr, 0) == 0x86df);
  fail_unless(arc_strindex(c, sexpr, 1) == 0x9f8d);
}
END_TEST

START_TEST(test_read_char)
{
  value thr, sio, sexpr;

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "#\\a"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  sexpr = TVALR(thr);
  fail_unless(TYPE(sexpr) == T_CHAR);
  fail_unless(arc_char2rune(c, sexpr) == 'a');

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "#\\;"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  sexpr = TVALR(thr);
  fail_unless(TYPE(sexpr) == T_CHAR);
  fail_unless(arc_char2rune(c, sexpr) == ';');

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "#\\\351\276\215"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  sexpr = TVALR(thr);
  fail_unless(TYPE(sexpr) == T_CHAR);
  fail_unless(arc_char2rune(c, sexpr) == 0x9f8d);

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "#\\102"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  sexpr = TVALR(thr);
  fail_unless(TYPE(sexpr) == T_CHAR);
  fail_unless(arc_char2rune(c, sexpr) == 'B');

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "#\\newline"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  sexpr = TVALR(thr);
  fail_unless(TYPE(sexpr) == T_CHAR);
  fail_unless(arc_char2rune(c, sexpr) == '\n');

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "#\\null"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  sexpr = TVALR(thr);
  fail_unless(TYPE(sexpr) == T_CHAR);
  fail_unless(arc_char2rune(c, sexpr) == 0);

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "#\\u5a"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  sexpr = TVALR(thr);
  fail_unless(TYPE(sexpr) == T_CHAR);
  fail_unless(arc_char2rune(c, sexpr) == 0x5a);

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "#\\x5a"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  sexpr = TVALR(thr);
  fail_unless(TYPE(sexpr) == T_CHAR);
  fail_unless(arc_char2rune(c, sexpr) == 0x5a);

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "#\\u4e9c"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  sexpr = TVALR(thr);
  fail_unless(TYPE(sexpr) == T_CHAR);
  fail_unless(arc_char2rune(c, sexpr) == 0x4e9c);

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "#\\U12031"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  sexpr = TVALR(thr);
  fail_unless(TYPE(sexpr) == T_CHAR);
  fail_unless(arc_char2rune(c, sexpr) == 0x12031);

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "#\\\346\227\245"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  sexpr = TVALR(thr);
  fail_unless(TYPE(sexpr) == T_CHAR);
  fail_unless(arc_char2rune(c, sexpr) == 0x65e5);
}
END_TEST

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
  tcase_add_test(tc_reader, test_read_quote);
  tcase_add_test(tc_reader, test_read_qquote);
  tcase_add_test(tc_reader, test_read_comma);
  tcase_add_test(tc_reader, test_read_string);
  tcase_add_test(tc_reader, test_read_char);

  suite_add_tcase(s, tc_reader);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

