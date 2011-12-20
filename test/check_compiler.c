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

arc *c, cc;

START_TEST(test_compile_nil)
{
  value str, sexpr, fp, cctx, code, ret;

  str = arc_mkstringc(c, "()");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(ret == CNIL);

  str = arc_mkstringc(c, "nil");
  fp = arc_instring(c, str);
  sexpr = arc_read(c, fp);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CTRUE);
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
  arc_compile(c, sexpr, cctx, CTRUE);
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
  arc_compile(c, sexpr, cctx, CTRUE);
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
  arc_compile(c, sexpr, cctx, CTRUE);
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
  arc_compile(c, sexpr, cctx, CTRUE);
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
  arc_compile(c, sexpr, cctx, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(TYPE(ret) == T_BIGNUM);
  fail_unless(REP(sexpr)._char == 'a');
  mpz_init(expected);
  mpz_set_str(expected, "300000000000000000000000000000", 10);
  fail_unless(mpz_cmp(expected, REP(sum)._bignum) == 0);
  mpz_clear(expected);
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
  arc_compile(c, sexpr, cctx, CTRUE);
  code = arc_cctx2code(c, cctx);
  ret = arc_macapply(c, code, CNIL);
  fail_unless(TYPE(ret) == T_FLONUM);
  fail_unless(fabs(3.14159 - REP(sexpr)._flonum) < 1e-6);
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
  arc_init_reader(c);
  cc.genv = arc_mkhash(c, 8);
  cc.stksize = TSTKSIZE;
  cc.quantum = PQUANTA;

  tcase_add_test(tc_compiler, test_compile_nil);
  tcase_add_test(tc_compiler, test_compile_t);
  tcase_add_test(tc_compiler, test_compile_fixnum);
  tcase_add_test(tc_compiler, test_compile_string);
  tcase_add_test(tc_compiler, test_compile_char);

#ifdef HAVE_GMP_H
  tcase_add_test(tc_compiler, test_compile_bignum);
#endif

  tcase_add_test(tc_compiler, test_compile_flonum);

  suite_add_tcase(s, tc_compiler);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
