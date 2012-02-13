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
#include <setjmp.h>
#include "../src/arcueid.h"
#include "../src/builtin.h"
#include "../src/vmengine.h"
#include "../src/symbols.h"
#include "../config.h"

arc *c, cc;
jmp_buf err_jmp_buf;
value current_exception;

#define TEST(testexpr) {				\
  value str, sexpr, fp, cctx, code;			\
  str = arc_mkstringc(c, testexpr);			\
  fp = arc_instring(c, str, CNIL);			\
  sexpr = arc_read(c, fp, CNIL);			\
  cctx = arc_mkcctx(c, INT2FIX(1), 0);			\
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);		\
  code = arc_cctx2code(c, cctx);			\
  ret = arc_macapply(c, code, CNIL, 0); }

static void error_handler(struct arc *c, value err)
{
  current_exception = err;
  longjmp(err_jmp_buf, 1);
}

START_TEST(test_divzero_fixnum)
{
  value ret;

  if (setjmp(err_jmp_buf) == 1) {
    fail_unless(1);		/* success */
    return;
  }
  TEST("(/ 1 0)");
  fail("division by zero exception not raised");
  fail_unless(ret);
}
END_TEST

START_TEST(test_err)
{
  value ret;

  if (setjmp(err_jmp_buf) == 1) {
    fail_unless(1);		/* success */
    return;
  }
  TEST("(err \"test raising an error\")");
  fail("err did not raise an exception");
  fail_unless(ret);
}
END_TEST

START_TEST(test_on_err)
{
  value ret, details;

  if (setjmp(err_jmp_buf) == 1) {
    fail("on-err did not catch the exception!");
    return;
  }
  TEST("(on-err (fn (x) x) (fn () (err \"test raising an error\")))");
  fail_unless(TYPE(ret) == T_EXCEPTION);
  details = arc_exc_details(c, ret);
  fail_unless(FIX2INT(arc_strcmp(c, details, arc_mkstringc(c, "test raising an error"))) == 0);
}
END_TEST

START_TEST(test_on_err_nested)
{
  value ret;

  if (setjmp(err_jmp_buf) == 1) {
    fail("on-err did not catch the exception!");
    return;
  }
  /* this should evaluate to 4 as the continuations are unwound */
  TEST("(+ (let x 1 (+ (on-err (fn (x) 1) (fn () (+ 100 (err \"raise error\")))) x)) 2)");
  fail_unless(ret == INT2FIX(4));
}
END_TEST

START_TEST(test_ccc)
{
  value ret;

  TEST("(+ 1 (ccc (fn (c) (c 41) 43)))");
  fail_unless(ret == INT2FIX(42));
}
END_TEST

START_TEST(test_protect_noerr)
{
  value ret;

  TEST("(with (x 0 y 0) (list (+ 3 (protect (fn () (protect (fn () (+ x y)) (fn () (++ y)))) (fn () (= x (+ y 1))))) x y))");
  fail_unless(car(ret) == INT2FIX(3));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(1));
}
END_TEST

START_TEST(test_protect_ccc)
{
  value ret;

  TEST("(with (x 0 y 0 cont nil) (list (+ 1 (ccc (fn (c) (= cont c) (protect (fn () (protect (fn () (cont 100)) (fn () (++ y)))) (fn () (++ x)))))) x y))");
  fail_unless(car(ret) == INT2FIX(101));
  fail_unless(car(cdr(ret)) == INT2FIX(1));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(1));
}
END_TEST

/* First case of err.  No on-err rescue functions registered */
START_TEST(test_protect_err1)
{
  value ret;

  if (setjmp(err_jmp_buf) == 1) {
    /* Success.  Check if the global variables that were mentioned in the
       protect clauses were set appropriately */
    TEST("(and (is x 3) (is y 2))");
    fail_unless(ret == CTRUE);
    return;
  }
  TEST("(do (= x nil) (= y nil) (protect (fn () (protect (fn () (err \"test raising an error\")) (fn () (= y 2)))) (fn () (= x (+ y 1)))))");
  fail("err did not raise an exception");
  fail_unless(ret);
}
END_TEST

/* Second case of err.  On-err rescue functions registered */
START_TEST(test_protect_err2)
{
  value ret;

  if (setjmp(err_jmp_buf) == 1) {
    fail("on-err did not catch the exception!");
    return;
  }
  /* This should set y to 1, and x to y+2, and the return value of on-err
     should be 1 + x + y */
  TEST("(with (x nil y nil exc nil) (list (+ 1 (on-err (fn (ex) (= exc ex) (+ x y)) (fn () (protect (fn () (protect (fn () (err \"test raising an error\")) (fn () (= y 1)))) (fn () (= x (+ y 2))))))) x y exc))");
  fail_unless(car(ret) == INT2FIX(5));
  fail_unless(car(cdr(ret)) == INT2FIX(3));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(1));
  fail_unless(TYPE(car(cdr(cdr(cdr(ret))))) == T_EXCEPTION);
}
END_TEST

/* compare with the tests for protect in check_error.c */
START_TEST(test_after)
{
  value ret;

  TEST("(with (x nil y nil) (list (+ 1 (on-err (fn (ex) (= exc ex) (+ x y)) (fn () (after (after (err \"test\") (= y 1)) (= x (+ y 2)))))) x y exc))");
  fail_unless(car(ret) == INT2FIX(5));
  fail_unless(car(cdr(ret)) == INT2FIX(3));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(1));
  fail_unless(TYPE(car(cdr(cdr(cdr(ret))))) == T_EXCEPTION);
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Error handling");
  TCase *tc_err = tcase_create("Error handling");
  SRunner *sr;
  char *loadstr = "arc.arc";
  value initload, sexpr, cctx, code;

  c = &cc;
  arc_init(c);
  c->signal_error = error_handler;

  initload = arc_infile(c, arc_mkstringc(c, loadstr));
  arc_bindsym(c, arc_intern_cstr(c, "initload"), initload);
  while ((sexpr = arc_read(c, initload, CNIL)) != CNIL) {
    cctx = arc_mkcctx(c, INT2FIX(1), 0);
    arc_compile(c, sexpr, cctx, CNIL, CTRUE);
    code = arc_cctx2code(c, cctx);
    arc_macapply(c, code, CNIL, 0);
    c->rungc(c);
  }
  arc_close(c, initload);

  tcase_add_test(tc_err, test_divzero_fixnum);
  tcase_add_test(tc_err, test_err);
  tcase_add_test(tc_err, test_on_err);
  tcase_add_test(tc_err, test_on_err_nested);
  tcase_add_test(tc_err, test_ccc);
  tcase_add_test(tc_err, test_protect_noerr);
  tcase_add_test(tc_err, test_protect_ccc);
  tcase_add_test(tc_err, test_protect_err1);
  tcase_add_test(tc_err, test_protect_err2);
  tcase_add_test(tc_err, test_after);

  suite_add_tcase(s, tc_err);
  sr = srunner_create(s);
  /* srunner_set_fork_status(sr, CK_NOFORK); */
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
