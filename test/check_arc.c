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
  fail_unless(car(ret) == INT2FIX(3));
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

  suite_add_tcase(s, tc_arc);
  sr = srunner_create(s);
  /* srunner_set_fork_status(sr, CK_NOFORK); */
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

