/* 
  Copyright (C) 2012 Rafael R. Sevilla

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
#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include "../src/arcueid.h"
#include "../src/builtin.h"
#include "../src/symbols.h"

arc c, *cc;

START_TEST(test_atom)
{
  value str, fp, sexpr;

  str = arc_mkstringc(&c, "0");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_FIXNUM);
  fail_unless(FIX2INT(sexpr) == 0);

  str = arc_mkstringc(&c, "foo");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_SYMBOL);
  fail_unless(arc_is(&c, str, arc_sym2name(&c, sexpr)) == CTRUE);

  str = arc_mkstringc(&c, "0+1i");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_COMPLEX);
}
END_TEST

START_TEST(test_string)
{
  value str, sexpr, fp;
  int i;
  char *teststr;

  /* basic */
  str = arc_mkstringc(&c, "\"foo\"");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_STRING);
  fail_unless(arc_is(&c, arc_mkstringc(&c, "foo"), sexpr) == CTRUE);

  /* escapes */
  teststr = "\b\t\n\v\f\r\\\'\"\a";
  str = arc_mkstringc(&c, "\"\\b\\t\\n\\v\\f\\r\\\\\'\\\"\\a\"");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_STRING);
  for (i=0; i<arc_strlen(&c, sexpr); i++)
    fail_unless((Rune)teststr[i] == arc_strindex(&c, sexpr, i));

  /* Unicode */
  str = arc_mkstringc(&c, "\"\\U086dF\351\276\215\"");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_STRING);
  fail_unless(arc_strindex(&c, sexpr, 0) == 0x86df);
  fail_unless(arc_strindex(&c, sexpr, 1) == 0x9f8d);

  /* A long string */
  teststr = "\"Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.\"";
  str = arc_mkstringc(&c, teststr);
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_STRING);
  for (i=0; i<arc_strlen(&c, sexpr); i++)
    fail_unless((Rune)teststr[i+1] == arc_strindex(&c, sexpr, i));
}
END_TEST

START_TEST(test_list)
{
  value str, sexpr, fp;

  str = arc_mkstringc(&c, "()");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_NIL);
  fail_unless(sexpr == CNIL);

  str = arc_mkstringc(&c, "(foo 1 2 3)");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(TYPE(car(sexpr)) == T_SYMBOL);
  fail_unless(arc_is(&c, arc_mkstringc(&c, "foo"),
		      arc_sym2name(&c, car(sexpr))) == CTRUE);
  fail_unless(TYPE(car(cdr(sexpr))) == T_FIXNUM);
  fail_unless(FIX2INT((car(cdr(sexpr)))) == 1);
  fail_unless(TYPE(car(cdr(cdr(sexpr)))) == T_FIXNUM);
  fail_unless(FIX2INT((car(cdr(cdr(sexpr))))) == 2);
  fail_unless(TYPE(car(cdr(cdr(cdr(sexpr))))) == T_FIXNUM);
  fail_unless(FIX2INT((car(cdr(cdr(cdr(sexpr)))))) == 3);

  str = arc_mkstringc(&c, "(foo (bar 4) 5)");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(TYPE(car(sexpr)) == T_SYMBOL);
  fail_unless(arc_is(&c, arc_mkstringc(&c, "foo"),
		      arc_sym2name(&c, car(sexpr))) == CTRUE);
  fail_unless(TYPE(car(cdr(sexpr))) == T_CONS);
  fail_unless(TYPE(car(car(cdr(sexpr)))) == T_SYMBOL);
  fail_unless(arc_is(&c, arc_mkstringc(&c, "bar"),
		      arc_sym2name(&c, car(car(cdr(sexpr))))) == CTRUE);
  fail_unless(TYPE(car(cdr(car(cdr(sexpr))))) == T_FIXNUM);
  fail_unless(FIX2INT(car(cdr(car(cdr(sexpr))))) == 4);
  fail_unless(TYPE(car(cdr(cdr(sexpr)))) == T_FIXNUM);
  fail_unless(FIX2INT(car(cdr(cdr(sexpr)))) == 5);

  str = arc_mkstringc(&c, "(1 2 . 3)");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(car(sexpr) == INT2FIX(1));
  fail_unless(car(cdr(sexpr)) == INT2FIX(2));
  fail_unless(cdr(cdr(sexpr)) == INT2FIX(3));
}
END_TEST

START_TEST(test_character)
{
  value str, sexpr, fp;

  str = arc_mkstringc(&c, "#\\a");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CHAR);
  fail_unless(REP(sexpr)._char == 'a');

  /* Issue #15 */
  str = arc_mkstringc(&c, "#\\;");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CHAR);
  fail_unless(REP(sexpr)._char == ';');

  str = arc_mkstringc(&c, "#\\\351\276\215");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CHAR);
  fail_unless(REP(sexpr)._char == 0x9f8d);

  str = arc_mkstringc(&c, "#\\102");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CHAR);
  fail_unless(REP(sexpr)._char == 'B');

  str = arc_mkstringc(&c, "#\\102");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CHAR);
  fail_unless(REP(sexpr)._char == 'B');

  str = arc_mkstringc(&c, "#\\newline");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CHAR);
  fail_unless(REP(sexpr)._char == '\n');

  str = arc_mkstringc(&c, "#\\null");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CHAR);
  fail_unless(REP(sexpr)._char == '\0');

  str = arc_mkstringc(&c, "#\\u5a");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CHAR);
  fail_unless(REP(sexpr)._char == 0x5a);

  str = arc_mkstringc(&c, "#\\x5a");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CHAR);
  fail_unless(REP(sexpr)._char == 0x5a);

  str = arc_mkstringc(&c, "#\\u4e9c");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CHAR);
  fail_unless(REP(sexpr)._char == 0x4e9c);

  str = arc_mkstringc(&c, "#\\U12031");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CHAR);
  fail_unless(REP(sexpr)._char == 0x12031);

  str = arc_mkstringc(&c, "#\\\346\227\245");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CHAR);
  fail_unless(REP(sexpr)._char == 0x65e5);

}
END_TEST

START_TEST(test_bracketfn)
{
  value str, sexpr, fnbody, fp;

  /* This bracket function should become (fn (_) (+ 1 _)) */
  str = arc_mkstringc(&c, "[+ 1 _]");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(TYPE(car(sexpr)) == T_SYMBOL);
  fail_unless(car(sexpr) == ARC_BUILTIN(cc, S_FN));
  fail_unless(TYPE(car(cdr(sexpr))) == T_CONS);
  fail_unless(TYPE(car(car(cdr(sexpr)))) == T_SYMBOL);
  fail_unless(car(car(cdr(sexpr))) == ARC_BUILTIN(cc, S_US));
  fnbody = car(cdr(cdr(sexpr)));
  fail_unless(TYPE(fnbody) == T_CONS);
  fail_unless(TYPE(car(fnbody)) == T_SYMBOL);
  fail_unless(car(fnbody) == arc_intern(&c, arc_mkstringc(&c, "+")));
  fail_unless(TYPE(car(cdr(fnbody))) == T_FIXNUM);
  fail_unless(FIX2INT(car(cdr(fnbody))) == 1);
  fail_unless(TYPE(car(cdr(cdr(fnbody)))) == T_SYMBOL);
  fail_unless(car(cdr(cdr(fnbody))) == ARC_BUILTIN(cc, S_US));
}
END_TEST

START_TEST(test_quote)
{
  value str, sexpr, qexpr, fp;

  str = arc_mkstringc(&c, "'(+ 1 2)");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(car(sexpr)) == T_SYMBOL);
  fail_unless(car(sexpr) == ARC_BUILTIN(cc, S_QUOTE));
  qexpr = car(cdr(sexpr));
  fail_unless(TYPE(qexpr) == T_CONS);
  fail_unless(TYPE(car(qexpr)) == T_SYMBOL);
  fail_unless(car(qexpr) == arc_intern(&c, arc_mkstringc(&c, "+")));
  fail_unless(TYPE(car(cdr(qexpr))) == T_FIXNUM);
  fail_unless(FIX2INT(car(cdr(qexpr))) == 1);
  fail_unless(TYPE(car(cdr(cdr(qexpr)))) == T_FIXNUM);
  fail_unless(FIX2INT(car(cdr(cdr(qexpr)))) == 2);

  str = arc_mkstringc(&c, "`(* 3 4)");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(TYPE(car(sexpr)) == T_SYMBOL);
  fail_unless(car(sexpr) == ARC_BUILTIN(cc, S_QQUOTE));
  qexpr = car(cdr(sexpr));
  fail_unless(TYPE(qexpr) == T_CONS);
  fail_unless(TYPE(car(qexpr)) == T_SYMBOL);
  fail_unless(car(qexpr) == arc_intern(&c, arc_mkstringc(&c, "*")));
  fail_unless(TYPE(car(cdr(qexpr))) == T_FIXNUM);
  fail_unless(FIX2INT(car(cdr(qexpr))) == 3);
  fail_unless(TYPE(car(cdr(cdr(qexpr)))) == T_FIXNUM);
  fail_unless(FIX2INT(car(cdr(cdr(qexpr)))) == 4);
}
END_TEST

START_TEST(test_ssyntax)
{
  value str, sexpr, fp;

  str = arc_mkstringc(&c, "~");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_SYMBOL);
  fail_unless(sexpr == ARC_BUILTIN(cc, S_NO));

  str = arc_mkstringc(&c, "a:");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_SYMBOL);
  fail_unless(sexpr == arc_intern(&c, arc_mkstringc(&c, "a")));

  str = arc_mkstringc(&c, ":a");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_SYMBOL);
  fail_unless(sexpr == arc_intern(&c, arc_mkstringc(&c, "a")));

  str = arc_mkstringc(&c, "a:b");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(car(sexpr) == ARC_BUILTIN(cc, S_COMPOSE));
  fail_unless(TYPE(car(cdr(sexpr))) == T_SYMBOL);
  fail_unless(car(cdr(sexpr)) == arc_intern(&c, arc_mkstringc(&c, "a")));
  fail_unless(TYPE(car(cdr(cdr(sexpr)))) == T_SYMBOL);
  fail_unless(car(cdr(cdr(sexpr))) == arc_intern(&c, arc_mkstringc(&c, "b")));

  str = arc_mkstringc(&c, "a:b:c");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(car(sexpr) == ARC_BUILTIN(cc, S_COMPOSE));
  fail_unless(TYPE(car(cdr(sexpr))) == T_SYMBOL);
  fail_unless(car(cdr(sexpr)) == arc_intern(&c, arc_mkstringc(&c, "a")));
  fail_unless(TYPE(car(cdr(cdr(sexpr)))) == T_SYMBOL);
  fail_unless(car(cdr(cdr(sexpr))) == arc_intern(&c, arc_mkstringc(&c, "b")));
  fail_unless(TYPE(car(cdr(cdr(cdr(sexpr))))) == T_SYMBOL);
  fail_unless(car(cdr(cdr(cdr(sexpr)))) == arc_intern(&c, arc_mkstringc(&c, "c")));

  str = arc_mkstringc(&c, "~a");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(car(sexpr) == ARC_BUILTIN(cc, S_COMPLEMENT));
  fail_unless(TYPE(car(cdr(sexpr))) == T_SYMBOL);
  fail_unless(car(cdr(sexpr)) == arc_intern(&c, arc_mkstringc(&c, "a")));

  str = arc_mkstringc(&c, "a:~b:c");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(car(sexpr) == ARC_BUILTIN(cc, S_COMPOSE));
  fail_unless(TYPE(car(cdr(sexpr))) == T_SYMBOL);
  fail_unless(car(cdr(sexpr)) == arc_intern(&c, arc_mkstringc(&c, "a")));
  fail_unless(TYPE(car(cdr(cdr(sexpr)))) == T_CONS);
  fail_unless(car(car(cdr(cdr(sexpr)))) == ARC_BUILTIN(cc, S_COMPLEMENT));
  fail_unless(TYPE(car(cdr(car(cdr(cdr(sexpr)))))) == T_SYMBOL);
  fail_unless(car(cdr(car(cdr(cdr(sexpr))))) == arc_intern(&c, arc_mkstringc(&c, "b")));
  fail_unless(TYPE(car(cdr(cdr(cdr(sexpr))))) == T_SYMBOL);
  fail_unless(car(cdr(cdr(cdr(sexpr)))) == arc_intern(&c, arc_mkstringc(&c, "c")));

  sexpr = arc_ssexpand(&c, arc_intern(&c, arc_mkstringc(&c, "a:~b:c")));
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(car(sexpr) == ARC_BUILTIN(cc, S_COMPOSE));
  fail_unless(TYPE(car(cdr(sexpr))) == T_SYMBOL);
  fail_unless(car(cdr(sexpr)) == arc_intern(&c, arc_mkstringc(&c, "a")));
  fail_unless(TYPE(car(cdr(cdr(sexpr)))) == T_CONS);
  fail_unless(car(car(cdr(cdr(sexpr)))) == ARC_BUILTIN(cc, S_COMPLEMENT));
  fail_unless(TYPE(car(cdr(car(cdr(cdr(sexpr)))))) == T_SYMBOL);
  fail_unless(car(cdr(car(cdr(cdr(sexpr))))) == arc_intern(&c, arc_mkstringc(&c, "b")));
  fail_unless(TYPE(car(cdr(cdr(cdr(sexpr))))) == T_SYMBOL);
  fail_unless(car(cdr(cdr(cdr(sexpr)))) == arc_intern(&c, arc_mkstringc(&c, "c")));

  str = arc_mkstringc(&c, "a&");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_SYMBOL);
  fail_unless(sexpr == arc_intern(&c, arc_mkstringc(&c, "a")));

  str = arc_mkstringc(&c, "&a");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_SYMBOL);
  fail_unless(sexpr == arc_intern(&c, arc_mkstringc(&c, "a")));

  str = arc_mkstringc(&c, "a&b");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(car(sexpr) == ARC_BUILTIN(cc, S_ANDF));
  fail_unless(TYPE(car(cdr(sexpr))) == T_SYMBOL);
  fail_unless(car(cdr(sexpr)) == arc_intern(&c, arc_mkstringc(&c, "a")));
  fail_unless(TYPE(car(cdr(cdr(sexpr)))) == T_SYMBOL);
  fail_unless(car(cdr(cdr(sexpr))) == arc_intern(&c, arc_mkstringc(&c, "b")));

  str = arc_mkstringc(&c, "a&b&c");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(car(sexpr) == ARC_BUILTIN(cc, S_ANDF));
  fail_unless(TYPE(car(cdr(sexpr))) == T_SYMBOL);
  fail_unless(car(cdr(sexpr)) == arc_intern(&c, arc_mkstringc(&c, "a")));
  fail_unless(TYPE(car(cdr(cdr(sexpr)))) == T_SYMBOL);
  fail_unless(car(cdr(cdr(sexpr))) == arc_intern(&c, arc_mkstringc(&c, "b")));
  fail_unless(TYPE(car(cdr(cdr(cdr(sexpr))))) == T_SYMBOL);
  fail_unless(car(cdr(cdr(cdr(sexpr)))) == arc_intern(&c, arc_mkstringc(&c, "c")));

  str = arc_mkstringc(&c, "a.b");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(TYPE(car(sexpr)) == T_SYMBOL);
  fail_unless(car(sexpr) == arc_intern(&c, arc_mkstringc(&c, "a")));
  fail_unless(car(cdr(sexpr)) == arc_intern(&c, arc_mkstringc(&c, "b")));

  str = arc_mkstringc(&c, "a.b.c");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(TYPE(car(sexpr)) == T_CONS);
  fail_unless(car(car(sexpr)) == arc_intern(&c, arc_mkstringc(&c, "a")));
  fail_unless(car(cdr(car(sexpr))) == arc_intern(&c, arc_mkstringc(&c, "b")));
  fail_unless(car(cdr(sexpr)) == arc_intern(&c, arc_mkstringc(&c, "c")));

  str = arc_mkstringc(&c, ".a");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(TYPE(car(sexpr)) == T_SYMBOL);
  fail_unless(car(sexpr) == ARC_BUILTIN(cc, S_GET));
  fail_unless(car(cdr(sexpr)) == arc_intern(&c, arc_mkstringc(&c, "a")));

  str = arc_mkstringc(&c, "a!b");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(TYPE(car(sexpr)) == T_SYMBOL);
  fail_unless(car(sexpr) == arc_intern(&c, arc_mkstringc(&c, "a")));
  fail_unless(car(car(cdr(sexpr))) == ARC_BUILTIN(cc, S_QUOTE));
  fail_unless(car(cdr(car(cdr(sexpr)))) == arc_intern(&c, arc_mkstringc(&c, "b")));

  str = arc_mkstringc(&c, "!a");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(TYPE(car(sexpr)) == T_SYMBOL);
  fail_unless(car(sexpr) == ARC_BUILTIN(cc, S_GET));
  fail_unless(car(cdr(sexpr)) == ARC_BUILTIN(cc, S_QUOTE));
  fail_unless(car(cdr(cdr(sexpr))) == arc_intern(&c, arc_mkstringc(&c, "a")));
}
END_TEST

START_TEST(test_comment)
{
  value str, sexpr, fp;

  /* inline comments */
  str = arc_mkstringc(&c, "(123 ; Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.\n456)");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(TYPE(car(sexpr)) == T_FIXNUM);
  fail_unless(INT2FIX(123) == car(sexpr));
  fail_unless(TYPE(cdr(sexpr)) == T_CONS);
  fail_unless(TYPE(car(cdr(sexpr))) == T_FIXNUM);
  fail_unless(INT2FIX(456) == car(cdr(sexpr)));
  fail_unless(cdr(cdr(sexpr)) == CNIL);

  /* long comments */
  str = arc_mkstringc(&c, "; Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed\n; do eiusmod tempor incididunt ut labore et dolore magna aliqua.\n; Ut enim ad minim veniam, quis nostrud exercitation ullamco\n; laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure\n; dolor in reprehenderit in voluptate velit esse cillum dolore eu\n; fugiat nulla pariatur. Excepteur sint occaecat cupidatat non\n; proident, sunt in culpa qui officia deserunt mollit anim id\n; est laborum.\n(789 897)");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(TYPE(car(sexpr)) == T_FIXNUM);
  fail_unless(INT2FIX(789) == car(sexpr));
  fail_unless(TYPE(cdr(sexpr)) == T_CONS);
  fail_unless(TYPE(car(cdr(sexpr))) == T_FIXNUM);
  fail_unless(INT2FIX(897) == car(cdr(sexpr)));
  fail_unless(cdr(cdr(sexpr)) == CNIL);
}
END_TEST

START_TEST(test_comma)
{
  value str, fp, sexpr;

  str = arc_mkstringc(&c, ",1");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(TYPE(car(sexpr)) == T_SYMBOL);
  fail_unless(car(sexpr) == ARC_BUILTIN(cc, S_UNQUOTE));
  fail_unless(car(cdr(sexpr)) == INT2FIX(1));

  str = arc_mkstringc(&c, ",@2");
  fp = arc_instring(&c, str, CNIL);
  sexpr = arc_read(&c, fp, CNIL);
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(TYPE(car(sexpr)) == T_SYMBOL);
  fail_unless(car(sexpr) == ARC_BUILTIN(cc, S_UNQUOTESP));
  fail_unless(car(cdr(sexpr)) == INT2FIX(2));
}
END_TEST

extern unsigned long long gcepochs;

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Reader");
  TCase *tc_reader = tcase_create("Reader");
  SRunner *sr;
  unsigned long long oldepoch;

  arc_set_memmgr(&c);
  arc_init_reader(&c);

  oldepoch = gcepochs;
  while (gcepochs == oldepoch) {
    c.rungc(&c);
  }

  oldepoch = gcepochs;
  while (gcepochs == oldepoch) {
    c.rungc(&c);
  }

  oldepoch = gcepochs;
  while (gcepochs == oldepoch) {
    c.rungc(&c);
  }

  cc = &c;
  tcase_add_test(tc_reader, test_atom);
  tcase_add_test(tc_reader, test_string);
  tcase_add_test(tc_reader, test_list);
  tcase_add_test(tc_reader, test_character);
  tcase_add_test(tc_reader, test_bracketfn);
  tcase_add_test(tc_reader, test_quote);
  tcase_add_test(tc_reader, test_ssyntax);
  tcase_add_test(tc_reader, test_comment);
  tcase_add_test(tc_reader, test_comma);

  suite_add_tcase(s, tc_reader);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
