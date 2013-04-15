/* Do not change this comment -- the tests depend on it! */
/* 
  Copyright (C) 2013 Rafael R. Sevilla

  This file is part of Arcueid

  Arcueid is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; either version 3 of the
  License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
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

extern void __arc_print_string(arc *c, value ppstr);

arc *c, cc;

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

START_TEST(test_do)
{
  value ret, cctx, code, clos;

  TEST("(do)");
  fail_unless(ret == CNIL);

  TEST("(do 1)");
  fail_unless(ret == INT2FIX(1));

  TEST("(do 1 2)");
  fail_unless(ret == INT2FIX(2));

  TEST("(do 1 2 3)");
  fail_unless(ret == INT2FIX(3));
}
END_TEST

START_TEST(test_safeset)
{
  value ret, cctx, code, clos;

  TEST("(safeset foo 1)");
  fail_unless(ret == INT2FIX(1));

  TEST("foo");
  fail_unless(ret == INT2FIX(1));

  /*
  TEST("(safeset foo 2)");
  fail_unless(ret == INT2FIX(2));

  TEST("foo");
  fail_unless(ret == INT2FIX(2));
  */
}
END_TEST

START_TEST(test_def)
{
  value ret, cctx, code, clos;

  TEST("(def bar (x) (+ x 1))");
  fail_unless(TYPE(ret) == T_CLOS);

  TEST("(bar 10)");
  fail_unless(ret == INT2FIX(11));
}
END_TEST

START_TEST(test_caar)
{
  value ret, cctx, code, clos;

  TEST("(caar '((5 6) 7 8))");
  fail_unless(ret == INT2FIX(5));;
}
END_TEST

START_TEST(test_no)
{
  value ret, cctx, code, clos;

  TEST("(no 1)");
  fail_unless(NIL_P(ret));

  TEST("(no nil)");
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_acons)
{
  value ret, cctx, code, clos;

  TEST("(acons nil)");
  fail_unless(NIL_P(ret));

  TEST("(acons '())");
  fail_unless(NIL_P(ret));

  TEST("(acons ())");
  fail_unless(NIL_P(ret));

  TEST("(acons 1)");
  fail_unless(NIL_P(ret));

  TEST("(acons '(1 2))");
  fail_unless(ret == CTRUE);

}
END_TEST

START_TEST(test_atom)
{
  value ret, cctx, code, clos;

  TEST("(atom nil)");
  fail_unless(ret == CTRUE);

  TEST("(atom '())");
  fail_unless(ret == CTRUE);

  TEST("(atom ())");
  fail_unless(ret == CTRUE);

  TEST("(atom 1)");
  fail_unless(ret == CTRUE);

  TEST("(atom '(1 2))");
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_copylist)
{
  value ret, cctx, code, clos;

  TEST("(copylist '(1 2 3))");
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(3));
}
END_TEST

START_TEST(test_list)
{
  value ret, cctx, code, clos;

  TEST("(list 1 2 3)");
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(3));
}
END_TEST

START_TEST(test_idfn)
{
  value ret, cctx, code, clos;

  TEST("(idfn '(1 2 3))");
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(3));

  TEST("(idfn 31337)");
  fail_unless(ret == INT2FIX(31337));
}
END_TEST

START_TEST(test_map1)
{
  value ret, cctx, code, clos;

  TEST("(map1 [+ _ 1] '(1 2 3))");
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(2));
  fail_unless(car(cdr(ret)) == INT2FIX(3));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(4));
}
END_TEST

START_TEST(test_pair)
{
  value ret, cctx, code, clos;

  TEST("(pair '(1 2 3))");
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
  value ret, cctx, code, clos;

  TEST("(mac foofoo () (list '+ 1 2))");
  fail_unless(TYPE(ret) == T_TAGGED);
  fail_unless(arc_type(c, ret) == ARC_BUILTIN(c, S_MAC));

  /* Try to use the macro */
  TEST("(+ 10 (foofoo))");
  fail_unless(FIX2INT(ret) == 13);
}
END_TEST

START_TEST(test_and)
{
  value ret, cctx, code, clos;

  TEST("(and 1 2 3)");
  fail_unless(ret == INT2FIX(3));

  TEST("(and 1 nil 3)");
  fail_unless(ret == CNIL);

  TEST("(and nil 3 4)");
  fail_unless(ret == CNIL);
}
END_TEST

START_TEST(test_alref)
{
  value ret, cctx, code, clos;

  TEST("(alref '((a 1) (b 2)) 'a)");
  fail_unless(ret == INT2FIX(1));

  TEST("(alref '((a 1) (b 2)) 'b)");
  fail_unless(ret == INT2FIX(2));

  TEST("(alref '((a 1) (b 2)) 'c)");
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_assoc)
{
  value ret, cctx, code, clos;

  TEST("(assoc 'a '((a 1) (b 2)))");
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == arc_intern_cstr(c, "a"));
  fail_unless(car(cdr(ret)) == INT2FIX(1));

  TEST("(assoc 'b '((a 1) (b 2)))");
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == arc_intern_cstr(c, "b"));
  fail_unless(car(cdr(ret)) == INT2FIX(2));

  TEST("(assoc 'c '((a 1) (b 2)))");
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_with)
{
  value ret, cctx, code, clos;

  TEST("(with (a 1 b 2) a)");
  fail_unless(ret == INT2FIX(1));

  TEST("(with (a 1 b 2) b)");
  fail_unless(ret == INT2FIX(2));

  TEST("(with (a 1 b 2))");
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_let)
{
  value ret, cctx, code, clos;

  TEST("(let x 1 x)");
  fail_unless(ret == INT2FIX(1));

  TEST("(let x 1)");
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_withs)
{
  value ret, cctx, code, clos;

  /* uses upper binding a is 3 */
  TEST("(let a 3 (with (a 1 b (+ a 1)) (+ a b)))");
  fail_unless(ret == INT2FIX(5));

  /* uses simultaneous binding, so a is 1 */
  TEST("(let a 3 (withs (a 1 b (+ a 1)) (+ a b)))");
  fail_unless(ret == INT2FIX(3));
}
END_TEST

START_TEST(test_join)
{
  value ret, cctx, code, clos;

  TEST("(join '(1 2 3) '(4 5 6))");
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
  value ret, cctx, code, clos;

  TEST("((rfn fact (x) (if (is x 0) 1 (* x (fact (- x 1))))) 7)");
  fail_unless(ret == INT2FIX(5040));
}
END_TEST

START_TEST(test_afn)
{
  value ret, cctx, code, clos;

  TEST("((afn (x) (if (is x 0) 1 (* x (self (- x 1))))) 7)");
  fail_unless(ret == INT2FIX(5040));
}
END_TEST

START_TEST(test_compose)
{
  value ret, cctx, code, clos;

  TEST("(let div2 (fn (x) (/ x 2)) ((compose div2 len) \"abcd\"))");
  fail_unless(ret == INT2FIX(2));

  TEST("(let div2 (fn (x) (/ x 2)) (div2:len \"abcd\"))");
  fail_unless(ret == INT2FIX(2));
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
  Suite *s = suite_create("arc.arc");
  TCase *tc_arc = tcase_create("arc.arc");
  SRunner *sr;
  value ret, cctx, code, clos;
  int i;

  c = &cc;
  c->errhandler = errhandler;
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
    if (i == 79) {
      break;
    }
    /*
    TEST("(disp (eval sexpr))");
    TEST("(disp #\\u000a)");
    */
    TEST("(eval sexpr)");
    c->gc(c);
  }
  TEST("(close initload)");
  c->gc(c);

  tcase_add_test(tc_arc, test_do);
  tcase_add_test(tc_arc, test_safeset);
  tcase_add_test(tc_arc, test_def);
  tcase_add_test(tc_arc, test_caar);
  tcase_add_test(tc_arc, test_no);
  tcase_add_test(tc_arc, test_acons);
  tcase_add_test(tc_arc, test_atom);
  tcase_add_test(tc_arc, test_list);
  tcase_add_test(tc_arc, test_copylist);
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

  suite_add_tcase(s, tc_arc);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

