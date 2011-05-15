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
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA
  02110-1301 USA.
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

START_TEST(test_vm_basic)
{
  value cctx, thr, func;

  cctx = arc_mkcctx(&c, 15, 0);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, inop);
  arc_gcode(&c, cctx, ihlt);
  func = arc_mkcode(&c, CCTX_VCODE(cctx), arc_mkstringc(&c, "test"), CNIL, 0);
  thr = arc_mkthread(&c, func, 2048, 0);
  arc_vmengine(&c, thr, 1000);
  fail_unless(TVALR(thr) == INT2FIX(31337));
}
END_TEST

START_TEST(test_vm_apply)
{
#if 0
  Inst **ctp, *code, *ofs, *base;
  value vcode, func, thr, vcode2, func2;

  arc_vmengine(&c, CNIL, 0);
  vcode = arc_mkvmcode(&c, 4);
  code = (Inst*)&VINDEX(vcode, 0);
  ctp = &code;
  gen_ldi(ctp, INT2FIX(31330));
  gen_add(ctp);
  gen_ret(ctp);
  func = arc_mkcode(&c, vcode, arc_mkstringc(&c, "test"), CNIL, 0);
  func = arc_mkclosure(&c, func, CNIL);

  vcode2 = arc_mkvmcode(&c, 10);
  base = code = (Inst*)&VINDEX(vcode2, 0);
  ctp = &code;
  gen_ldi(ctp, INT2FIX(0xf1e));
  gen_push(ctp);
  gen_cont(ctp, 0);
  ofs = *ctp - 1;
  gen_ldi(ctp, INT2FIX(7));
  gen_push(ctp);
  gen_ldi(ctp, func);
  gen_apply(ctp, 1);
  *((int *)ofs) = (*ctp - base);
  gen_hlt(ctp);

  func2 = arc_mkcode(&c, vcode2, arc_mkstringc(&c, "test"), CNIL, 0);
  thr = arc_mkthread(&c, func2, 2048, 0);
  arc_vmengine(&c, thr, 1000);
  fail_unless(TVALR(thr) == INT2FIX(31337));
  fail_unless(*TSP(thr) == INT2FIX(0xf1e));
#endif
}
END_TEST

START_TEST(test_vm_loadstore)
{
#if 0
  Inst **ctp, *code;
  value vcode, func, thr;
  value sym, str;

  str = arc_mkstringc(&c, "foo");
  sym = arc_intern(&c, str);

  arc_vmengine(&c, CNIL, 0);

  vcode = arc_mkvmcode(&c, 7);
  code = (Inst*)&VINDEX(vcode, 0);
  ctp = &code;
  func = arc_mkcode(&c, vcode, arc_mkstringc(&c, "test"), CNIL, 1);
  CODE_LITERAL(func, 0) = sym;
  gen_ldl(ctp, 0);
  gen_hlt(ctp);
  thr = arc_mkthread(&c, func, 2048, 0);
  arc_vmengine(&c, thr, 1000);
  fail_unless(TVALR(thr) == sym);

  code = (Inst*)&VINDEX(vcode, 0);
  ctp = &code;
  gen_ldi(ctp, INT2FIX(31337));
  gen_stg(ctp, 0);
  gen_ldg(ctp, 0);
  gen_hlt(ctp);
  thr = arc_mkthread(&c, func, 2048, 0);
  arc_vmengine(&c, thr, 1000);
  fail_unless(TVALR(thr) == INT2FIX(31337));
  fail_unless(arc_hash_lookup(&c, c.genv, sym) == INT2FIX(31337));
#endif
}
END_TEST

#if 0
static value test_fn(arc *c, value x)
{
  return(x + INT2FIX(31330) - 1);
}

#endif

START_TEST(test_vm_apply_cfunc)
{
#if 0
  Inst **ctp, *code, *ofs, *base;
  value func, thr, vcode2, func2;

  arc_vmengine(&c, CNIL, 0);
  func = arc_mkccode(&c, 1, test_fn);

  vcode2 = arc_mkvmcode(&c, 10);
  base = code = (Inst*)&VINDEX(vcode2, 0);
  ctp = &code;
  gen_ldi(ctp, INT2FIX(0xf1e));
  gen_push(ctp);
  gen_cont(ctp, 0);
  ofs = *ctp - 1;
  gen_ldi(ctp, INT2FIX(7));
  gen_push(ctp);
  gen_ldi(ctp, func);
  gen_apply(ctp, 1);
  *((int *)ofs) = (*ctp - base);
  gen_hlt(ctp);

  func2 = arc_mkcode(&c, vcode2, arc_mkstringc(&c, "test"), CNIL, 0);
  thr = arc_mkthread(&c, func2, 2048, 0);
  arc_vmengine(&c, thr, 1000);
  fail_unless(TVALR(thr) == INT2FIX(31337));
  fail_unless(*TSP(thr) == INT2FIX(0xf1e));
#endif
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

  tcase_add_test(tc_vm, test_vm_basic);
  tcase_add_test(tc_vm, test_vm_loadstore);
  tcase_add_test(tc_vm, test_vm_apply);
  tcase_add_test(tc_vm, test_vm_apply_cfunc);

  suite_add_tcase(s, tc_vm);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

