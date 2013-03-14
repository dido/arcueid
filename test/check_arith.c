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
  value maxfixnum, one, negone, sum;

  for (i=1; i<=100; i++)
    v = __arc_add2(c, v, INT2FIX(i));
  fail_unless(TYPE(v) == T_FIXNUM);
  fail_unless(FIX2INT(v) == 5050);  

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

#ifdef HAVE_GMP_H

/*================================= Additions involving bignums */

START_TEST(test_add_bignum)
{
  value val1, val2, sum;
  mpz_t expected;

  val1 = arc_mkbignuml(c, 0);
  mpz_set_str(REPBNUM(val1), "100000000000000000000000000000", 10);
  val2 = arc_mkbignuml(c, 0);
  mpz_set_str(REPBNUM(val2), "200000000000000000000000000000", 10);
  sum = __arc_add2(c, val1, val2);
  fail_unless(TYPE(sum) == T_BIGNUM);
  mpz_init(expected);
  mpz_set_str(expected, "300000000000000000000000000000", 10);
  fail_unless(mpz_cmp(expected, REPBNUM(sum)) == 0);
  mpz_clear(expected);
}
END_TEST

START_TEST(test_add_bignum2rational)
{
  value v1, v2, sum;
  mpq_t expected;

  v1 = arc_mkrationall(c, 1, 2);
  v2 = arc_mkbignuml(c, 0);
  mpz_set_str(REPBNUM(v2), "100000000000000000000000000000", 10);
  sum = __arc_add2(c, v1, v2);
  fail_unless(TYPE(sum) == T_RATIONAL);
  mpq_init(expected);
  mpq_set_str(expected, "200000000000000000000000000001/2", 10);
  fail_unless(mpq_cmp(expected, REPRAT(sum)) == 0);
  mpq_clear(expected);

  v1 = arc_mkbignuml(c, 0);
  mpz_set_str(REPBNUM(v1), "100000000000000000000000000000", 10);
  v2 = arc_mkrationall(c, 1, 2);
  sum = __arc_add2(c, v1, v2);
  fail_unless(TYPE(sum) == T_RATIONAL);
  mpq_init(expected);
  mpq_set_str(expected, "200000000000000000000000000001/2", 10);
  fail_unless(mpq_cmp(expected, REPRAT(sum)) == 0);
  mpq_clear(expected);
}
END_TEST

START_TEST(test_add_bignum2flonum)
{
  value v1, v2, sum;

  v1 = arc_mkflonum(c, 0.0);
  v2 = arc_mkbignuml(c, 0);
  mpz_set_str(REPBNUM(v2), "10000000000000000000000", 10);
  sum = __arc_add2(c, v1, v2);
  fail_unless(TYPE(sum) == T_FLONUM);
  fail_unless(fabs(REPFLO(sum) - 1e22) < 1e-6);

  v1 = arc_mkbignuml(c, 0);
  mpz_set_str(REPBNUM(v1), "10000000000000000000000", 10);
  v2 = arc_mkflonum(c, 0.0);
  sum = __arc_add2(c, v1, v2);
  fail_unless(TYPE(sum) == T_FLONUM);
  fail_unless(fabs(REPFLO(sum) - 1e22) < 1e-6);
}
END_TEST

START_TEST(test_add_bignum2complex)
{
  value v1, v2, sum;

  v1 = arc_mkcomplex(c, 0.0 + I*1.1);
  v2 = arc_mkbignuml(c, 0);
  mpz_set_str(REPBNUM(v2), "10000000000000000000000", 10);
  sum = __arc_add2(c, v1, v2);
  fail_unless(TYPE(sum) == T_COMPLEX);
  fail_unless(fabs(creal(REPCPX(sum)) - 1e22) < 1e-6);
  fail_unless(fabs(cimag(REPCPX(sum)) - 1.1) < 1e-6);

  v1 = arc_mkbignuml(c, 0);
  mpz_set_str(REPBNUM(v1), "10000000000000000000000", 10);
  v2 = arc_mkcomplex(c, 0.0 + I*1.1);
  sum = __arc_add2(c, v1, v2);
  fail_unless(TYPE(sum) == T_COMPLEX);
  fail_unless(fabs(creal(REPCPX(sum)) - 1e22) < 1e-6);
  fail_unless(fabs(cimag(REPCPX(sum)) - 1.1) < 1e-6);
}
END_TEST

START_TEST(test_add_rational)
{
  value val1, val2, sum;
  mpz_t expected;

  val1 = arc_mkrationall(c, 1, 2);
  val2 = arc_mkrationall(c, 1, 4);
  sum = __arc_add2(c, val1, val2);
  fail_unless(TYPE(sum) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REPRAT(sum), 3, 4) == 0);

  /* Conversions of rationals to integer types when appropriate */
  val1 = sum;
  sum = __arc_add2(c, val1, val2);
  fail_unless(TYPE(sum) == T_FIXNUM);
  fail_unless(FIX2INT(sum) == 1);

  val1 = arc_mkrationall(c, 0, 1);
  mpq_set_str(REPRAT(val1), "1606938044258990275541962092341162602522202993782792835301375/4", 10);
  val2 = arc_mkrationall(c, 0, 1);
  mpq_set_str(REPRAT(val2), "1/4", 10);
  sum = __arc_add2(c, val1, val2);
  fail_unless(TYPE(sum) == T_BIGNUM);
  mpz_init(expected);
  mpz_set_str(expected, "401734511064747568885490523085290650630550748445698208825344", 10);
  fail_unless(mpz_cmp(expected, REPBNUM(sum)) == 0);
  mpz_clear(expected);
}
END_TEST

START_TEST(test_add_rational2flonum)
{
  value val1, val2, sum;

  val1 = arc_mkflonum(c, 0.5);
  val2 = arc_mkrationall(c, 1, 2);
  sum = __arc_add2(c, val1, val2);
  fail_unless(TYPE(sum) == T_FLONUM);
  fail_unless(fabs(1.0 - REPFLO(sum)) < 1e-6);

  val1 = arc_mkrationall(c, 1, 2);
  val2 = arc_mkflonum(c, 0.5);
  sum = __arc_add2(c, val1, val2);
  fail_unless(TYPE(sum) == T_FLONUM);
  fail_unless(fabs(1.0 - REPFLO(sum)) < 1e-6);
}
END_TEST

START_TEST(test_add_rational2complex)
{
  value val1, val2, sum;

  val1 = arc_mkcomplex(c, 0.5 + I*0.5);
  val2 = arc_mkrationall(c, 1, 2);
  sum = __arc_add2(c, val1, val2);
  fail_unless(TYPE(sum) == T_COMPLEX);
  fail_unless(fabs(1.0 - creal(REPCPX(sum))) < 1e-6);
  fail_unless(fabs(0.5 - cimag(REPCPX(sum))) < 1e-6);

  val1 = arc_mkrationall(c, 1, 2);
  val2 = arc_mkcomplex(c, 0.5 + I*0.5);
  sum = __arc_add2(c, val1, val2);
  fail_unless(TYPE(sum) == T_COMPLEX);
  fail_unless(fabs(1.0 - creal(REPCPX(sum))) < 1e-6);
  fail_unless(fabs(0.5 - cimag(REPCPX(sum))) < 1e-6);
}
END_TEST

#endif

START_TEST(test_add_flonum)
{
  value val1, val2, sum;

  val1 = arc_mkflonum(c, 2.71828);
  val2 = arc_mkflonum(c, 3.14159);

  sum = __arc_add2(c, val1, val2);
  fail_unless(TYPE(sum) == T_FLONUM);
  fail_unless(fabs(5.85987 - REPFLO(sum)) < 1e-6);
}
END_TEST

START_TEST(test_add_flonum2complex)
{
  value val1, val2, sum;

  val1 = arc_mkflonum(c, 0.5);
  val2 = arc_mkcomplex(c, 3 - I*4);
  sum = __arc_add2(c, val1, val2);
  fail_unless(TYPE(sum) == T_COMPLEX);
  fail_unless(fabs(3.5 - creal(REPCPX(sum))) < 1e-6);
  fail_unless(fabs(-4 - cimag(REPCPX(sum))) < 1e-6);

  val1 = arc_mkcomplex(c, 3 - I*4);
  val2 = arc_mkflonum(c, 0.5);
  sum = __arc_add2(c, val1, val2);
  fail_unless(TYPE(sum) == T_COMPLEX);
  fail_unless(fabs(3.5 - creal(REPCPX(sum))) < 1e-6);
  fail_unless(fabs(-4 - cimag(REPCPX(sum))) < 1e-6);
}
END_TEST

START_TEST(test_add_complex)
{
  value val1, val2, sum;

  val1 = arc_mkcomplex(c, 1 - I*2);
  val2 = arc_mkcomplex(c, 3 - I*4);

  sum = __arc_add2(c, val1, val2);
  fail_unless(TYPE(sum) == T_COMPLEX);
  fail_unless(fabs(4 - creal(REPCPX(sum))) < 1e-6);
  fail_unless(fabs(-6 - cimag(REPCPX(sum))) < 1e-6);
}
END_TEST

START_TEST(test_add_cons)
{
  value val1, val2, sum;

  val1 = cons(c, INT2FIX(1), cons(c, INT2FIX(2), CNIL));
  val2 = cons(c, INT2FIX(3), cons(c, INT2FIX(4), CNIL));

  sum = __arc_add2(c, val1, CNIL);
  fail_unless(TYPE(sum) == T_CONS);
  fail_unless(car(sum) == INT2FIX(1));
  fail_unless(cadr(sum) == INT2FIX(2));
  fail_unless(NIL_P(cddr(sum)));

  sum = __arc_add2(c, CNIL, val2);
  fail_unless(TYPE(sum) == T_CONS);
  fail_unless(car(sum) == INT2FIX(3));
  fail_unless(cadr(sum) == INT2FIX(4));
  fail_unless(NIL_P(cddr(sum)));

  sum = __arc_add2(c, val1, val2);
  fail_unless(TYPE(sum) == T_CONS);
  fail_unless(car(sum) == INT2FIX(1));
  fail_unless(cadr(sum) == INT2FIX(2));
  fail_unless(car(cddr(sum)) == INT2FIX(3));
  fail_unless(cadr(cddr(sum)) == INT2FIX(4));
  fail_unless(NIL_P(cddr(cddr(sum))));
}
END_TEST

START_TEST(test_add_string)
{
  value val1, val2, sum;

  val1 = arc_mkstringc(c, "foo");
  val2 = arc_mkstringc(c, "bar");

  sum = __arc_add2(c, val1, CNIL);
  fail_unless(TYPE(sum) == T_STRING);
  fail_unless(arc_strcmp(c, sum, val1) == 0);

  sum = __arc_add2(c, CNIL, val2);
  fail_unless(TYPE(sum) == T_STRING);
  fail_unless(arc_strcmp(c, sum, val2) == 0);

  sum = __arc_add2(c, val1, val2);
  fail_unless(TYPE(sum) == T_STRING);
  fail_unless(arc_strcmp(c, sum, arc_mkstringc(c, "foobar")) == 0);
}
END_TEST

START_TEST(test_add_string2char)
{
  value ch1, ch2, str, sum;

  ch1 = arc_mkchar(c, 0x9060);
  ch2 = arc_mkchar(c, 0x91ce);
  sum = __arc_add2(c, ch1, CNIL);
  fail_unless(TYPE(sum) == T_STRING);
  fail_unless(arc_strcmp(c, sum, arc_mkstringc(c, "遠")) == 0);

  sum = __arc_add2(c, CNIL, ch2);
  fail_unless(TYPE(sum) == T_STRING);
  fail_unless(arc_strcmp(c, sum, arc_mkstringc(c, "野")) == 0);

  sum = __arc_add2(c, ch1, ch2);
  fail_unless(TYPE(sum) == T_STRING);
  fail_unless(arc_strcmp(c, sum, arc_mkstringc(c, "遠野")) == 0);

  str = arc_mkstringc(c, "遠");
  sum = __arc_add2(c, str, ch2);
  fail_unless(TYPE(sum) == T_STRING);
  fail_unless(arc_strcmp(c, sum, arc_mkstringc(c, "遠野")) == 0);

  str = arc_mkstringc(c, "野");
  sum = __arc_add2(c, ch1, str);
  fail_unless(TYPE(sum) == T_STRING);
  fail_unless(arc_strcmp(c, sum, arc_mkstringc(c, "遠野")) == 0);
}
END_TEST

START_TEST(test_mul_fixnum)
{
  value prod;
#ifdef HAVE_GMP_H
  mpz_t expected;
#endif

  prod = __arc_mul2(c, INT2FIX(8), INT2FIX(21));
  fail_unless(TYPE(prod) == T_FIXNUM);
  fail_unless(FIX2INT(prod) == 168);

  prod = __arc_mul2(c, INT2FIX(-8), INT2FIX(-21));
  fail_unless(TYPE(prod) == T_FIXNUM);
  fail_unless(FIX2INT(prod) == 168);

  prod = __arc_mul2(c, INT2FIX(-8), INT2FIX(21));
  fail_unless(TYPE(prod) == T_FIXNUM);
  fail_unless(FIX2INT(prod) == -168);

  prod = __arc_mul2(c, INT2FIX(8), INT2FIX(-21));
  fail_unless(TYPE(prod) == T_FIXNUM);
  fail_unless(FIX2INT(prod) == -168);

#ifdef HAVE_GMP_H
  prod = __arc_mul2(c, INT2FIX(2), INT2FIX(FIXNUM_MAX));
  fail_unless(TYPE(prod) == T_BIGNUM);
  mpz_init(expected);
  mpz_set_si(expected, 2*FIXNUM_MAX);
  fail_unless(mpz_cmp(expected, REPBNUM(prod)) == 0);

  prod = __arc_mul2(c, INT2FIX(-2), INT2FIX(-FIXNUM_MAX));
  fail_unless(TYPE(prod) == T_BIGNUM);
  mpz_set_si(expected, 2*FIXNUM_MAX);
  fail_unless(mpz_cmp(expected, REPBNUM(prod)) == 0);

  prod = __arc_mul2(c, INT2FIX(-2), INT2FIX(FIXNUM_MAX));
  fail_unless(TYPE(prod) == T_BIGNUM);
  mpz_set_si(expected, -2*FIXNUM_MAX);
  fail_unless(mpz_cmp(expected, REPBNUM(prod)) == 0);

  prod = __arc_mul2(c, INT2FIX(2), INT2FIX(-FIXNUM_MAX));
  fail_unless(TYPE(prod) == T_BIGNUM);
  mpz_set_si(expected, -2*FIXNUM_MAX);
  fail_unless(mpz_cmp(expected, REPBNUM(prod)) == 0);
  mpz_clear(expected);
#else

  prod = __arc_mul2(c, INT2FIX(2), INT2FIX(FIXNUM_MAX));
  fail_unless(TYPE(prod) == T_FLONUM);
  fail_unless(REPFLO(prod) - 2*FIXNUM_MAX < 1e-6);

  prod = __arc_mul2(c, INT2FIX(-2), INT2FIX(-FIXNUM_MAX));
  fail_unless(TYPE(prod) == T_FLONUM);
  fail_unless(REPFLO(prod) - 2*FIXNUM_MAX < 1e-6);

  prod = __arc_mul2(c, INT2FIX(-2), INT2FIX(FIXNUM_MAX));
  fail_unless(TYPE(prod) == T_FLONUM);
  fail_unless(REPFLO(prod) - -2*FIXNUM_MAX < 1e-6);

  prod = __arc_mul2(c, INT2FIX(2), INT2FIX(-FIXNUM_MAX));
  fail_unless(TYPE(prod) == T_FLONUM);
  fail_unless(REPFLO(prod) - -2*FIXNUM_MAX < 1e-6);
#endif

}
END_TEST

#ifdef HAVE_GMP_H

START_TEST(test_mul_fixnum2bignum)
{
  value factorial;
  mpz_t expected;
  int i;

  factorial = INT2FIX(1);
  for (i=1; i<=100; i++)
    factorial = __arc_mul2(c, INT2FIX(i), factorial);

  mpz_init(expected);
  mpz_set_str(expected, "93326215443944152681699238856266700490715968264381621468592963895217599993229915608941463976156518286253697920827223758251185210916864000000000000000000000000", 10);
  fail_unless(mpz_cmp(expected, REPBNUM(factorial)) == 0);
  mpz_clear(expected);

  factorial = INT2FIX(1);
  for (i=1; i<=100; i++)
    factorial = __arc_mul2(c, factorial, INT2FIX(i));

  /* multiply from the other side */
  mpz_init(expected);
  mpz_set_str(expected, "93326215443944152681699238856266700490715968264381621468592963895217599993229915608941463976156518286253697920827223758251185210916864000000000000000000000000", 10);
  fail_unless(mpz_cmp(expected, REPBNUM(factorial)) == 0);
  mpz_clear(expected);
}
END_TEST

START_TEST(test_mul_fixnum2rational)
{
  value v1, v2, prod;

  v1 = arc_mkrationall(c, 1, 2);
  v2 = INT2FIX(3);
  prod = __arc_mul2(c, v1, v2);
  fail_unless(TYPE(prod) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REPRAT(prod), 3, 2) == 0);

  v1 = INT2FIX(3);
  v2 = arc_mkrationall(c, 1, 2);
  prod = __arc_mul2(c, v1, v2);
  fail_unless(TYPE(prod) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REPRAT(prod), 3, 2) == 0);

  v1 = INT2FIX(2);
  v2 = arc_mkrationall(c, 1, 2);
  prod = __arc_mul2(c, v1, v2);
  fail_unless(TYPE(prod) == T_FIXNUM);
  fail_unless(FIX2INT(prod) == 1);
}
END_TEST

#endif

START_TEST(test_mul_fixnum2flonum)
{
  value val1, val2, prod;

  val1 = INT2FIX(2);
  val2 = arc_mkflonum(c, 3.14159);

  prod = __arc_mul2(c, val1, val2);
  fail_unless(TYPE(prod) == T_FLONUM);
  fail_unless(fabs(6.28318 - REPFLO(prod)) < 1e-6);

  val1 = INT2FIX(3);
  prod = __arc_mul2(c, prod, val1);
  fail_unless(TYPE(prod) == T_FLONUM);
  fail_unless(fabs(18.84954 - REPFLO(prod)) < 1e-6);
}
END_TEST

START_TEST(test_mul_fixnum2complex)
{
  value v1, v2, prod;

  v1 = arc_mkcomplex(c, 1.0 + I*2.0);
  v2 = INT2FIX(3);
  prod = __arc_mul2(c, v1, v2);
  fail_unless(TYPE(prod) == T_COMPLEX);
  fail_unless(fabs(3.0 - creal(REPCPX(prod))) < 1e-6);
  fail_unless(fabs(6.0 - cimag(REPCPX(prod))) < 1e-6);

  v1 = INT2FIX(3);
  v1 = arc_mkcomplex(c, 1.0 + I*2.0);
  prod = __arc_mul2(c, v1, v2);
  fail_unless(fabs(3.0 - creal(REPCPX(prod))) < 1e-6);
  fail_unless(fabs(6.0 - cimag(REPCPX(prod))) < 1e-6);
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

  /* Additions of fixnums */
  tcase_add_test(tc_arith, test_add_fixnum);
#ifdef HAVE_GMP_H
  tcase_add_test(tc_arith, test_add_fixnum2bignum);
  tcase_add_test(tc_arith, test_add_fixnum2rational);
#endif
  tcase_add_test(tc_arith, test_add_fixnum2flonum);
  tcase_add_test(tc_arith, test_add_fixnum2complex);


#ifdef HAVE_GMP_H
  /* Additions of bignums */
  tcase_add_test(tc_arith, test_add_bignum);

  tcase_add_test(tc_arith, test_add_bignum2rational);
  tcase_add_test(tc_arith, test_add_bignum2flonum);
  tcase_add_test(tc_arith, test_add_bignum2complex);

  /* Additions of rationals */
  tcase_add_test(tc_arith, test_add_rational);
  tcase_add_test(tc_arith, test_add_rational2flonum);
  tcase_add_test(tc_arith, test_add_rational2complex);

#endif

  /* Additions of flonums */
  tcase_add_test(tc_arith, test_add_flonum);
  tcase_add_test(tc_arith, test_add_flonum2complex);

  /* Additions of complexes */
  tcase_add_test(tc_arith, test_add_complex);

  /* Miscellaneous type additions -- it is possible to add conses
     (concatenating the lists), strings and characters (producing
     strings) */
  tcase_add_test(tc_arith, test_add_cons);
  tcase_add_test(tc_arith, test_add_string);
  tcase_add_test(tc_arith, test_add_string2char);


  /* Multiplication of fixnums */
  tcase_add_test(tc_arith, test_mul_fixnum);
#ifdef HAVE_GMP_H
  tcase_add_test(tc_arith, test_mul_fixnum2bignum);
  tcase_add_test(tc_arith, test_mul_fixnum2rational);
#endif
  tcase_add_test(tc_arith, test_mul_fixnum2flonum);
  tcase_add_test(tc_arith, test_mul_fixnum2complex);

#if 0

#ifdef HAVE_GMP_H
  /* Multiplication of bignums */
  tcase_add_test(tc_arith, test_mul_bignum);

  tcase_add_test(tc_arith, test_mul_bignum2rational);
  tcase_add_test(tc_arith, test_mul_bignum2flonum);
  tcase_add_test(tc_arith, test_mul_bignum2complex);

  /* Multiplication of rationals */
  tcase_add_test(tc_arith, test_mul_rational);
  tcase_add_test(tc_arith, test_mul_rational2flonum);
  tcase_add_test(tc_arith, test_mul_rational2complex);

#endif

  /* Multiplication of flonums */
  tcase_add_test(tc_arith, test_mul_flonum);
  tcase_add_test(tc_arith, test_mul_flonum2complex);

  /* Multiplication of complexes */
  tcase_add_test(tc_arith, test_mul_complex);

#endif

  suite_add_tcase(s, tc_arith);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

