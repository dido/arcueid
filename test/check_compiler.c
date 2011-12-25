/* 
  Copyright (C) 2011 Rafael R. Sevilla

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
#include "../src/arcueid.h"
#include "../src/vmengine.h"
#include "../src/symbols.h"
#include "../config.h"

arc *c, cc;

START_TEST(test_compile_nil)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "()");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CNIL);

  str = arc_mkstringc(c, "nil");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CNIL);
}
END_TEST

START_TEST(test_compile_t)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "t");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_compile_fixnum)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "123");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(123));
}
END_TEST

START_TEST(test_compile_string)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "\"foo\"");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(TYPE(ret) == T_STRING);
  fail_unless(arc_is(c, arc_mkstringc(c, "foo"), ret) == CTRUE);
}
END_TEST

START_TEST(test_compile_char)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "#\\a");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(TYPE(ret) == T_CHAR);
  fail_unless(REP(sexpr)._char == 'a');
}
END_TEST

#ifdef HAVE_GMP_H
START_TEST(test_compile_bignum)
{
  value str, sexpr, fp, cctx, code, ret;
  mpz_t expected;

  str = arc_mkstringc(c, "300000000000000000000000000000");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(TYPE(ret) == T_BIGNUM);
  mpz_init(expected);
  mpz_set_str(expected, "300000000000000000000000000000", 10);
  fail_unless(mpz_cmp(expected, REP(ret)._bignum) == 0);
  mpz_clear(expected);
}
END_TEST

START_TEST(test_compile_rational)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "1/2");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(TYPE(ret) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REP(ret)._rational, 1, 2) == 0);
}
END_TEST

#endif

START_TEST(test_compile_flonum)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "3.14159");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(TYPE(ret) == T_FLONUM);
  fail_unless(fabs(3.14159 - REP(sexpr)._flonum) < 1e-6);
}
END_TEST

START_TEST(test_compile_complex)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "1.1+2.2i");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(TYPE(ret) == T_COMPLEX);
  fail_unless(fabs(1.1 - REP(sexpr)._complex.re) < 1e-6);
  fail_unless(fabs(2.2 - REP(sexpr)._complex.im) < 1e-6);
}
END_TEST

START_TEST(test_compile_ident)
{
  value str, sexpr, fp, cctx, code, ret;

  /* Make a global symbol binding for foo*/
  arc_bindcstr(c, "foo", INT2FIX(31337));
  str = arc_mkstringc(c, "foo");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(TYPE(ret) == T_FIXNUM);
  fail_unless(ret == INT2FIX(31337));

  /* Try to compile a local symbol binding for foo as well */
  str = arc_mkstringc(c, "((fn (foo) foo) 73313)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(TYPE(ret) == T_FIXNUM);
  fail_unless(ret == INT2FIX(73313));
}
END_TEST

/* simple case of an empty if, should just be nil */
START_TEST(test_compile_if_empty)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(if)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CNIL);
}
END_TEST

/* should compile (if x) to simply x */
START_TEST(test_compile_if_x)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(if 4)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(4));
}
END_TEST

/* should compile a full if statement */
START_TEST(test_compile_if_full)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(if t 1 2)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(1));

  str = arc_mkstringc(c, "(if nil 1 2)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(2));
}
END_TEST

/* should compile a partial if statement */
START_TEST(test_compile_if_partial)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(if t 1)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(1));

  str = arc_mkstringc(c, "(if nil 1)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CNIL);
}
END_TEST

/* should compile a compound if statement */
START_TEST(test_compile_if_compound)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(if t 1 t 3 t 5 6)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(1));

  str = arc_mkstringc(c, "(if nil 1 t 3 t 5 6)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(3));

  str = arc_mkstringc(c, "(if nil 1 nil 3 t 5 6)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(5));

  str = arc_mkstringc(c, "(if nil 1 nil 3 nil 5 6)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(6));
}
END_TEST

START_TEST(test_compile_fn_basic)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "((fn (a b c) a) 1 2 3)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(1));

  str = arc_mkstringc(c, "((fn (a b c) b) 1 2 3)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(2));

  str = arc_mkstringc(c, "((fn (a b c) c) 1 2 3)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(3));

  str = arc_mkstringc(c, "((fn a a) 1 2 3)");
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

  str = arc_mkstringc(c, "((fn (a . b) a) 1 2 3 4)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(1));

  str = arc_mkstringc(c, "((fn (a . b) b) 1 2 3 4)");
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

  /* implicit progn */
  str = arc_mkstringc(c, "((fn nil 1 2))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(2));
}
END_TEST

START_TEST(test_compile_fn_oarg)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "((fn (a (o b)) a) 1 2)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(1));

  str = arc_mkstringc(c, "((fn (a (o b)) b) 1 2)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(2));

  str = arc_mkstringc(c, "((fn (a (o b)) b) 1)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CNIL);

  str = arc_mkstringc(c, "((fn (a (o b 7)) b) 1)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(7));

  str = arc_mkstringc(c, "((fn (a (o b 7)) b) 1 2)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(2));
}
END_TEST

START_TEST(test_compile_fn_dsb)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "((fn (a (b c (d e) f) g) a) 1 '(2 3 (4 5) 6) 7)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(1));

  str = arc_mkstringc(c, "((fn (a (b c (d e) f) g) b) 1 '(2 3 (4 5) 6) 7)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(2));

  str = arc_mkstringc(c, "((fn (a (b c (d e) f) g) c) 1 '(2 3 (4 5) 6) 7)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(3));

  str = arc_mkstringc(c, "((fn (a (b c (d e) f) g) d) 1 '(2 3 (4 5) 6) 7)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(4));

  str = arc_mkstringc(c, "((fn (a (b c (d e) f) g) e) 1 '(2 3 (4 5) 6) 7)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(5));

  str = arc_mkstringc(c, "((fn (a (b c (d e) f) g) f) 1 '(2 3 (4 5) 6) 7)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(6));

  str = arc_mkstringc(c, "((fn (a (b c (d e) f) g) g) 1 '(2 3 (4 5) 6) 7)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(7));
}
END_TEST

START_TEST(test_compile_quote)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "'(1 2 3)");
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

START_TEST(test_compile_qquote)
{
  value str, sexpr, fp, cctx, code, ret;

  arc_bindcstr(c, "a", INT2FIX(1));
  arc_bindcstr(c, "b", cons(c, INT2FIX(2), cons(c, INT2FIX(3), cons(c, INT2FIX(4), CNIL))));
  str = arc_mkstringc(c, "`(0 ,a ,@b 5 6)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
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
  value str, sexpr, fp, cctx, code, ret;

  /* assign to global vars */
  str = arc_mkstringc(c, "(assign leet 1337 eleet 31337)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  arc_macapply(c, code, CNIL);
  ret = arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "leet"));
  fail_unless(ret == INT2FIX(1337));
  ret = arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "eleet"));
  fail_unless(ret == INT2FIX(31337));

  /* Test assign to a local var */
  str = arc_mkstringc(c, "((fn (x y) (assign x 12345) x) 54321 6789)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(12345));
}
END_TEST

START_TEST(test_compile_inline_cons)
{
  value str, sexpr, fp, cctx, code, ret;

  /* assign to global vars */
  str = arc_mkstringc(c, "(cons 1 (cons 2 (cons 3 nil)))");
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

START_TEST(test_compile_inline_car)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(car '(1 2 3))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(1));
}
END_TEST

START_TEST(test_compile_inline_cdr)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(cdr '(1 2 3))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(2));
  fail_unless(car(cdr(ret)) == INT2FIX(3));
}
END_TEST

START_TEST(test_compile_inline_scar)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(assign xyzzy '(1 2 3))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  arc_macapply(c, code, CNIL);

  str = arc_mkstringc(c, "(scar xyzzy 4)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  arc_macapply(c, code, CNIL);
  ret = arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "xyzzy"));
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(4));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(3));
}
END_TEST

START_TEST(test_compile_inline_is)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(is 1 1)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CTRUE);

  str = arc_mkstringc(c, "(is 1 2)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CNIL);
}
END_TEST

START_TEST(test_compile_inline_plus)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(+ 1 2 3 4 5 6 7)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(28));
}
END_TEST

START_TEST(test_compile_inline_times)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(* 1 2 3 4 5 6 7)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(5040));
}
END_TEST

START_TEST(test_compile_inline_minus)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(- 1)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(FIX2INT(ret) == -1);

  str = arc_mkstringc(c, "(- 3 2 1)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(FIX2INT(ret) == 0);

}
END_TEST

START_TEST(test_compile_inline_div)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(/ 2.0)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(TYPE(ret) == T_FLONUM);
  fail_unless(fabs(REP(ret)._flonum - 0.5) < 1e-6);

  str = arc_mkstringc(c, "(/ 8 2 2)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(FIX2INT(ret) == 2);

}
END_TEST

START_TEST(test_compile_macro)
{
  value str, sexpr, fp, cctx, code, ret;

  /* define a macro */
  str = arc_mkstringc(c, "(assign foo (annotate 'mac (fn () '(+ 1 2))))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(TYPE(ret) == T_TAGGED);
  fail_unless(arc_type(c, ret) == ARC_BUILTIN(c, S_MAC));

  /* Try to use the macro */
  str = arc_mkstringc(c, "(+ 10 (foo))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(FIX2INT(ret) == 13);

  /* Define another few macros. The "do" macro */
  str = arc_mkstringc(c, "(assign do (annotate 'mac (fn args `((fn () ,@args)))))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(TYPE(ret) == T_TAGGED);
  fail_unless(arc_type(c, ret) == ARC_BUILTIN(c, S_MAC));

  /* Try to use the "do" macro */
  str = arc_mkstringc(c, "(do 1 2)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(FIX2INT(ret) == 2);

  /* Define another macro */
  str = arc_mkstringc(c, "(assign when (annotate 'mac (fn (test . body) `(if ,test (do ,@body)))))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(TYPE(ret) == T_TAGGED);
  fail_unless(arc_type(c, ret) == ARC_BUILTIN(c, S_MAC));

  str = arc_mkstringc(c, "(when 1 2 3)");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(FIX2INT(ret) == 3);
}
END_TEST

START_TEST(test_compile_if_fn)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "(if 1 ((fn () 2 3)))");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == INT2FIX(3));
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Compiler");
  TCase *tc_compiler = tcase_create("Compiler");
  SRunner *sr;

  c = &cc;
  arc_set_memmgr(c);
  cc.genv = arc_mkhash(c, 8);
  arc_init_reader(c);
  arc_init_builtins(c);
  cc.stksize = TSTKSIZE;
  cc.quantum = PQUANTA;

  tcase_add_test(tc_compiler, test_compile_nil);
  tcase_add_test(tc_compiler, test_compile_t);
  tcase_add_test(tc_compiler, test_compile_fixnum);
  tcase_add_test(tc_compiler, test_compile_string);
  tcase_add_test(tc_compiler, test_compile_char);

#ifdef HAVE_GMP_H
  tcase_add_test(tc_compiler, test_compile_bignum);
  tcase_add_test(tc_compiler, test_compile_rational);
#endif

  tcase_add_test(tc_compiler, test_compile_flonum);
  tcase_add_test(tc_compiler, test_compile_complex);
  tcase_add_test(tc_compiler, test_compile_ident);

  tcase_add_test(tc_compiler, test_compile_if_empty);
  tcase_add_test(tc_compiler, test_compile_if_x);
  tcase_add_test(tc_compiler, test_compile_if_full);
  tcase_add_test(tc_compiler, test_compile_if_partial);
  tcase_add_test(tc_compiler, test_compile_if_compound);
  tcase_add_test(tc_compiler, test_compile_if_fn);

  tcase_add_test(tc_compiler, test_compile_fn_basic);
  tcase_add_test(tc_compiler, test_compile_fn_oarg);
  tcase_add_test(tc_compiler, test_compile_fn_dsb);

  tcase_add_test(tc_compiler, test_compile_quote);
  tcase_add_test(tc_compiler, test_compile_qquote);

  tcase_add_test(tc_compiler, test_compile_assign);

  tcase_add_test(tc_compiler, test_compile_inline_cons);
  tcase_add_test(tc_compiler, test_compile_inline_car);
  tcase_add_test(tc_compiler, test_compile_inline_cdr);
  tcase_add_test(tc_compiler, test_compile_inline_scar);
  tcase_add_test(tc_compiler, test_compile_inline_is);
  tcase_add_test(tc_compiler, test_compile_inline_plus);
  tcase_add_test(tc_compiler, test_compile_inline_times);
  tcase_add_test(tc_compiler, test_compile_inline_minus);
  tcase_add_test(tc_compiler, test_compile_inline_div);

  tcase_add_test(tc_compiler, test_compile_macro);



  suite_add_tcase(s, tc_compiler);
  sr = srunner_create(s);
  /* srunner_set_fork_status(sr, CK_NOFORK); */
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
