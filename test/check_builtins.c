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

START_TEST(test_builtin_exact)
{
  fail_unless(test_builtin("exact", 1, INT2FIX(31337)) == CTRUE);
  fail_unless(test_builtin("exact", 1, arc_mkflonum(&c, 3.14159)) == CNIL);
}
END_TEST

START_TEST(test_builtin_spaceship)
{
  fail_unless(test_builtin("<=>", 2, INT2FIX(31338), INT2FIX(31337)) == INT2FIX(-1));
  fail_unless(test_builtin("<=>", 2, INT2FIX(31337), INT2FIX(31338)) == INT2FIX(1));
  fail_unless(test_builtin("<=>", 2, INT2FIX(31337), INT2FIX(31337)) == INT2FIX(0));
}
END_TEST

START_TEST(test_builtin_expt)
{
  value val;
  mpz_t expected;

  fail_unless(test_builtin("expt", 2, INT2FIX(24), INT2FIX(2)) == INT2FIX(16777216));
  val = test_builtin("expt", 2, INT2FIX(24), arc_mkflonum(&c, 2));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(16777216.0 - REP(val)._flonum) < 1e6);

  val = test_builtin("expt", 2, arc_mkflonum(&c, 24.0), INT2FIX(2));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(16777216.0 - REP(val)._flonum) < 1e6);

  val = test_builtin("expt", 2, arc_mkflonum(&c, 24.0), arc_mkflonum(&c, 2.0));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(16777216.0 - REP(val)._flonum) < 1e6);

  val = test_builtin("expt", 2, arc_mkflonum(&c, 0.5), arc_mkflonum(&c, -1));
  fail_unless(TYPE(val) == T_COMPLEX);
  fail_unless(fabs(REP(val)._complex.re) < 1e6);
  fail_unless(fabs(1.0 - REP(val)._complex.im) < 1e6);

  val = test_builtin("expt", 2, arc_mkcomplex(&c, 2.0, 2.0),
		     arc_mkcomplex(&c, 1.0, 1.0));
  fail_unless(TYPE(val) == T_COMPLEX);
  fail_unless(fabs(-0.26565399849241 - REP(val)._complex.re) < 1e6);
  fail_unless(fabs(0.319818113856136 - REP(val)._complex.im) < 1e6);

#ifdef HAVE_GMP_H
  val = test_builtin("expt", 2, INT2FIX(1024), INT2FIX(2));
  fail_unless(TYPE(val) == T_BIGNUM);
  mpz_init(expected);
  mpz_set_str(expected, "179769313486231590772930519078902473361797697894230657273430081157732675805500963132708477322407536021120113879871393357658789768814416622492847430639474124377767893424865485276302219601246094119453082952085005768838150682342462881473913110540827237163350510684586298239947245938479716304835356329624224137216", 10);
  fail_unless(mpz_cmp(expected, REP(val)._bignum) == 0);

  val = test_builtin("expt", 2, INT2FIX(2), val);
  fail_unless(TYPE(val) == T_BIGNUM);
  mpz_set_str(expected, "32317006071311007300714876688669951960444102669715484032130345427524655138867890893197201411522913463688717960921898019494119559150490921095088152386448283120630877367300996091750197750389652106796057638384067568276792218642619756161838094338476170470581645852036305042887575891541065808607552399123930385521914333389668342420684974786564569494856176035326322058077805659331026192708460314150258592864177116725943603718461857357598351152301645904403697613233287231227125684710820209725157101726931323469678542580656697935045997268352998638215525166389437335543602135433229604645318478604952148193555853611059596230656", 10);
  fail_unless(mpz_cmp(expected, REP(val)._bignum) == 0);

  mpz_clear(expected);
#endif
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
  tcase_add_test(tc_bif, test_builtin_exact);
  tcase_add_test(tc_bif, test_builtin_spaceship);

  tcase_add_test(tc_bif, test_builtin_expt);

  suite_add_tcase(s, tc_bif);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

