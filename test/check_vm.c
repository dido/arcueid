/* 
  Copyright (C) 2010 Rafael R. Sevilla

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
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include <math.h>
#include <stdio.h>
#include "../src/arcueid.h"
#include "../src/alloc.h"
#include "../config.h"
#include "../src/vmengine.h"

arc c;

#define ITEST_HEADER(nlits) \
  value cctx, thr, func; \
  int i; \
  cctx = arc_mkcctx(&c, 1, nlits)

#define ITEST_FOOTER(nlits) \
  func = arc_mkcode(&c, CCTX_VCODE(cctx), arc_mkstringc(&c, "test"), CNIL, nlits); \
  for (i=0; i<nlits; i++) \
    CODE_LITERAL(func, i) = VINDEX(CCTX_LITS(cctx), i); \
  thr = arc_mkthread(&c, func, 2048, 0); \
  arc_vmengine(&c, thr, 1000)

START_TEST(test_vm_push)
{
  ITEST_HEADER(0);
  arc_gcode(&c, cctx, inil);
  arc_gcode(&c, cctx, ipush);
  arc_gcode(&c, cctx, itrue);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, ipush);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(*(TSP(thr)+1) == INT2FIX(31337));
  fail_unless(*(TSP(thr)+2) == CTRUE);
  fail_unless(*(TSP(thr)+3) == CNIL);
}
END_TEST

START_TEST(test_vm_pop)
{
  ITEST_HEADER(0);
  arc_gcode(&c, cctx, inil);
  arc_gcode(&c, cctx, ipush);
  arc_gcode(&c, cctx, itrue);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, ipush);
  arc_gcode(&c, cctx, ipop);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == INT2FIX(31337));
  fail_unless(*(TSP(thr)+1) == CTRUE);
  fail_unless(*(TSP(thr)+2) == CNIL);
}
END_TEST

START_TEST(test_vm_ldi)
{
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, inop);
  arc_gcode(&c, cctx, ihlt);
  func = arc_mkcode(&c, CCTX_VCODE(cctx), arc_mkstringc(&c, "test"), CNIL, 0);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == INT2FIX(31337));
}
END_TEST

START_TEST(test_vm_ldl)
{
  ITEST_HEADER(1);
  VINDEX(CCTX_LITS(cctx), 0) = arc_mkflonum(&c, 3.1415926535);
  arc_gcode1(&c, cctx, ildl, 0);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(1);
  fail_unless(TYPE(TVALR(thr)) == T_FLONUM);
  fail_unless(fabs(REP(TVALR(thr))._flonum - 3.1415926535) < 1e-6);
}
END_TEST

START_TEST(test_vm_ldg)
{
  value sym;
  ITEST_HEADER(1);
  sym = arc_intern_cstr(&c, "foo");
  VINDEX(CCTX_LITS(cctx), 0) = sym;
  arc_hash_insert(&c, c.genv, sym, INT2FIX(31337));
  arc_gcode1(&c, cctx, ildg, 0);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(1);
  fail_unless(TYPE(TVALR(thr)) == T_FIXNUM);
  fail_unless(TVALR(thr) == INT2FIX(31337));
}
END_TEST

START_TEST(test_vm_stg)
{
  value sym;
  ITEST_HEADER(1);
  sym = arc_intern_cstr(&c, "foo");
  VINDEX(CCTX_LITS(cctx), 0) = sym;
  arc_hash_insert(&c, c.genv, sym, INT2FIX(0));
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode1(&c, cctx, istg, 0);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(1);
  fail_unless(arc_hash_lookup(&c, c.genv, sym) == INT2FIX(31337));
}
END_TEST

/* environment instructions */
START_TEST(test_vm_envs)
{
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ienv, 2);
  arc_gcode(&c, cctx, itrue);
  arc_gcode2(&c, cctx, iste, 0, 0);
  arc_gcode1(&c, cctx, ildi, INT2FIX(1234));
  arc_gcode2(&c, cctx, iste, 0, 1);
  arc_gcode1(&c, cctx, ienv, 2);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode2(&c, cctx, iste, 0, 0);
  arc_gcode1(&c, cctx, ildi, INT2FIX(73313));
  arc_gcode2(&c, cctx, iste, 0, 1);
  arc_gcode2(&c, cctx, ilde, 1, 1);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == INT2FIX(1234));
  fail_unless(ENV_VALUE(car(TENVR(thr)), 0) == INT2FIX(31337));
  fail_unless(ENV_VALUE(car(TENVR(thr)), 1) == INT2FIX(73313));
}
END_TEST

static int error = 0;

static void signal_error_test(struct arc *c, const char *fmt, ...)
{
  error = 1;
}

START_TEST(test_vm_mvarg)
{
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ienv, 2);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31338));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, imvarg, 0);
  arc_gcode1(&c, cctx, imvarg, 1);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(ENV_VALUE(car(TENVR(thr)), 0) == INT2FIX(31338));
  fail_unless(ENV_VALUE(car(TENVR(thr)), 1) == INT2FIX(31337));
}
END_TEST

START_TEST(test_vm_mvarg_fail)
{
  /* test failure */
  c.signal_error = signal_error_test;
  error = 0;
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ienv, 2);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, imvarg, 0);
  arc_gcode1(&c, cctx, imvarg, 1);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(error == 1);
}
END_TEST

START_TEST(test_vm_mvoarg)
{
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ienv, 2);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, imvoarg, 0);
  arc_gcode1(&c, cctx, imvoarg, 1);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(ENV_VALUE(car(TENVR(thr)), 0) == INT2FIX(31337));
  fail_unless(ENV_VALUE(car(TENVR(thr)), 1) == CNIL);
}
END_TEST

START_TEST(test_vm_mvrarg)
{
  value rarg;

  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ienv, 1);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31338));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, imvrarg, 0);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  rarg = ENV_VALUE(car(TENVR(thr)), 0);
  fail_unless(car(rarg) == INT2FIX(31337));
  fail_unless(car(cdr(rarg)) == INT2FIX(31338));
  fail_unless(cdr(cdr(rarg)) == CNIL);
}
END_TEST

START_TEST(test_vm_true)
{
  ITEST_HEADER(0);
  arc_gcode(&c, cctx, itrue);
  arc_gcode(&c, cctx, inop);
  arc_gcode(&c, cctx, ihlt);
  func = arc_mkcode(&c, CCTX_VCODE(cctx), arc_mkstringc(&c, "test"), CNIL, 0);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == CTRUE);
}
END_TEST

START_TEST(test_vm_nil)
{
  ITEST_HEADER(1);
  arc_gcode(&c, cctx, inil);
  arc_gcode(&c, cctx, inop);
  arc_gcode(&c, cctx, ihlt);
  func = arc_mkcode(&c, CCTX_VCODE(cctx), arc_mkstringc(&c, "test"), CNIL, 0);
  ITEST_FOOTER(1);
  fail_unless(TVALR(thr) == CNIL);
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Virtual Machine");
  TCase *tc_vm = tcase_create("Virtual Machine");
  SRunner *sr;

  arc_set_memmgr(&c);
  arc_init_reader(&c);
  c.genv = arc_mkhash(&c, 8);

  tcase_add_test(tc_vm, test_vm_push);
  tcase_add_test(tc_vm, test_vm_pop);
  tcase_add_test(tc_vm, test_vm_ldi);
  tcase_add_test(tc_vm, test_vm_ldl);
  tcase_add_test(tc_vm, test_vm_ldg);
  tcase_add_test(tc_vm, test_vm_stg);
  tcase_add_test(tc_vm, test_vm_envs);
  tcase_add_test(tc_vm, test_vm_mvarg);
  tcase_add_test(tc_vm, test_vm_mvarg_fail);
  tcase_add_test(tc_vm, test_vm_mvoarg);
  tcase_add_test(tc_vm, test_vm_mvrarg);
  tcase_add_test(tc_vm, test_vm_true);
  tcase_add_test(tc_vm, test_vm_nil);

  suite_add_tcase(s, tc_vm);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

