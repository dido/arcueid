/* 
  Copyright (C) 2013 Rafael R. Sevilla

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
  along with this program; if not, see <http://www.gnu.org/licenses/>.
*/
#include <check.h>
#include "../src/arcueid.h"
#include "../src/vmengine.h"
#include "../src/arith.h"
#include "../config.h"

arc cc;
arc *c;

#define CPUSH_(val) CPUSH(thr, val)

#define XCALL(fname, ...) do {			\
    TVALR(thr) = arc_mkaff(c, fname, CNIL);	\
    TARGC(thr) = NARGS(__VA_ARGS__);		\
    FOR_EACH(CPUSH_, __VA_ARGS__);		\
    __arc_thr_trampoline(c, thr);		\
  } while (0)

START_TEST(test_add_fixnum)
{
  int i;
  value v = INT2FIX(0);

  for (i=1; i<=100; i++)
    v = __arc_add2(c, v, INT2FIX(i));
  fail_unless(TYPE(v) == T_FIXNUM);
  fail_unless(FIX2INT(v) == 5050);  
}
END_TEST

START_TEST(test_add_limit)
{
  value maxfixnum, one, sum;

  maxfixnum = INT2FIX(FIXNUM_MAX);
  one = INT2FIX(1);
  sum = __arc_add2(c, maxfixnum, one);
#ifdef HAVE_GMP_H
  fail_unless(TYPE(sum) == T_BIGNUM);
  fail_unless(mpz_get_si(REPBNUM(sum)) == FIXNUM_MAX + 1);
#else
  fail_unless(TYPE(sum) == T_FLONUM);
  fail_unless(REPFLO(sum) - (FIXNUM_MAX + 1) < 1e-6);
#endif
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Arithmetic Operations");
  TCase *tc_arith = tcase_create("Arithmetic Operations");
  SRunner *sr;

  c = &cc;
  arc_init(c);

  tcase_add_test(tc_arith, test_add_fixnum);
  tcase_add_test(tc_arith, test_add_limit);

  suite_add_tcase(s, tc_arith);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

