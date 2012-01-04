/* 
  Copyright (C) 2012 Rafael R. Sevilla

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
#include <unistd.h>
#include <stdarg.h>
#include <setjmp.h>
#include "../src/arcueid.h"
#include "../src/vmengine.h"
#include "../src/symbols.h"
#include "../config.h"

arc *c, cc;
jmp_buf err_jmp_buf;
value current_exception;

#define TEST(testexpr) {				\
  value str, sexpr, fp, cctx, code;			\
  str = arc_mkstringc(c, testexpr);			\
  fp = arc_instring(c, str);				\
  sexpr = arc_read(c, fp);				\
  cctx = arc_mkcctx(c, INT2FIX(1), 0);			\
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);		\
  code = arc_cctx2code(c, cctx);			\
  ret = arc_macapply(c, code, CNIL); }

static void error_handler(struct arc *c, value err)
{
  current_exception = err;
  longjmp(err_jmp_buf, 1);
}

START_TEST(test_divzero_fixnum)
{
  value ret;

  if (setjmp(err_jmp_buf) == 1) {
    fail_unless(1);		/* success */
    return;
  }
  TEST("(/ 1 1)");
  fail("division by zero exception not raised");
  fail_unless(ret);
}
END_TEST


int main(void)
{
  int number_failed;
  Suite *s = suite_create("Error handling");
  TCase *tc_err = tcase_create("Error handling");
  SRunner *sr;
  char *loadstr = "arc.arc";
  value initload, sexpr, cctx, code;

  c = &cc;
  arc_init(c);
  c->signal_error = error_handler;

  initload = arc_infile(c, arc_mkstringc(c, loadstr));
  while ((sexpr = arc_read(c, initload)) != CNIL) {
    cctx = arc_mkcctx(c, INT2FIX(1), 0);
    arc_compile(c, sexpr, cctx, CNIL, CTRUE);
    code = arc_cctx2code(c, cctx);
    arc_macapply(c, code, CNIL);
    c->rungc(c);
  }
  arc_close(c, initload);

  tcase_add_test(tc_err, test_divzero_fixnum);

  suite_add_tcase(s, tc_err);
  sr = srunner_create(s);
  /* srunner_set_fork_status(sr, CK_NOFORK); */
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
