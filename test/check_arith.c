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

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
#ifndef alloca
# define alloca __builtin_alloca
#endif
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# include <stddef.h>
void *alloca (size_t);
#endif

arc cc;
arc *c;

#define CPUSH_(val) CPUSH(thr, val)

#define XCALL(fname, ...) do {			\
    TVALR(thr) = arc_mkaff(c, fname, CNIL);	\
    TARGC(thr) = NARGS(__VA_ARGS__);		\
    FOR_EACH(CPUSH_, __VA_ARGS__);		\
    __arc_thr_trampoline(c, thr);		\
  } while (0)

/*================================= Additions involving fixnums */

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
  value maxfixnum, one, negone, sum;

  maxfixnum = INT2FIX(FIXNUM_MAX);
  one = INT2FIX(1);
  negone = INT2FIX(-1);
  sum = __arc_add2(c, maxfixnum, one);
#ifdef HAVE_GMP_H
  fail_unless(TYPE(sum) == T_BIGNUM);
  fail_unless(mpz_get_si(REPBNUM(sum)) == FIXNUM_MAX + 1);

  sum = __arc_add2(c, negone, sum);
  fail_unless(TYPE(sum) == T_FIXNUM);
  fail_unless(FIX2INT(sum) == FIXNUM_MAX);
#else
  fail_unless(TYPE(sum) == T_FLONUM);
  fail_unless(REPFLO(sum) - (FIXNUM_MAX + 1) < 1e-6);
#endif
  /* Negative side */
  maxfixnum = INT2FIX(-FIXNUM_MAX);
  sum = __arc_add2(c, maxfixnum, negone);

#ifdef HAVE_GMP_H
  fail_unless(TYPE(sum) == T_BIGNUM);
  fail_unless(mpz_get_si(REPBNUM(sum)) == -FIXNUM_MAX - 1);
#else
  fail_unless(TYPE(sum) == T_FLONUM);
  fail_unless(REPFLO(sum) - (-FIXNUM_MAX - 1) < 1e-6);
#endif
}
END_TEST

START_TEST(test_add_fixnum2flonum)
{
  value val1, val2, sum;

  val1 = INT2FIX(1);
  val2 = arc_mkflonum(c, 3.14159);

  sum = __arc_add2(c, val1, val2);
  fail_unless(TYPE(sum) == T_FLONUM);
  fail_unless(fabs(4.14159 - REPFLO(sum)) < 1e-6);

  val1 = INT2FIX(-1);
  sum = __arc_add2(c, sum, val1);
  fail_unless(TYPE(sum) == T_FLONUM);
  fail_unless(fabs(3.14159 - REPFLO(sum)) < 1e-6);
}
END_TEST

START_TEST(test_add_fixnum2complex)
{
  value val1, val2, sum;

  val1 = INT2FIX(1);
  val2 = arc_mkcomplex(c, 1.1 + I*2.2);

  sum = __arc_add2(c, val1, val2);
  fail_unless(TYPE(sum) == T_COMPLEX);
  fail_unless(fabs(2.1 - creal(REPCPX(sum))) < 1e-6);
  fail_unless(fabs(2.2 - cimag(REPCPX(sum))) < 1e-6);

  val1 = INT2FIX(-1);
  sum = __arc_add2(c, sum, val1);
  fail_unless(TYPE(sum) == T_COMPLEX);
  fail_unless(fabs(1.1 - creal(REPCPX(sum))) < 1e-6);
  fail_unless(fabs(2.2 - cimag(REPCPX(sum))) < 1e-6);
}
END_TEST

#ifdef HAVE_GMP_H

START_TEST(test_add_fixnum2bignum)
{
  value bn, sum;
  char *str;

  bn = arc_mkbignuml(c, 0);
  mpz_set_str(REPBNUM(bn), "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", 10);
  sum = __arc_add2(c, bn, INT2FIX(1));
  fail_unless(TYPE(sum) == T_BIGNUM);
  str = alloca(mpz_sizeinbase(REPBNUM(bn), 10) + 2);
  mpz_get_str(str, 10, REPBNUM(sum));
  fail_unless(strcmp(str, "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001") == 0);
}
END_TEST

START_TEST(test_add_fixnum2rational)
{
  value v1, v2, sum;

  v1 = arc_mkrationall(c, 1, 2);
  v2 = INT2FIX(1);
  sum = __arc_add2(c, v1, v2);
  fail_unless(TYPE(sum) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REPRAT(sum), 3, 2) == 0);

  sum = __arc_add2(c, v2, v1);
  fail_unless(TYPE(sum) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REPRAT(sum), 3, 2) == 0);
}
END_TEST

#endif

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
  tcase_add_test(tc_arith, test_add_fixnum2flonum);
  tcase_add_test(tc_arith, test_add_fixnum2complex);
#ifdef HAVE_GMP_H
  tcase_add_test(tc_arith, test_add_fixnum2bignum);
  tcase_add_test(tc_arith, test_add_fixnum2rational);
#endif

  suite_add_tcase(s, tc_arith);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

