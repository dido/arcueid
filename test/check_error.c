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

arc *c, cc;

#define QUANTA 65536

#define CPUSH_(val) CPUSH(thr, val)

#define XCALL0(clos) do {			\
    TQUANTA(thr) = QUANTA;			\
    TVALR(thr) = clos;				\
    TARGC(thr) = 0;				\
    __arc_thr_trampoline(c, thr, TR_FNAPP);	\
  } while (0)

#define XCALL(fname, ...) do {			\
    TVALR(thr) = arc_mkaff(c, fname, CNIL);	\
    TARGC(thr) = NARGS(__VA_ARGS__);		\
    FOR_EACH(CPUSH_, __VA_ARGS__);		\
    __arc_thr_trampoline(c, thr, TR_FNAPP);	\
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
  cctx = TVALR(thr);				\
  code = arc_cctx2code(c, cctx);		\
  clos = arc_mkclos(c, code, CNIL);		\
  XCALL0(clos);					\
  ret = TVALR(thr)

START_TEST(test_dynamic_wind_noerr)
{
  value thr, cctx, clos, code, ret;
  thr = arc_mkthread(c);

  TEST("(assign protect (annotate 'mac (fn (during after) `(dynamic-wind (fn ()) ,during ,after))))");
  TEST("(assign list (fn args args))");

  TEST("((fn (x y cont) (list (+ 3 (protect (fn () (protect (fn () (+ x y)) (fn () (assign y (+ y 1))))) (fn () (assign x (+ y 1))))) x y)) 0 0 nil)");
  fail_unless(car(ret) == INT2FIX(3));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(1));
}
END_TEST

START_TEST(test_dynamic_wind_ccc)
{
  value thr, cctx, clos, code, ret;
  thr = arc_mkthread(c);

  TEST("(assign protect (annotate 'mac (fn (during after) `(dynamic-wind (fn ()) ,during ,after))))");
  TEST("(assign list (fn args args))");

  TEST("((fn (x y cont) (list (+ 1 (ccc (fn (c) (assign cont c) (protect (fn () (protect (fn () (cont 100)) (fn () (assign y (+ y 1))))) (fn () (assign x (+ x 1))))))) x y)) 0 0 nil)");
  fail_unless(car(ret) == INT2FIX(101));
  fail_unless(car(cdr(ret)) == INT2FIX(1));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(1));
}
END_TEST

START_TEST(test_on_err_noerr)
{
  value thr, cctx, clos, code, ret;
  thr = arc_mkthread(c);

  TEST("(on-err (fn (err) (details err)) (fn () 1))");
  fail_unless(ret == INT2FIX(1));
}
END_TEST

START_TEST(test_on_err)
{
  value thr, cctx, clos, code, ret;
  thr = arc_mkthread(c);

  c->curthread = thr;
  TEST("(on-err (fn (err) (details err)) (fn () (err \"bad\")))");
  fail_unless(TYPE(ret) == T_STRING);
  fail_unless(arc_strcmp(c, ret, arc_mkstringc(c, "bad")) == 0);
}
END_TEST

START_TEST(test_on_err_cstrfmt)
{
  value thr, cctx, clos, code, ret;
  thr = arc_mkthread(c);

  c->curthread = thr;
  TEST("(on-err (fn (err) (details err)) (fn () (/ 1 0)))");
  fail_unless(TYPE(ret) == T_STRING);
  fail_unless(arc_strcmp(c, ret, arc_mkstringc(c, "Division by zero")) == 0);
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Error Handling");
  TCase *tc_err = tcase_create("Error Handling");
  SRunner *sr;

  c = &cc;
  arc_init(c);

  tcase_add_test(tc_err, test_dynamic_wind_noerr);
  tcase_add_test(tc_err, test_dynamic_wind_ccc);
  tcase_add_test(tc_err, test_on_err_noerr);
  tcase_add_test(tc_err, test_on_err);
  tcase_add_test(tc_err, test_on_err_cstrfmt);

  suite_add_tcase(s, tc_err);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

