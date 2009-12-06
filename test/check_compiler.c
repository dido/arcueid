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
#include <math.h>
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
  ofs = *ctp;
  gen_cont(ctp, 0);
  gen_ldi(ctp, func);
  gen_apply(ctp, 0);
  *(ofs+1) = (Inst)(*ctp - ofs);
  gen_hlt(ctp);

  stub = carc_mkcode(&c, vcode, CNIL, CNIL, 0);
  thr = carc_mkthread(&c, stub, 2048, 0);
  carc_vmengine(&c, thr, 1000);
  return(thr);
}

START_TEST(test_literal)
{
  value expr;
  value code, thr;

  expr = INT2FIX(31337);
  code = carc_compile(&c, expr, CNIL, CNIL);
  thr = stub_call(code);
  fail_unless(TVALR(thr) == INT2FIX(31337));

  expr = carc_mkflonum(&c, 3.14159);
  code = carc_compile(&c, expr, CNIL, CNIL);
  thr = stub_call(code);
  fail_unless(TYPE(TVALR(thr)) == T_FLONUM);
  fail_unless(fabs(REP(TVALR(thr))._flonum - 3.14159) < 1e-6);
}
END_TEST

START_TEST(test_if)
{
  value expr, ifkwd;
  value code, thr;

  ifkwd = carc_intern(&c, carc_mkstringc(&c, "if"));

  /* (if t 1 2) */
  expr = cons(&c, ifkwd,
	      cons(&c, CTRUE, cons(&c, INT2FIX(1),
				   cons(&c, INT2FIX(2), CNIL))));
  code = carc_compile(&c, expr, CNIL, CNIL);
  thr = stub_call(code);
  fail_unless(TVALR(thr) == INT2FIX(1));

  /* (if nil 1 2) */
  expr = cons(&c, ifkwd,
	      cons(&c, CNIL, cons(&c, INT2FIX(1),
				    cons(&c, INT2FIX(2), CNIL))));
  code = carc_compile(&c, expr, CNIL, CNIL);
  thr = stub_call(code);
  fail_unless(TVALR(thr) == INT2FIX(2));

  /* Test the cases where there aren't enough elements in the statement
     (if) */
  expr = cons(&c, ifkwd, CNIL);
  code = carc_compile(&c, expr, CNIL, CNIL);
  thr = stub_call(code);
  fail_unless(TVALR(thr) == CNIL);

  /* (if t) */
  expr = cons(&c, ifkwd, cons(&c, CTRUE, CNIL));
  code = carc_compile(&c, expr, CNIL, CNIL);
  thr = stub_call(code);
  fail_unless(TVALR(thr) == CNIL);

  /* (if t 1) */
  expr = cons(&c, ifkwd, cons(&c, CTRUE, cons(&c, INT2FIX(1), CNIL)));
  code = carc_compile(&c, expr, CNIL, CNIL);
  thr = stub_call(code);
  fail_unless(TVALR(thr) == INT2FIX(1));

  /* Test the cases where there are more than three elements (nested if)
     (if nil 1 nil 2 3) */
  expr = cons(&c, ifkwd,
	      cons(&c, CNIL,
		   cons(&c, INT2FIX(1),
			cons(&c, CNIL, cons(&c, INT2FIX(2),
					    cons(&c, INT2FIX(3), CNIL))))));
  code = carc_compile(&c, expr, CNIL, CNIL);
  thr = stub_call(code);
  fail_unless(TVALR(thr) == INT2FIX(3));

  /* (if nil 1 t 2 3) */
  expr = cons(&c, ifkwd,
	      cons(&c, CNIL,
		   cons(&c, INT2FIX(1),
			cons(&c, CTRUE, cons(&c, INT2FIX(2),
					     cons(&c, INT2FIX(3), CNIL))))));
  code = carc_compile(&c, expr, CNIL, CNIL);
  thr = stub_call(code);
  fail_unless(TVALR(thr) == INT2FIX(2));

  /* (if t 1 nil 2 3) */
  expr = cons(&c, ifkwd,
	      cons(&c, CTRUE,
		   cons(&c, INT2FIX(1),
			cons(&c, CNIL, cons(&c, INT2FIX(2),
					    cons(&c, INT2FIX(3), CNIL))))));
  code = carc_compile(&c, expr, CNIL, CNIL);
  thr = stub_call(code);
  fail_unless(TVALR(thr) == INT2FIX(1));

  /* Further cases where the second branch has only a then portion:
     (if nil 1 t 2) */
  expr = cons(&c, ifkwd,
	      cons(&c, CNIL,
		   cons(&c, INT2FIX(1),
			cons(&c, CTRUE, cons(&c, INT2FIX(2), CNIL)))));
  code = carc_compile(&c, expr, CNIL, CNIL);
  thr = stub_call(code);
  fail_unless(TVALR(thr) == INT2FIX(2));

  /* (if nil 1 nil 2) */
  expr = cons(&c, ifkwd,
	      cons(&c, CNIL,
		   cons(&c, INT2FIX(1),
			cons(&c, CNIL, cons(&c, INT2FIX(2), CNIL)))));
  code = carc_compile(&c, expr, CNIL, CNIL);
  thr = stub_call(code);
  fail_unless(TVALR(thr) == CNIL);
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
  carc_init_compiler(&c);
  carc_vmengine(&c, CNIL, 0);

  tcase_add_test(tc_compiler, test_literal);
  tcase_add_test(tc_compiler, test_if);

  suite_add_tcase(s, tc_compiler);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
