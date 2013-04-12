
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
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include "../src/arcueid.h"
#include "../src/alloc.h"
#include "../src/arith.h"
#include "../config.h"
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

arc cc;
arc *c;

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

START_TEST(test_pp_string)
{
  value thr, cctx, clos, code, ret;
  value ppfp, result;

  thr = c->curthread;
  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);

  TEST("(write \"abc\" ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "\"abc\"")) == 0);

  /* test for some low characters */
  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write \"\\n\" ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "\"\\n\"")) == 0);

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write \"\\u0000a\" ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "\"\\n\"")) == 0);

  /* and some high characters */
  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write \"\\u09060\" ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "\"遠\"")) == 0);

  /* Tests for using disp */
  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(disp \"abc\" ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "abc")) == 0);

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(disp \"\\n\" ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "\n")) == 0);

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(disp \"\\u09060\" ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "遠")) == 0);
}
END_TEST


START_TEST(test_pp_symbol)
{
  value thr, cctx, clos, code, ret;
  value ppfp, result;

  thr = c->curthread;

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write 'foo ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "foo")) == 0);

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(disp 'foo ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "foo")) == 0);

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write nil ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "nil")) == 0);

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(disp nil ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "nil")) == 0);

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write t ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "t")) == 0);

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(disp t ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "t")) == 0);
}
END_TEST

START_TEST(test_pp_fixnum)
{
  value thr, cctx, clos, code, ret;
  value ppfp, result;

  thr = c->curthread;

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write 1234 ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "1234")) == 0);

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(disp 1234 ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "1234")) == 0);
}
END_TEST

#ifdef HAVE_GMP_H

START_TEST(test_pp_bignum)
{
  value thr, cctx, clos, code, ret;
  value ppfp, result;

  thr = c->curthread;

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write 10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000")) == 0);

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(disp 10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000")) == 0);
}
END_TEST

START_TEST(test_pp_rational)
{
  value thr, cctx, clos, code, ret;
  value ppfp, result;

  thr = c->curthread;

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write 1/2 ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "1/2")) == 0);

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(disp 1/2 ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "1/2")) == 0);
}
END_TEST

#endif

START_TEST(test_pp_flonum)
{
  value thr, cctx, clos, code, ret;
  value ppfp, result;

  thr = c->curthread;

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write 3.14159 ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "3.14159")) == 0);

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(disp 3.14159 ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "3.14159")) == 0);
}
END_TEST

START_TEST(test_pp_char)
{
  value thr, cctx, clos, code, ret;
  value ppfp, result;

  thr = c->curthread;

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write #\\u0000 ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "#\\nul")) == 0);

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write #\\nul ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "#\\nul")) == 0);

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write #\\u0009 ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "#\\tab")) == 0);

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write #\\a ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "#\\a")) == 0);

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write #\\u9f8d ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "#\\龍")) == 0);

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(disp #\\u0000 ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strlen(c, result) == 1);
  fail_unless(arc_strindex(c, result, 0) == 0);

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(disp #\\nul ppfp)");
  fail_unless(NIL_P(ret));
  fail_unless(arc_strlen(c, result) == 1);
  fail_unless(arc_strindex(c, result, 0) == 0);
  result = arc_inside(c, ppfp);

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(disp #\\u0009 ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "\t")) == 0);

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(disp #\\a ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "a")) == 0);

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(disp #\\u9f8d ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "龍")) == 0);

}
END_TEST


START_TEST(test_pp_complex)
{
  value thr, cctx, clos, code, ret;
  value ppfp, result;

  thr = c->curthread;

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write 3.14159+2.718i ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "3.14159+2.718i")) == 0);

  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(disp 3.14159+2.718i ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "3.14159+2.718i")) == 0);
}
END_TEST

START_TEST(test_pp_cons)
{
  value thr, cctx, clos, code, ret;
  value ppfp, result;
  value list1, list2, list3, list4, list5, list6;

  thr = c->curthread;

  list1 = cons(c, INT2FIX(1), cons(c, INT2FIX(2), cons(c, INT2FIX(3), CNIL)));
  arc_bindcstr(c, "lst", list1);
  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write lst ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "(1 2 3)")) == 0);

  /* Various types of circular lists */
  list2 = cons(c, INT2FIX(1), cons(c, INT2FIX(2), cons(c, INT2FIX(3), CNIL)));
  car(list2) = list2;
  arc_bindcstr(c, "lst", list2);
  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write lst ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "((...) 2 3)")) == 0);

  list3 = cons(c, INT2FIX(1), cons(c, INT2FIX(2), cons(c, INT2FIX(3), CNIL)));
  car(cdr(list3)) = list3;
  arc_bindcstr(c, "lst", list3);
  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write lst ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "(1 (...) 3)")) == 0);

  list4 = cons(c, INT2FIX(1), cons(c, INT2FIX(2), cons(c, INT2FIX(3), CNIL)));
  car(cdr(cdr(list4))) = list4;
  arc_bindcstr(c, "lst", list4);
  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write lst ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "(1 2 (...))")) == 0);

  list5 = cons(c, INT2FIX(1), cons(c, INT2FIX(2), cons(c, INT2FIX(3), CNIL)));
  cdr(cdr(cdr(list5))) = list5;
  arc_bindcstr(c, "lst", list5);
  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write lst ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "(1 2 3 . (...))")) == 0);

  list6 = cons(c, list1, cons(c, list1, CNIL));
  arc_bindcstr(c, "lst", list6);
  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write lst ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result,
			 arc_mkstringc(c, "((1 2 3) (1 2 3))")) == 0);
}
END_TEST

START_TEST(test_pp_vector)
{
  value thr, cctx, clos, code, ret, vec;
  value ppfp, result;

  thr = c->curthread;

  vec = arc_mkvector(c, 3);
  VINDEX(vec, 0) = INT2FIX(1);
  VINDEX(vec, 1) = INT2FIX(2);
  VINDEX(vec, 2) = INT2FIX(3);
  arc_bindcstr(c, "vec", vec);
  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write vec ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "#(1 2 3)")) == 0);

  /* some circularity */
  VINDEX(vec, 0) = vec;
  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write vec ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "#((...) 2 3)")) == 0);

  VINDEX(vec, 0) = INT2FIX(1);
  VINDEX(vec, 1) = vec;
  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write vec ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "#(1 (...) 3)")) == 0);

  VINDEX(vec, 1) = INT2FIX(2);
  VINDEX(vec, 2) = vec;
  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write vec ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "#(1 2 (...))")) == 0);
}
END_TEST

START_TEST(test_pp_hash)
{
  value thr, cctx, clos, code, ret, hash;
  value ppfp, result;

  thr = c->curthread;

  hash = arc_mkhash(c, 10);
  arc_hash_insert(c, hash, INT2FIX(1), INT2FIX(2));
  arc_bindcstr(c, "hash", hash);
  ppfp = arc_outstring(c, CNIL);
  arc_bindcstr(c, "ppfp", ppfp);
  TEST("(write hash ppfp)");
  fail_unless(NIL_P(ret));
  result = arc_inside(c, ppfp);
  fail_unless(arc_strcmp(c, result, arc_mkstringc(c, "#hash((1 . 2))")) == 0);
}
END_TEST

static void errhandler(arc *c, value str)
{
  fprintf(stderr, "Error\n");
  __arc_print_string(c, str);
  abort();
}

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Pretty Printer");
  TCase *tc_pp = tcase_create("Pretty Printer");
  SRunner *sr;

  c = &cc;
  arc_init(c);
  c->curthread = arc_mkthread(c);
  c->errhandler = errhandler;

  tcase_add_test(tc_pp, test_pp_fixnum);
#ifdef HAVE_GMP_H
  tcase_add_test(tc_pp, test_pp_bignum);
  tcase_add_test(tc_pp, test_pp_rational);
#endif
  tcase_add_test(tc_pp, test_pp_flonum);
  tcase_add_test(tc_pp, test_pp_complex);
  tcase_add_test(tc_pp, test_pp_symbol);
  tcase_add_test(tc_pp, test_pp_string);
  tcase_add_test(tc_pp, test_pp_char);
  tcase_add_test(tc_pp, test_pp_cons);
  tcase_add_test(tc_pp, test_pp_hash);
  tcase_add_test(tc_pp, test_pp_vector);

  suite_add_tcase(s, tc_pp);
  sr = srunner_create(s);
  /*  srunner_set_fork_status(sr, CK_NOFORK); */
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
