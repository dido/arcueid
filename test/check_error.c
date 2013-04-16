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
#include <stdlib.h>
#include <setjmp.h>
#include "../src/arcueid.h"
#include "../src/vmengine.h"
#include "../src/builtins.h"

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
#ifndef alloca
# define alloca __builtin_alloca
#endif
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# include <stddef.h>
void *alloca (size_t);
#endif

extern void __arc_print_string(arc *c, value ppstr);

arc *c, cc;
jmp_buf errbuf;

#define QUANTA 1048576

#define CPUSH_(val) CPUSH(c->curthread, val)

#define XCALL0(clos) do {				\
    TQUANTA(c->curthread) = QUANTA;			\
    TVALR(c->curthread) = clos;				\
    TARGC(c->curthread) = 0;				\
    __arc_thr_trampoline(c, c->curthread, TR_FNAPP);	\
  } while (0)

#define XCALL(fname, ...) do {				\
    TVALR(c->curthread) = arc_mkaff(c, fname, CNIL);	\
    TARGC(c->curthread) = NARGS(__VA_ARGS__);		\
    FOR_EACH(CPUSH_, __VA_ARGS__);			\
    __arc_thr_trampoline(c, c->curthread, TR_FNAPP);	\
  } while (0)

AFFDEF(compile_something)
{
  AARG(something);
  value sexpr;
  AVAR(sio);
  AFBEGIN;
  TQUANTA(thr) = QUANTA;	/* needed so macros can execute */
  AV(sio) = arc_instring(c, AV(something), CNIL);
  AFCALL(arc_mkaff(c, arc_sread, CNIL), AV(sio), CNIL);
  sexpr = AFCRV;
  AFTCALL(arc_mkaff(c, arc_compile, CNIL), sexpr, arc_mkcctx(c), CNIL, CTRUE);
  AFEND;
}
AFFEND

#define COMPILE(str) XCALL(compile_something, arc_mkstringc(c, str))

#define TEST(sexpr)				\
  COMPILE(sexpr);				\
  cctx = TVALR(c->curthread);			\
  code = arc_cctx2code(c, cctx);		\
  clos = arc_mkclos(c, code, CNIL);		\
  XCALL0(clos);					\
  ret = TVALR(c->curthread)

static void errhandler(arc *c, value str)
{
  longjmp(errbuf, 1);
}

static void errhandler2(arc *c, value str)
{
  fprintf(stderr, "Error\n");
  __arc_print_string(c, str);
  longjmp(errbuf, 1);
}


START_TEST(test_divzero_fixnum)
{
  value ret, cctx, code, clos;

  c->errhandler = errhandler;
  if (setjmp(errbuf) == 1) {
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
  value ret, cctx, code, clos;

  c->errhandler = errhandler;
  if (setjmp(errbuf) == 1) {
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
  value ret, cctx, code, clos;
  value details;

  c->errhandler = errhandler2;
  if (setjmp(errbuf) == 1) {
    fail("on-err did not catch the exception!");
    return;
  }
  TEST("(on-err (fn (x) x) (fn () (err \"test raising an error\")))");
  fail_unless(TYPE(ret) == T_EXCEPTION);
  details = arc_details(c, ret);
  fail_unless(FIX2INT(arc_strcmp(c, details, arc_mkstringc(c, "test raising an error"))) == 0);
}
END_TEST

START_TEST(test_on_err_nested)
{
  value ret, cctx, code, clos;

  c->errhandler = errhandler2;
  if (setjmp(errbuf) == 1) {
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
  value ret, cctx, code, clos;

  TEST("(+ 1 (ccc (fn (c) (c 41) 43)))");
  fail_unless(ret == INT2FIX(42));
}
END_TEST

START_TEST(test_protect_noerr)
{
  value ret, cctx, code, clos;

  TEST("(with (x 0 y 0) (list (+ 3 (protect (fn () (protect (fn () (+ x y)) (fn () (++ y)))) (fn () (= x (+ y 1))))) x y))");
  fail_unless(car(ret) == INT2FIX(3));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(1));
}
END_TEST

START_TEST(test_protect_ccc)
{
  value ret, cctx, code, clos;

  TEST("(with (x 0 y 0 cont nil) (list (+ 1 (ccc (fn (c) (= cont c) (protect (fn () (protect (fn () (cont 100)) (fn () (++ y)))) (fn () (++ x)))))) x y))");
  fail_unless(car(ret) == INT2FIX(101));
  fail_unless(car(cdr(ret)) == INT2FIX(1));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(1));
}
END_TEST

/* First case of err.  No on-err rescue functions registered */
START_TEST(test_protect_err1)
{
  value ret, cctx, code, clos;

  c->errhandler = errhandler2;
  if (setjmp(errbuf) == 1) {
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
  value ret, cctx, code, clos;

  c->errhandler = errhandler2;
  if (setjmp(errbuf) == 1) {
    fail("on-err did not catch the exception!");
    return;
  }

  TEST("(with (x nil exc nil) (list (+ 1 (on-err (fn (ex) (= exc ex) (+ x 1)) (fn () (protect (fn () (err \"test\")) (fn () (= x 1)))))) x exc))");
  fail_unless(car(ret) == INT2FIX(3));

  /* This should set y to 1, and x to y+2, and the return value of on-err
     should be 1 + x + y */
  TEST("(with (x nil y nil exc nil) (list (+ 1 (on-err (fn (ex) (= exc ex) (+ x y)) (fn () (protect (fn () (protect (fn () (err \"test raising an error\")) (fn () (= y 1)))) (fn () (= x (+ y 2))))))) x y exc))");
  fail_unless(car(ret) == INT2FIX(5));
  fail_unless(car(cdr(ret)) == INT2FIX(3));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(1));
  fail_unless(TYPE(car(cdr(cdr(cdr(ret))))) == T_EXCEPTION);
}
END_TEST

START_TEST(test_after)
{
  value ret, cctx, code, clos;

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
  Suite *s = suite_create("error handling");
  TCase *tc_err = tcase_create("error handling");
  SRunner *sr;
  value ret, cctx, code, clos;
  int i;

  c = &cc;
  c->errhandler = errhandler;

  if (setjmp(errbuf) != 0) {
    printf("unhandled error received\n");
    abort();
  }
  arc_init(c);
  c->curthread = arc_mkthread(c);
  /* Load arc.arc into our system */
  TEST("(assign initload (infile \"./arc.arc\"))");
  if (NIL_P(ret)) {
    fprintf(stderr, "failed to load arc.arc");
    return(EXIT_FAILURE);
  }
  i=0;
  for (;;) {
    i++;
    TEST("(assign sexpr (sread initload nil))");
    if (ret == CNIL)
      break;
    /*
    printf("%d: ", i);
    TEST("(disp sexpr)");
    TEST("(disp #\\u000a)");
    */
    /*
    TEST("(disp (eval sexpr))");
    TEST("(disp #\\u000a)");
    */
    TEST("(eval sexpr)");
    c->gc(c);
  }
  TEST("(close initload)");
  c->gc(c);

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
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

