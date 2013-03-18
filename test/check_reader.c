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
#include "../src/arith.h"
#include "../config.h"

arc cc;
arc *c;

#define CPUSH_(val) CPUSH(thr, val)

#define XCALL(fname, ...) do {			\
    TVALR(thr) = arc_mkaff(c, fname, CNIL);	\
    TARGC(thr) = NARGS(__VA_ARGS__);		\
    FOR_EACH(CPUSH_, __VA_ARGS__);		\
    __arc_thr_trampoline(c, thr, TR_FNAPP);	\
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

START_TEST(test_read_comment)
{
  value thr, sio, sexpr;

  /* inline comments */
  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "(foo ; Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.\nbar)"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  sexpr = TVALR(thr);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(car(sexpr) == arc_intern_cstr(c, "foo"));
  fail_unless(cadr(sexpr) == arc_intern_cstr(c, "bar"));

  /* inline comments in an bracketed function */
  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "[cons ; Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.\n\t  _ nil]"), CNIL);
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

  /* long comments */
  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "; Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed\n; do eiusmod tempor incididunt ut labore et dolore magna aliqua.\n; Ut enim ad minim veniam, quis nostrud exercitation ullamco\n; laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure\n; dolor in reprehenderit in voluptate velit esse cillum dolore eu\n; fugiat nulla pariatur. Excepteur sint occaecat cupidatat non\n; proident, sunt in culpa qui officia deserunt mollit anim id\n; est laborum.\n(baz buzz))"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  sexpr = TVALR(thr);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(car(sexpr) == arc_intern_cstr(c, "baz"));
  fail_unless(cadr(sexpr) == arc_intern_cstr(c, "buzz"));

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

START_TEST(test_read_number)
{
  value thr, sio;
#ifdef HAVE_GMP_H
  value expected;
#endif

  /* Hmm... reference Arc doesn't support the 0xdeadbeef form for hexadecimal
     constants? The Limbo-style radix selector used here is an Arcueid
     extension. */
  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "16rdeadbeef"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  fail_unless(TYPE(TVALR(thr)) == T_FIXNUM);
  fail_unless(TVALR(thr) == INT2FIX(0xdeadbeef));

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "31337"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  fail_unless(TYPE(TVALR(thr)) == T_FIXNUM);
  fail_unless(TVALR(thr) == INT2FIX(31337));

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "8r1234"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  fail_unless(TYPE(TVALR(thr)) == T_FIXNUM);
  fail_unless(TVALR(thr) == INT2FIX(01234));

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "01234"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  fail_unless(TYPE(TVALR(thr)) == T_FIXNUM);
  fail_unless(TVALR(thr) == INT2FIX(1234));

#ifdef HAVE_GMP_H
  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  fail_unless(TYPE(TVALR(thr)) == T_BIGNUM);
  expected = arc_mkbignuml(c, 0);
  mpz_set_str(REPBNUM(expected), "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", 10);
  fail_unless(arc_is2(c, TVALR(thr), expected) == CTRUE);

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "1/2"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  fail_unless(TYPE(TVALR(thr)) == T_RATIONAL);
  expected = arc_mkrationall(c, 1, 2);
  fail_unless(arc_is2(c, TVALR(thr), expected) == CTRUE);
#endif

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "3.1415926"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  fail_unless(TYPE(TVALR(thr)) == T_FLONUM);
  fail_unless(fabs(REPFLO(TVALR(thr)) - 3.1415926) < 1e-6);

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "3.1415926+2.7182828i"), CNIL);
  XCALL(arc_sread, sio, CNIL);
  fail_unless(TYPE(TVALR(thr)) == T_COMPLEX);
  fail_unless(fabs(creal(REPCPX(TVALR(thr))) - 3.1415926) < 1e-6);
  fail_unless(fabs(cimag(REPCPX(TVALR(thr))) - 2.7182828) < 1e-6);

}
END_TEST

START_TEST(test_read_atstring)
{
  value str, sexpr, fp, thr;

  c->atstrings = 1;
  thr = arc_mkthread(c);
  str = arc_mkstringc(c, "\"now @@ escaped at-signs\"");
  fp = arc_instring(c, str, CNIL);
  XCALL(arc_sread, fp, CNIL);
  sexpr = TVALR(thr);
  fail_unless(TYPE(sexpr) == T_STRING);
  fail_unless(arc_is2(c, arc_mkstringc(c, "now @ escaped at-signs"), sexpr) == CTRUE);

  thr = arc_mkthread(c);
  str = arc_mkstringc(c, "\"and @(+ 1 1) from (+ 1 1)\"");
  fp = arc_instring(c, str, CNIL);
  XCALL(arc_sread, fp, CNIL);
  sexpr = TVALR(thr);
  fail_unless(TYPE(sexpr) == T_CONS);
  /* XXX study what the cons cell this created actually contains */

  c->atstrings = 0;
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

  tcase_add_test(tc_reader, test_read_list);
  tcase_add_test(tc_reader, test_read_imp_list);
  tcase_add_test(tc_reader, test_read_bracket_fn);
  tcase_add_test(tc_reader, test_read_quote);
  tcase_add_test(tc_reader, test_read_qquote);
  tcase_add_test(tc_reader, test_read_comma);
  tcase_add_test(tc_reader, test_read_string);
  tcase_add_test(tc_reader, test_read_char);
  tcase_add_test(tc_reader, test_read_comment);
  tcase_add_test(tc_reader, test_read_symbol);
  tcase_add_test(tc_reader, test_read_number);
  tcase_add_test(tc_reader, test_read_atstring);

  suite_add_tcase(s, tc_reader);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

