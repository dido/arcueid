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

START_TEST(test_builtin_is)
{
  value sym;
  int contofs, base;

  ITEST_HEADER(1);
  sym = arc_intern_cstr(&c, "is");
  VINDEX(CCTX_LITS(cctx), 0) = sym;
  base = FIX2INT(CCTX_VCPTR(cctx));
  arc_gcode1(&c, cctx, icont, 0);
  contofs = FIX2INT(CCTX_VCPTR(cctx)) - 1;
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildg, 0);
  arc_gcode1(&c, cctx, iapply, 2);
  VINDEX(CCTX_VCODE(cctx), contofs) = FIX2INT(CCTX_VCPTR(cctx)) - base;
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(1);
  fail_unless(TVALR(thr) == CTRUE);
}
END_TEST

START_TEST(test_builtin_iso)
{
  value sym;
  int contofs, base;

  ITEST_HEADER(1);
  sym = arc_intern_cstr(&c, "iso");
  VINDEX(CCTX_LITS(cctx), 0) = sym;
  base = FIX2INT(CCTX_VCPTR(cctx));
  arc_gcode1(&c, cctx, icont, 0);
  contofs = FIX2INT(CCTX_VCPTR(cctx)) - 1;
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildg, 0);
  arc_gcode1(&c, cctx, iapply, 2);
  VINDEX(CCTX_VCODE(cctx), contofs) = FIX2INT(CCTX_VCPTR(cctx)) - base;
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(1);
  fail_unless(TVALR(thr) == CTRUE);
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

  suite_add_tcase(s, tc_bif);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

