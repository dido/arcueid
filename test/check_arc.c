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
  along with this program. If not, see <http://www.gnu.org/licenses/>
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include <math.h>
#include <unistd.h>
#include <stdarg.h>
#include "../src/arcueid.h"
#include "../src/vmengine.h"
#include "../src/symbols.h"
#include "../config.h"

arc *c, cc;

static void error_handler(struct arc *c, const char *fmt, ...)
{
  /* always fail the test if this ever gets called */
  fail(fmt);
}

START_TEST(test_do)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(do)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CNIL);

  str = arc_mkstringc(c, "(do 1)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(1));

  str = arc_mkstringc(c, "(do 1 2)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(2));

  str = arc_mkstringc(c, "(do 1 2 3)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(3));
}
END_TEST

START_TEST(test_safeset)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(safeset foo 2)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(2));
  fail_unless(arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "foo")) == INT2FIX(2));

  str = arc_mkstringc(c, "(safeset foo 3)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(3));
  fail_unless(arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "foo")) == INT2FIX(3));
}
END_TEST

START_TEST(test_apply)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(apply (fn a a) '(1 2) '(3 4))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(TYPE(ret) == T_CONS);
  fail_unless(TYPE(car(ret)) == T_CONS);
  fail_unless(car(car(ret)) == INT2FIX(1));
  fail_unless(car(cdr(car(ret))) == INT2FIX(2));
  fail_unless(TYPE(cdr(ret)) == T_CONS);
  fail_unless(car(cdr(ret)) == INT2FIX(3));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(4));
}
END_TEST

START_TEST(test_def)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(def bar (a b) (+ a b))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(TYPE(ret) == T_CLOS);

  str = arc_mkstringc(c, "(bar 42 75)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(117));
}
END_TEST

START_TEST(test_caar)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(caar '((1 2) 3 4))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(1));
}
END_TEST

START_TEST(test_cadr)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(cadr '((1 2) 3 4))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(3));
}
END_TEST

START_TEST(test_cddr)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(cddr '((1 2) 3 4))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(TYPE(ret) == T_CONS);
  fail_unless(car(ret) == INT2FIX(4));
}
END_TEST

START_TEST(test_no)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(no nil)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CTRUE);

  str = arc_mkstringc(c, "(no 1)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CNIL);
}
END_TEST

START_TEST(test_acons)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(acons nil)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CNIL);

  str = arc_mkstringc(c, "(acons 1)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CNIL);

  str = arc_mkstringc(c, "(acons '(1 2))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_copylist)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(copylist '(1 2 3))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(3));
}
END_TEST

START_TEST(test_list)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(list 1 2 3)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(3));
}
END_TEST

START_TEST(test_idfn)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(idfn '(1 2 3))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(3));

  str = arc_mkstringc(c, "(idfn 31337)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(31337));
}
END_TEST

START_TEST(test_map1)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(map1 [+ _ 1] '(1 2 3))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(2));
  fail_unless(car(cdr(ret)) == INT2FIX(3));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(4));
}
END_TEST

START_TEST(test_pair)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(pair '(1 2 3))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(CONS_P(ret));
  fail_unless(CONS_P(car(ret)));
  fail_unless(car(car(ret)) == INT2FIX(1));
  fail_unless(car(cdr(car(ret))) == INT2FIX(2));
  fail_unless(CONS_P(car(cdr(ret))));
  fail_unless(car(car(cdr(ret))) == INT2FIX(3));
}
END_TEST

START_TEST(test_mac)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(mac foofoo () (list '+ 1 2))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(TYPE(ret) == T_TAGGED);
  fail_unless(arc_type(c, ret) == ARC_BUILTIN(c, S_MAC));

  /* Try to use the macro */
  str = arc_mkstringc(c, "(+ 10 (foofoo))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(FIX2INT(ret) == 13);
}
END_TEST

START_TEST(test_and)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(and 1 2 3)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(3));

  str = arc_mkstringc(c, "(and 1 nil 3)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CNIL);

  str = arc_mkstringc(c, "(and nil 3 4)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CNIL);
}
END_TEST

START_TEST(test_assoc)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(assoc 'a '((a 1) (b 2)))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == arc_intern_cstr(c, "a"));
  fail_unless(car(cdr(ret)) == INT2FIX(1));

  str = arc_mkstringc(c, "(assoc 'b '((a 1) (b 2)))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == arc_intern_cstr(c, "b"));
  fail_unless(car(cdr(ret)) == INT2FIX(2));

  str = arc_mkstringc(c, "(assoc 'c '((a 1) (b 2)))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_alref)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(alref '((a 1) (b 2)) 'a)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(1));

  str = arc_mkstringc(c, "(alref '((a 1) (b 2)) 'b)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(2));

  str = arc_mkstringc(c, "(alref '((a 1) (b 2)) 'c)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_with)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(with (a 1 b 2) a)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(1));

  str = arc_mkstringc(c, "(with (a 1 b 2) b)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(2));

  str = arc_mkstringc(c, "(with (a 1 b 2))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_let)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(let x 1 x)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(1));

  str = arc_mkstringc(c, "(let x 1)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_withs)
{
  value str, sexpr, fp, cctx, code, ret;

  /* uses upper binding a is 3 */
  str = arc_mkstringc(c, "(let a 3 (with (a 1 b (+ a 1)) (+ a b)))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(5));

  /* uses simultaneous binding, so a is 1 */
  str = arc_mkstringc(c, "(let a 3 (withs (a 1 b (+ a 1)) (+ a b)))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(3));
}
END_TEST

START_TEST(test_join)
{
  value str, sexpr, fp, cctx, code, ret;

  /* uses upper binding a is 3 */
  str = arc_mkstringc(c, "(join '(1 2 3) '(4 5 6))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(3));
  fail_unless(car(cdr(cdr(cdr(ret)))) == INT2FIX(4));
  fail_unless(car(cdr(cdr(cdr(cdr(ret))))) == INT2FIX(5));
  fail_unless(car(cdr(cdr(cdr(cdr(cdr(ret)))))) == INT2FIX(6));
}
END_TEST

START_TEST(test_rfn)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "((rfn fact (x) (if (is x 0) 1 (* x (fact (- x 1))))) 7)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(5040));
}
END_TEST

START_TEST(test_afn)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "((afn (x) (if (is x 0) 1 (* x (self (- x 1))))) 7)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(5040));
}
END_TEST

START_TEST(test_compose)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(let div2 (fn (x) (/ x 2)) ((compose div2 len) \"abcd\"))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(2));

  str = arc_mkstringc(c, "(let div2 (fn (x) (/ x 2)) (div2:len \"abcd\"))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(2));
}
END_TEST

START_TEST(test_complement)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "((complement atom) 'foo)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));

  str = arc_mkstringc(c, "((complement atom) '(1 2 3))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CTRUE);

  str = arc_mkstringc(c, "(~atom 'foo)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));

  str = arc_mkstringc(c, "(~atom '(1 2 3))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_rev)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(rev '(1 2 3))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(3));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(1));
}
END_TEST

START_TEST(test_isnt)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(isnt 1 1)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));

  str = arc_mkstringc(c, "(isnt 1 2)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_or)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(or 1 2 3)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(1));

  str = arc_mkstringc(c, "(or nil 2 3)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(2));

  str = arc_mkstringc(c, "(or nil nil 3)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(3));

  str = arc_mkstringc(c, "(or nil nil 3)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(3));

  str = arc_mkstringc(c, "(or nil nil nil)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_in)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(in 1 0 2 3)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));

  str = arc_mkstringc(c, "(in 1 1 2 3)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_when)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(when t (assign whentest 123))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(123));
  fail_unless(arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "whentest")) == INT2FIX(123));

  str = arc_mkstringc(c, "(when nil (assign whentest 456))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));
  fail_unless(arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "whentest")) == INT2FIX(123));
}
END_TEST

START_TEST(test_unless)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(unless nil (assign whentest 123))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(123));
  fail_unless(arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "whentest")) == INT2FIX(123));

  str = arc_mkstringc(c, "(unless t (assign whentest 456))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));
  fail_unless(arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "whentest")) == INT2FIX(123));
}
END_TEST

START_TEST(test_while)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(with (x 0) (do (while (isnt x 10) (= x (+ x 1))) x))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(10));
}
END_TEST

START_TEST(test_empty)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(empty nil)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CTRUE);

  str = arc_mkstringc(c, "(empty \"\")");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CTRUE);

  str = arc_mkstringc(c, "(empty (table))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CTRUE);

  str = arc_mkstringc(c, "(empty '(1 2 3))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));

  str = arc_mkstringc(c, "(empty \"abc\")");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));

  str = arc_mkstringc(c, "(let x (table) (do (= (x 'foo) 1) (empty x)))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_reclist)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(reclist no:car '(1 2 nil 3))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CTRUE);

  str = arc_mkstringc(c, "(reclist no:car '(1 2 3))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_recstring)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(recstring [is _ 1] \"abc\")");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CTRUE);

  str = arc_mkstringc(c, "(recstring [is _ 3] \"abc\")");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_testify)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "((testify [isa _ 'sym]) 'foo)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CTRUE);

  str = arc_mkstringc(c, "((testify [isa _ 'sym]) 1)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));

  str = arc_mkstringc(c, "((testify 'foo) 'foo)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CTRUE);

  str = arc_mkstringc(c, "((testify 'foo) 'bar)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));

  str = arc_mkstringc(c, "((testify 'foo) 1)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_some)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(some 1 '(1 2 3)))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CTRUE);

  str = arc_mkstringc(c, "(some #\\a \"abc\")");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CTRUE);

  str = arc_mkstringc(c, "(some 9 '(1 2 3)))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));

  str = arc_mkstringc(c, "(some #\\z \"abc\")");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_all)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(all 1 '(1 1 1)))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CTRUE);

  str = arc_mkstringc(c, "(all #\\a \"aaa\")");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CTRUE);

  str = arc_mkstringc(c, "(all 1 '(1 2 3)))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));

  str = arc_mkstringc(c, "(all #\\a \"abc\")");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));

  str = arc_mkstringc(c, "(all 9 '(1 2 3)))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));

  str = arc_mkstringc(c, "(all #\\z \"abc\")");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_find)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(find 2 '(1 2 3)))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(2));

  str = arc_mkstringc(c, "(find #\\c \"abc\")");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(TYPE(ret) == T_CHAR);
  fail_unless(REP(ret)._char = 'c');

  str = arc_mkstringc(c, "(find 9 '(1 2 3)))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));

  str = arc_mkstringc(c, "(find #\\z \"abc\")");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(NIL_P(ret));
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("arc.arc");
  TCase *tc_arc = tcase_create("arc.arc");
  SRunner *sr;
  char *loadstr = "arc.arc";
  value initload, sexpr, cctx, code;

  c = &cc;
  arc_set_memmgr(c);
  cc.genv = arc_mkhash(c, 16);
  arc_init_reader(c);
  arc_init_builtins(c);
  cc.stksize = TSTKSIZE;
  cc.quantum = PQUANTA;
  cc.signal_error = error_handler;

  initload = arc_infile(c, arc_mkstringc(c, loadstr));
  while ((sexpr = arc_read(c, initload)) != CNIL) {
    cctx = arc_mkcctx(c, INT2FIX(1), 0);
    arc_compile(c, sexpr, cctx, CNIL, CTRUE);
    code = arc_cctx2code(c, cctx);
    arc_macapply(c, code, CNIL);
    c->rungc(c);
  }
  arc_close(c, initload);

  tcase_add_test(tc_arc, test_do);
  tcase_add_test(tc_arc, test_safeset);
  tcase_add_test(tc_arc, test_apply);
  tcase_add_test(tc_arc, test_def);
  tcase_add_test(tc_arc, test_caar);
  tcase_add_test(tc_arc, test_cadr);
  tcase_add_test(tc_arc, test_cddr);
  tcase_add_test(tc_arc, test_no);
  tcase_add_test(tc_arc, test_acons);
  tcase_add_test(tc_arc, test_copylist);
  tcase_add_test(tc_arc, test_list);
  tcase_add_test(tc_arc, test_idfn);
  tcase_add_test(tc_arc, test_map1);
  tcase_add_test(tc_arc, test_pair);
  tcase_add_test(tc_arc, test_mac);
  tcase_add_test(tc_arc, test_and);
  tcase_add_test(tc_arc, test_assoc);
  tcase_add_test(tc_arc, test_alref);
  tcase_add_test(tc_arc, test_with);
  tcase_add_test(tc_arc, test_let);
  tcase_add_test(tc_arc, test_withs);
  tcase_add_test(tc_arc, test_join);
  tcase_add_test(tc_arc, test_rfn);
  tcase_add_test(tc_arc, test_afn);
  tcase_add_test(tc_arc, test_compose);
  tcase_add_test(tc_arc, test_complement);
  tcase_add_test(tc_arc, test_rev);
  tcase_add_test(tc_arc, test_isnt);
  /* w/uniq is used in many other macros so any problems with it should
     manifest when we test the other functions. */
  tcase_add_test(tc_arc, test_or);
  tcase_add_test(tc_arc, test_in);
  tcase_add_test(tc_arc, test_when);
  tcase_add_test(tc_arc, test_unless);
  tcase_add_test(tc_arc, test_while);
  tcase_add_test(tc_arc, test_empty);
  tcase_add_test(tc_arc, test_reclist);
  tcase_add_test(tc_arc, test_recstring);
  tcase_add_test(tc_arc, test_testify);
  tcase_add_test(tc_arc, test_some);
  tcase_add_test(tc_arc, test_all);
  tcase_add_test(tc_arc, test_find);

  suite_add_tcase(s, tc_arc);
  sr = srunner_create(s);
  /* srunner_set_fork_status(sr, CK_NOFORK); */
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

