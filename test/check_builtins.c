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
#include <stdarg.h>
#include "../src/arcueid.h"
#include "../src/alloc.h"
#include "../config.h"
#include "../src/vmengine.h"

arc c;

static value test_builtin(const char *symname, int argc, ...)
{
  value sym, cctx, thr, func;;
  int contofs, base, i;
  va_list ap;

  cctx = arc_mkcctx(&c, 1, 1);
  sym = arc_intern_cstr(&c, symname);
  VINDEX(CCTX_LITS(cctx), 0) = sym;
  base = FIX2INT(CCTX_VCPTR(cctx));
  arc_gcode1(&c, cctx, icont, 0);
  contofs = FIX2INT(CCTX_VCPTR(cctx)) - 1;
  va_start(ap, argc);
  for (i=0; i<argc; i++) {
    arc_gcode1(&c, cctx, ildi, va_arg(ap, value));
    arc_gcode(&c, cctx, ipush);
  }
  va_end(ap);
  arc_gcode1(&c, cctx, ildg, 0);
  arc_gcode1(&c, cctx, iapply, argc);
  VINDEX(CCTX_VCODE(cctx), contofs) = FIX2INT(CCTX_VCPTR(cctx)) - base;
  arc_gcode(&c, cctx, ihlt);
  func = arc_mkcode(&c, CCTX_VCODE(cctx), arc_mkstringc(&c, "test"), CNIL,
		    1);
  CODE_LITERAL(func, 0) = VINDEX(CCTX_LITS(cctx), 0);
  thr = arc_mkthread(&c, func, 2048, 0);
  arc_vmengine(&c, thr, 1000);
  return(TVALR(thr));
}

START_TEST(test_builtin_is)
{
  fail_unless(test_builtin("is", 2, INT2FIX(31337), INT2FIX(31337)) == CTRUE);
  fail_unless(test_builtin("is", 2, INT2FIX(31338), INT2FIX(31337)) == CNIL);
}
END_TEST

START_TEST(test_builtin_iso)
{
  fail_unless(test_builtin("iso", 2, INT2FIX(31337), INT2FIX(31337)) == CTRUE);
  fail_unless(test_builtin("iso", 2, INT2FIX(31338), INT2FIX(31337)) == CNIL);
}
END_TEST

START_TEST(test_builtin_gt)
{
  fail_unless(test_builtin(">", 2, INT2FIX(31337), INT2FIX(31338)) == CTRUE);
  fail_unless(test_builtin(">", 2, INT2FIX(31337), INT2FIX(31337)) == CNIL);
  fail_unless(test_builtin(">", 2, INT2FIX(31338), INT2FIX(31337)) == CNIL);
}
END_TEST

START_TEST(test_builtin_lt)
{
  fail_unless(test_builtin("<", 2, INT2FIX(31338), INT2FIX(31337)) == CTRUE);
  fail_unless(test_builtin("<", 2, INT2FIX(31337), INT2FIX(31338)) == CNIL);
  fail_unless(test_builtin("<", 2, INT2FIX(31337), INT2FIX(31337)) == CNIL);
}
END_TEST

START_TEST(test_builtin_gte)
{
  fail_unless(test_builtin(">=", 2, INT2FIX(31337), INT2FIX(31338)) == CTRUE);
  fail_unless(test_builtin(">=", 2, INT2FIX(31337), INT2FIX(31337)) == CTRUE);
  fail_unless(test_builtin(">=", 2, INT2FIX(31338), INT2FIX(31337)) == CNIL);
}
END_TEST

START_TEST(test_builtin_lte)
{
  fail_unless(test_builtin("<=", 2, INT2FIX(31338), INT2FIX(31337)) == CTRUE);
  fail_unless(test_builtin("<=", 2, INT2FIX(31337), INT2FIX(31338)) == CNIL);
  fail_unless(test_builtin("<=", 2, INT2FIX(31337), INT2FIX(31337)) == CTRUE);
}
END_TEST

START_TEST(test_builtin_bound)
{
  fail_unless(test_builtin("bound", 1, arc_intern_cstr(&c, "bound")) == CTRUE);
  fail_unless(test_builtin("bound", 1, arc_intern_cstr(&c, "foo")) == CNIL);
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Built-in Functions");
  TCase *tc_bif = tcase_create("Built-in Functions");
  SRunner *sr;

  arc_set_memmgr(&c);
  arc_init_reader(&c);
  c.genv = arc_mkhash(&c, 8);
  arc_init_builtins(&c);

  tcase_add_test(tc_bif, test_builtin_is);
  tcase_add_test(tc_bif, test_builtin_iso);
  tcase_add_test(tc_bif, test_builtin_gt);
  tcase_add_test(tc_bif, test_builtin_lt);
  tcase_add_test(tc_bif, test_builtin_gte);
  tcase_add_test(tc_bif, test_builtin_lte);
  tcase_add_test(tc_bif, test_builtin_bound);

  suite_add_tcase(s, tc_bif);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

