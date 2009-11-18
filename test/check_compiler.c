/* 
  Copyright (C) 2009 Rafael R. Sevilla

  This file is part of CArc

  CArc is free software; you can redistribute it and/or modify it
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
#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include "../src/carc.h"
#include "../src/vmengine.h"

extern void gen_ldi(Inst **ctp, value i2);
extern void gen_cont(Inst **ctp, value icofs);
extern void gen_apply(Inst **ctp, value iargc);
extern void gen_hlt(Inst **ctp);

carc c;

static value stub_call(value func)
{
  Inst **ctp, *code, *ofs, *base;
  value vcode, stub, thr;

  vcode = carc_mkvmcode(&c, 3);
  base = code = (Inst *)&VINDEX(vcode, 0);
  ctp = &code;
  gen_cont(ctp, 0);
  ofs = *ctp - 1;
  gen_ldi(ctp, func);
  gen_apply(ctp, 0);
  gen_hlt(ctp);

  stub = carc_mkcode(&c, vcode, CNIL, 0);
  thr = carc_mkthread(&c, stub, 2048, 0);
  carc_vmengine(&c, thr, 1000);
  return(thr);
}

START_TEST(test_literal)
{
  value expr = INT2FIX(31337);
  value code, thr;

  code = carc_mkclosure(&c, carc_compile(&c, expr, CNIL), CNIL);
  thr = stub_call(code);
  fail_unless(TVALR(thr) == INT2FIX(31337));
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Compiler");
  TCase *tc_compiler = tcase_create("Compiler");
  SRunner *sr;

  carc_set_memmgr(&c);
  carc_init_reader(&c);
  carc_vmengine(&c, CNIL, 0);

  tcase_add_test(tc_compiler, test_literal);

  suite_add_tcase(s, tc_compiler);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
