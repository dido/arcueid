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
#include "../src/arcueid.h"
#include "../src/builtin.h"
#include "../src/vmengine.h"
#include "../src/symbols.h"
#include "../config.h"

arc *c, cc;
extern unsigned long long gcepochs;

value TEST(const char *testexpr)
{
  value str, sexpr, fp, cctx, code;

  str = arc_mkstringc(c, testexpr);
  fp = arc_instring(c, str, CNIL);
  sexpr = arc_read(c, fp, CNIL);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  return(arc_macapply(c, code, CNIL));
}

START_TEST(test_nested_envs)
{
  value ret;
  unsigned long long oldepoch;
  int i;

  ret=TEST("((fn (foo) (assign test (fn (x) (foo x))) (assign test2 (fn (x) (+ 1 (foo x))))) (fn (x) x))");
  fail_unless(TYPE(ret) == T_CLOS);

  for (i=0; i<3; i++) {
    oldepoch = gcepochs;
    while (gcepochs == oldepoch) {
      c->rungc(c);
    }
  }
  ret=TEST("(test 1)");
  fail_unless(ret == INT2FIX(1));
}
END_TEST

void error_handler(struct arc *c, value exc)
{
  arc_print_string(c, arc_prettyprint(c, exc));
  printf("\n");
  fail("exception received");
}

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Garbage Collector");
  TCase *tc_gc = tcase_create("Garbage Collector");
  SRunner *sr;

  c = &cc;
  arc_init(c);

  tcase_add_test(tc_gc, test_nested_envs);

  suite_add_tcase(s, tc_gc);
  sr = srunner_create(s);
  /* srunner_set_fork_status(sr, CK_NOFORK); */
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
