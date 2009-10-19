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

extern void gen_nop(Inst **ctp);
extern void gen_drop(Inst **ctp);
extern void gen_dup(Inst **ctp);
extern void gen_ldl(Inst **ctp, value i2);
extern void gen_ldi(Inst **ctp, value i2);
extern void gen_jmp(Inst **ctp, value i);
extern void gen_jt(Inst **ctp, value i);
extern void gen_jf(Inst **ctp, value i);
extern void gen_add(Inst **ctp);
extern void gen_is(Inst **ctp);
extern void gen_hlt(Inst **ctp);

carc c;

START_TEST(test_vm)
{
  value thr;
  value fun;
  Inst **ctp, *code;

  /* Make a function manually */

  carc_vmengine(&c, CNIL, 0);
  fun = carc_mkvector(&c, 2);
  VINDEX(fun, 0) = carc_mkvector(&c, 12);
  code = (Inst *)&VINDEX(VINDEX(fun, 0), 0);
  ctp = &code;
  gen_ldi(ctp, INT2FIX(31330));
  gen_ldi(ctp, INT2FIX(1));
  gen_add(ctp);
  gen_dup(ctp);
  gen_ldi(ctp, INT2FIX(31337));
  gen_is(ctp);
  gen_jf(ctp, -9);
  gen_hlt(ctp);
  VINDEX(fun, 1) = carc_mkstringc(&c, "test");
  thr = carc_mkthread(&c, fun, 64, 0);
  carc_vmengine(&c, thr, 128);
  fail_unless(*TSP(thr) == INT2FIX(31337));
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Virtual Machine");
  TCase *tc_vm = tcase_create("Virtual Machine");
  SRunner *sr;

  carc_set_memmgr(&c);

  tcase_add_test(tc_vm, test_vm);

  suite_add_tcase(s, tc_vm);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

