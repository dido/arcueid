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
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include <math.h>
#include <stdio.h>
#include "../src/carc.h"
#include "../src/alloc.h"
#include "../config.h"
#include "../src/vmengine.h"

void gen_nop(Inst **ctp);
void gen_push(Inst **ctp);
void gen_pop(Inst **ctp);
void gen_ldl(Inst **ctp, value i2);
void gen_ldi(Inst **ctp, value i2);
void gen_ldg(Inst **ctp, value i2);
void gen_stg(Inst **ctp, value i2);
void gen_lde(Inst **ctp, value ienv, value iindx);
void gen_ste(Inst **ctp, value ienv, value iindx);
void gen_cont(Inst **ctp, value icofs);
void gen_env(Inst **ctp, value ienvsize);
void gen_apply(Inst **ctp, value iargc);
void gen_ret(Inst **ctp);
void gen_jmp(Inst **ctp, Inst * target);
void gen_jt(Inst **ctp, Inst * target);
void gen_jf(Inst **ctp, Inst * target);
void gen_true(Inst **ctp);
void gen_nil(Inst **ctp);
void gen_hlt(Inst **ctp);
void gen_add(Inst **ctp);
void gen_sub(Inst **ctp);
void gen_mul(Inst **ctp);
void gen_div(Inst **ctp);
void gen_cons(Inst **ctp);
void gen_car(Inst **ctp);
void gen_cdr(Inst **ctp);
void gen_scar(Inst **ctp);
void gen_scdr(Inst **ctp);
void gen_is(Inst **ctp);

carc c;

START_TEST(test_vm_basic)
{
  Inst **ctp, *code, *ofs;
  value vcode, func, thr;

  carc_vmengine(&c, CNIL, 0);
  vcode = carc_mkvmcode(&c, 14);
  code = (Inst*)&VINDEX(vcode, 0);
  ctp = &code;
  gen_ldi(ctp, INT2FIX(31330));
  gen_push(ctp);
  ofs = *ctp;
  gen_ldi(ctp, INT2FIX(1));
  gen_add(ctp);
  gen_push(ctp);
  gen_push(ctp);
  gen_ldi(ctp, INT2FIX(31337));
  gen_is(ctp);
  gen_jf(ctp, ofs);
  gen_pop(ctp);
  gen_hlt(ctp);
  func = carc_mkcode(&c, vcode, carc_mkstringc(&c, "test"), CNIL, 0);
  thr = carc_mkthread(&c, func, 2048, 0);
  carc_vmengine(&c, thr, 1000);
  fail_unless(TVALR(thr) == INT2FIX(31337));
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Virtual Machine");
  TCase *tc_vm = tcase_create("Virtual Machine");
  SRunner *sr;

  carc_set_memmgr(&c);

  tcase_add_test(tc_vm, test_vm_basic);

  suite_add_tcase(s, tc_vm);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

