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

#define TEST(sexpr)				\
  COMPILE(sexpr);				\
  cctx = TVALR(thr);				\
  code = arc_cctx2code(c, cctx);		\
  clos = arc_mkclos(c, code, CNIL);		\
  XCALL0(clos);					\
  ret = TVALR(thr)

AFFDEF(compile_something)
{
  AARG(something);
  value sexpr;
  AVAR(sio);
  AFBEGIN;
  TQUANTA(thr) = QUANTA;	/* needed so macros can execute */
  WV(sio, arc_instring(c, AV(something), CNIL));
  AFCALL(arc_mkaff(c, arc_sread, CNIL), AV(sio), CNIL);
  sexpr = AFCRV;
  AFTCALL(arc_mkaff(c, arc_compile, CNIL), sexpr, arc_mkcctx(c), CNIL, CTRUE);
  AFEND;
}
AFFEND

#define COMPILE(str) XCALL(compile_something, arc_mkstringc(c, str))

START_TEST(test_compile_nil)
{
  value thr, cctx, clos, code;

  thr = arc_mkthread(c);

  COMPILE("nil");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(NIL_P(TVALR(thr)));
}
END_TEST

START_TEST(test_compile_t)
{
  value thr, cctx, clos, code;

  thr = arc_mkthread(c);

  COMPILE("t");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(TVALR(thr) == CTRUE);
}
END_TEST

START_TEST(test_compile_char)
{
  value thr, cctx, clos, code;

  thr = arc_mkthread(c);

  COMPILE("#\\a");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(TYPE(TVALR(thr)) == T_CHAR);
  fail_unless(arc_char2rune(c, TVALR(thr)) == 'a');
}
END_TEST

START_TEST(test_compile_string)
{
  value thr, cctx, clos, code;

  thr = arc_mkthread(c);

  COMPILE("\"foo\"");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(TYPE(TVALR(thr)) == T_STRING);
  fail_unless(arc_is2(c, arc_mkstringc(c, "foo"), TVALR(thr)) == CTRUE);
}
END_TEST

START_TEST(test_compile_fixnum)
{
  value thr, cctx, clos, code;

  thr = arc_mkthread(c);

  COMPILE("31337");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(TVALR(thr) == INT2FIX(31337));
}
END_TEST

#ifdef HAVE_GMP_H

START_TEST(test_compile_bignum)
{
  value thr, cctx, clos, code;
  mpz_t expected;

  thr = arc_mkthread(c);

  COMPILE("300000000000000000000000000000");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(TYPE(TVALR(thr)) == T_BIGNUM);
  mpz_init(expected);
  mpz_set_str(expected, "300000000000000000000000000000", 10);
  fail_unless(mpz_cmp(expected, REPBNUM(TVALR(thr))) == 0);
  mpz_clear(expected);
}
END_TEST

START_TEST(test_compile_rational)
{
  value thr, cctx, clos, code;

  thr = arc_mkthread(c);

  COMPILE("1/2");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(TYPE(TVALR(thr)) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REPRAT(TVALR(thr)), 1, 2) == 0);
}
END_TEST

#endif

START_TEST(test_compile_complex)
{
  value thr, cctx, clos, code;

  thr = arc_mkthread(c);

  COMPILE("1.1+2.2i");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(TYPE(TVALR(thr)) == T_COMPLEX);
  fail_unless(cabs(REPCPX(TVALR(thr)) - (1.1+I*2.2)) < 1e-6);
}
END_TEST

START_TEST(test_compile_flonum)
{
  value thr, cctx, clos, code;

  thr = arc_mkthread(c);

  COMPILE("3.14159");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(TYPE(TVALR(thr)) == T_FLONUM);
  fail_unless(fabs(REPFLO(TVALR(thr)) - 3.14159) < 1e-6);
}
END_TEST

START_TEST(test_compile_gsym)
{
  value thr, cctx, clos, code;
  value sym = arc_intern_cstr(c, "foo");

  /* global symbol binding for foo */
  arc_hash_insert(c, c->genv, sym, INT2FIX(31337));
  thr = arc_mkthread(c);

  COMPILE("foo");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(TYPE(TVALR(thr)) == T_FIXNUM);
  fail_unless(TVALR(thr) == INT2FIX(31337));
}
END_TEST

/* simple case of an empty if, should just be nil */
START_TEST(test_compile_if_empty)
{
  value thr, cctx, clos, code;

  thr = arc_mkthread(c);

  COMPILE("(if)");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(NIL_P(TVALR(thr)));
}
END_TEST

/* should compile (if x) to simply x */
START_TEST(test_compile_if_x)
{
  value thr, cctx, clos, code;

  thr = arc_mkthread(c);

  COMPILE("(if 4)");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(TVALR(thr) == INT2FIX(4));
}
END_TEST

/* should compile a full if statement */
START_TEST(test_compile_if_full)
{
  value thr, cctx, clos, code;

  thr = arc_mkthread(c);

  COMPILE("(if t 1 2)");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(TVALR(thr) == INT2FIX(1));

  COMPILE("(if nil 1 2)");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(TVALR(thr) == INT2FIX(2));
}
END_TEST

/* should compile a partial if statement */
START_TEST(test_compile_if_partial)
{
  value thr, cctx, clos, code;

  thr = arc_mkthread(c);

  COMPILE("(if t 1)");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(TVALR(thr) == INT2FIX(1));

  COMPILE("(if nil 1)");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(NIL_P(TVALR(thr)));
}
END_TEST

/* should compile a compound if statement */
START_TEST(test_compile_if_compound)
{
  value thr, cctx, clos, code;

  thr = arc_mkthread(c);

  COMPILE("(if t 1 t 3 t 5 6)");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(TVALR(thr) == INT2FIX(1));

  COMPILE("(if nil 1 t 3 t 5 6)");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(TVALR(thr) == INT2FIX(3));

  COMPILE("(if nil 1 nil 3 t 5 6)");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(TVALR(thr) == INT2FIX(5));

  COMPILE("(if nil 1 nil 3 nil 5 6)");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(TVALR(thr) == INT2FIX(6));
}
END_TEST

AFFDEF(testfunc)
{
  AARG(arg1, arg2);
  AFBEGIN;
  ARETURN(INT2FIX(FIX2INT(AV(arg1)) + FIX2INT(AV(arg2))));
  AFEND;
}
AFFEND

START_TEST(test_compile_apply)
{
  value thr, cctx, clos, code;
  value sym = arc_intern_cstr(c, "foo");

  /* global symbol binding for foo */
  arc_hash_insert(c, c->genv, sym, arc_mkaff(c, testfunc, CNIL));
  thr = arc_mkthread(c);

  COMPILE("(foo (foo 1 2) (foo 3 4))");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(TYPE(TVALR(thr)) == T_FIXNUM);
  fail_unless(TVALR(thr) == INT2FIX(10));
}
END_TEST

START_TEST(test_compile_fn_basic)
{
  value thr, cctx, clos, code;

  thr = arc_mkthread(c);

  COMPILE("((fn (a b c) a) 1 2 3)");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(TYPE(TVALR(thr)) == T_FIXNUM);
  fail_unless(TVALR(thr) == INT2FIX(1));

  COMPILE("((fn (a b c) b) 1 2 3)");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(TYPE(TVALR(thr)) == T_FIXNUM);
  fail_unless(TVALR(thr) == INT2FIX(2));

  COMPILE("((fn (a b c) c) 1 2 3)");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(TYPE(TVALR(thr)) == T_FIXNUM);
  fail_unless(TVALR(thr) == INT2FIX(3));

  COMPILE("((fn a a) 1 2 3)");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(TYPE(TVALR(thr)) == T_CONS);
  fail_unless(car(TVALR(thr)) == INT2FIX(1));
  fail_unless(cadr(TVALR(thr)) == INT2FIX(2));
  fail_unless(car(cddr(TVALR(thr))) == INT2FIX(3));

  COMPILE("((fn (a . b) a) 1 2 3)");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(TYPE(TVALR(thr)) == T_FIXNUM);
  fail_unless(TVALR(thr) == INT2FIX(1));

  COMPILE("((fn (a . b) b) 1 2 3)");
  cctx = TVALR(thr);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  XCALL0(clos);
  fail_unless(TYPE(TVALR(thr)) == T_CONS);
  fail_unless(car(TVALR(thr)) == INT2FIX(2));
  fail_unless(cadr(TVALR(thr)) == INT2FIX(3));

}
END_TEST

START_TEST(test_compile_fn_oarg)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);

  TEST("((fn (a (o b)) a) 1 2)");
  fail_unless(ret == INT2FIX(1));

  TEST("((fn (a (o b)) b) 1 2)");
  fail_unless(ret == INT2FIX(2));

  TEST("((fn (a (o b)) b) 1)");
  fail_unless(NIL_P(ret));

  TEST("((fn (a (o b 7)) a) 1)");
  fail_unless(ret == INT2FIX(1));

  TEST("((fn (a (o b 7)) b) 1)");
  fail_unless(ret == INT2FIX(7));

  TEST("((fn (a (o b a)) a) 1)");
  fail_unless(ret == INT2FIX(1));

  TEST("((fn (a (o b a)) b) 1)");
  fail_unless(ret == INT2FIX(1));
}
END_TEST

START_TEST(test_compile_fn_dsb)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);

  TEST("((fn (a (b c) d) a) 1 '(2 3) 4)");
  fail_unless(ret == INT2FIX(1));

  TEST("((fn (a (b c) d) b) 1 '(2 3) 4)");
  fail_unless(ret == INT2FIX(2));

  TEST("((fn (a (b c) d) c) 1 '(2 3) 4)");
  fail_unless(ret == INT2FIX(3));

  TEST("((fn (a (b c) d) d) 1 '(2 3) 4)");
  fail_unless(ret == INT2FIX(4));

  /* something more complicated */
  TEST("((fn (a (b c (d e) f) g) a) 1 '(2 3 (4 5) 6) 7)");
  fail_unless(ret == INT2FIX(1));

  TEST("((fn (a (b c (d e) f) g) b) 1 '(2 3 (4 5) 6) 7)");
  fail_unless(ret == INT2FIX(2));

  TEST("((fn (a (b c (d e) f) g) c) 1 '(2 3 (4 5) 6) 7)");
  fail_unless(ret == INT2FIX(3));

  TEST("((fn (a (b c (d e) f) g) d) 1 '(2 3 (4 5) 6) 7)");
  fail_unless(ret == INT2FIX(4));

  TEST("((fn (a (b c (d e) f) g) e) 1 '(2 3 (4 5) 6) 7)");
  fail_unless(ret == INT2FIX(5));

  TEST("((fn (a (b c (d e) f) g) f) 1 '(2 3 (4 5) 6) 7)");
  fail_unless(ret == INT2FIX(6));

  TEST("((fn (a (b c (d e) f) g) g) 1 '(2 3 (4 5) 6) 7)");
  fail_unless(ret == INT2FIX(7));

  /* extra destructuring binds */
  TEST("((fn (a (b . c) d) a) 1 '(2 3 4) 5)");
  fail_unless(ret == INT2FIX(1));

  TEST("((fn (a (b . c) d) b) 1 '(2 3 4) 5)");
  fail_unless(ret == INT2FIX(2));

  TEST("((fn (a (b . c) d) c) 1 '(2 3 4) 5)");
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(3));
  fail_unless(cadr(ret) == INT2FIX(4));

  TEST("((fn (a (b . c) d) d) 1 '(2 3 4) 5)");
  fail_unless(ret == INT2FIX(5));

  /* Destructuring binds with optional arguments */
  TEST("((fn (a (b c (o d 123) e) f) d) 1 '(2 3 4) 4)");
  fail_unless(ret == INT2FIX(4));

  /* Destructuring binds with optional arguments */
  TEST("((fn (a (b c (o d 123) e) f) d) 1 '(2 3) 4)");
  fail_unless(ret == INT2FIX(123));

  TEST("((fn (a (b c (o d 123) e) f) d) 1 '(2 3 nil) 4)");
  fail_unless(NIL_P(ret));

  TEST("((fn (a (b c (o d 123) e) f) e) 1 '(2 3 nil) 4)");
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_compile_quote)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);

  TEST("'(1 2 3)");
  fail_unless(TYPE(ret) == T_CONS);
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(3));
}
END_TEST

START_TEST(test_compile_qquote)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);

  TEST("((fn (a b) `(0 ,a ,@b 5 6)) 1 '(2 3 4))");
  fail_unless(TYPE(ret) == T_CONS);
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(0));
  fail_unless(car(cdr(ret)) == INT2FIX(1));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(2));
  fail_unless(car(cdr(cdr(cdr(ret)))) == INT2FIX(3));
  fail_unless(car(cdr(cdr(cdr(cdr(ret))))) == INT2FIX(4));
  fail_unless(car(cdr(cdr(cdr(cdr(cdr(ret)))))) == INT2FIX(5));
  fail_unless(car(cdr(cdr(cdr(cdr(cdr(cdr(ret))))))) == INT2FIX(6));
}
END_TEST

START_TEST(test_compile_assign)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(assign leet 1337 eleet 31337)");
  ret = arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "leet"));
  fail_unless(ret == INT2FIX(1337));
  ret = arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "eleet"));
  fail_unless(ret == INT2FIX(31337));

  /* local variable assignment */
  TEST("((fn (x y) (assign x 12345) x) 54321 6789)");
  fail_unless(ret == INT2FIX(12345));
}
END_TEST

START_TEST(test_compile_inline_cons)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(cons 1 (cons 2 (cons 3 nil)))");
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(3));
}
END_TEST

START_TEST(test_compile_inline_car)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(car '(1 2 3))");
  fail_unless(ret == INT2FIX(1));
}
END_TEST

START_TEST(test_compile_inline_cdr)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(cdr '(1 2 3))");
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(2));
  fail_unless(car(cdr(ret)) == INT2FIX(3));
}
END_TEST

START_TEST(test_compile_inline_plus)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(+ 1 2 3 4 5 6 7)");
  fail_unless(ret == INT2FIX(28));
}
END_TEST

START_TEST(test_compile_inline_minus)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(- 1)");
  fail_unless(ret == INT2FIX(-1));

  TEST("(- 3 2 1)");
  fail_unless(ret == INT2FIX(0));
}
END_TEST

START_TEST(test_compile_inline_times)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(* 1 2 3 4 5 6 7)");
  fail_unless(ret == INT2FIX(5040));
}
END_TEST

START_TEST(test_compile_inline_div)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(/ 2.0)");
  fail_unless(TYPE(ret) == T_FLONUM);
  fail_unless(fabs(REPFLO(ret) - 0.5) < 1e-6);

  TEST("(/ 8 2 2)");
  fail_unless(FIX2INT(ret) == 2);
}
END_TEST

START_TEST(test_compile_macro)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  /* define a simple macro */
  TEST("(assign foo (annotate 'mac (fn () '(+ 1 2))))");
  fail_unless(TYPE(ret) == T_TAGGED);
  fail_unless(arc_type(c, ret) == ARC_BUILTIN(c, S_MAC));

  /* Try to use the macro */
  TEST("(+ 10 (foo))");
  fail_unless(FIX2INT(ret) == 13);

  /* Define another few macros. The "do" macro */
  TEST("(assign do (annotate 'mac (fn args `((fn () ,@args)))))");
  fail_unless(TYPE(ret) == T_TAGGED);
  fail_unless(arc_type(c, ret) == ARC_BUILTIN(c, S_MAC));

  /* Try to use the "do" macro */
  TEST("(do 1 2)");
  fail_unless(FIX2INT(ret) == 2);

  /* Define another macro */
  TEST("(assign when (annotate 'mac (fn (test . body) `(if ,test (do ,@body)))))");
  fail_unless(TYPE(ret) == T_TAGGED);
  fail_unless(arc_type(c, ret) == ARC_BUILTIN(c, S_MAC));

  TEST("(when 1 2 3)");
  fail_unless(FIX2INT(ret) == 3);

}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Compiler");
  TCase *tc_compiler = tcase_create("Compiler");
  SRunner *sr;

  c = &cc;
  arc_init(c);

  tcase_add_test(tc_compiler, test_compile_nil);
  tcase_add_test(tc_compiler, test_compile_t);
  tcase_add_test(tc_compiler, test_compile_char);
  tcase_add_test(tc_compiler, test_compile_string);
  tcase_add_test(tc_compiler, test_compile_fixnum);

#ifdef HAVE_GMP_H
  tcase_add_test(tc_compiler, test_compile_bignum);
  tcase_add_test(tc_compiler, test_compile_rational);
#endif

  tcase_add_test(tc_compiler, test_compile_flonum);
  tcase_add_test(tc_compiler, test_compile_complex);
  tcase_add_test(tc_compiler, test_compile_gsym);
  tcase_add_test(tc_compiler, test_compile_if_empty);
  tcase_add_test(tc_compiler, test_compile_if_x);
  tcase_add_test(tc_compiler, test_compile_if_full);
  tcase_add_test(tc_compiler, test_compile_if_partial);
  tcase_add_test(tc_compiler, test_compile_if_compound);
  tcase_add_test(tc_compiler, test_compile_apply);
  tcase_add_test(tc_compiler, test_compile_fn_basic);
  tcase_add_test(tc_compiler, test_compile_fn_oarg);
  tcase_add_test(tc_compiler, test_compile_fn_dsb);
  tcase_add_test(tc_compiler, test_compile_quote);
  tcase_add_test(tc_compiler, test_compile_qquote);
  tcase_add_test(tc_compiler, test_compile_assign);
  tcase_add_test(tc_compiler, test_compile_inline_cons);
  tcase_add_test(tc_compiler, test_compile_inline_car);
  tcase_add_test(tc_compiler, test_compile_inline_cdr);
  tcase_add_test(tc_compiler, test_compile_inline_plus);
  tcase_add_test(tc_compiler, test_compile_inline_times);
  tcase_add_test(tc_compiler, test_compile_inline_minus);
  tcase_add_test(tc_compiler, test_compile_inline_div);
  tcase_add_test(tc_compiler, test_compile_macro);

  suite_add_tcase(s, tc_compiler);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}


