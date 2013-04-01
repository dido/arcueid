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
#include "../src/arith.h"
#include "../config.h"
#include "../src/vmengine.h"
#include "../src/builtins.h"

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

arc *c, cc;

#define QUANTA 65536

#define CPUSH_(val) CPUSH(thr, val)

#define XCALL0(clos) do {			\
    TQUANTA(thr) = QUANTA;			\
    TVALR(thr) = clos;				\
    TARGC(thr) = 0;				\
    __arc_thr_trampoline(c, thr, TR_FNAPP);	\
  } while (0)

#define XCALL(fname, ...) do {			\
    TVALR(thr) = arc_mkaff(c, fname, CNIL);	\
    TARGC(thr) = NARGS(__VA_ARGS__);		\
    FOR_EACH(CPUSH_, __VA_ARGS__);		\
    __arc_thr_trampoline(c, thr, TR_FNAPP);	\
  } while (0)

AFFDEF(compile_something)
{
  AARG(something);
  value sexpr;
  AVAR(sio);
  AFBEGIN;
  TQUANTA(thr) = QUANTA;	/* needed so macros can execute */
  AV(sio) = arc_instring(c, AV(something), CNIL);
  AFCALL(arc_mkaff(c, arc_sread, CNIL), AV(sio), CNIL);
  sexpr = AFCRV;
  AFTCALL(arc_mkaff(c, arc_compile, CNIL), sexpr, arc_mkcctx(c), CNIL, CTRUE);
  AFEND;
}
AFFEND

#define COMPILE(str) XCALL(compile_something, arc_mkstringc(c, str))

#define TEST(sexpr)				\
  COMPILE(sexpr);				\
  cctx = TVALR(thr);				\
  code = arc_cctx2code(c, cctx);		\
  clos = arc_mkclos(c, code, CNIL);		\
  XCALL0(clos);					\
  ret = TVALR(thr)

extern void __arc_print_string(arc *c, value ppstr);


START_TEST(test_coerce_fixnum)
{
  value thr, cctx, clos, code, ret;
  char *fixnumstr, *coercestr;
  int len;

  thr = arc_mkthread(c);

  TEST("(coerce 1234 'fixnum)");
  fail_unless(TYPE(ret) == T_FIXNUM);
  fail_unless(ret == INT2FIX(1234));

  TEST("(coerce 1234 'int)");
  fail_unless(TYPE(ret) == T_FIXNUM);
  fail_unless(ret == INT2FIX(1234));

  TEST("(coerce 1234 'bignum)");
  fail_unless(TYPE(ret) == T_FIXNUM);
  fail_unless(ret == INT2FIX(1234));

  TEST("(coerce 1234 'rational)");
  fail_unless(TYPE(ret) == T_FIXNUM);
  fail_unless(ret == INT2FIX(1234));

  TEST("(coerce 1234 'flonum)");
  fail_unless(TYPE(ret) == T_FLONUM);
  fail_unless(fabs(REPFLO(ret) - 1234.0) < 1e-6);

  TEST("(coerce 1234 'complex)");
  fail_unless(TYPE(ret) == T_FLONUM);
  fail_unless(fabs(REPFLO(ret) - 1234.0) < 1e-6);

  TEST("(coerce 65 'char)");
  fail_unless(TYPE(ret) == T_CHAR);
  fail_unless(arc_char2rune(c, ret) == 'A');

  TEST("(coerce 1234 'string)");
  fail_unless(TYPE(ret) == T_STRING);
  fail_unless(arc_strcmp(c, ret, arc_mkstringc(c, "1234")) == 0);

  TEST("(coerce -1234 'string)");
  fail_unless(TYPE(ret) == T_STRING);
  fail_unless(arc_strcmp(c, ret, arc_mkstringc(c, "-1234")) == 0);

  len = snprintf(NULL, 0, "%ld", FIXNUM_MAX);
  fixnumstr = alloca(sizeof(char)*(len+1));
  snprintf(fixnumstr, len+1, "%ld", FIXNUM_MAX);

  len = snprintf(NULL, 0, "(coerce %s 'string)", fixnumstr);
  coercestr = alloca(sizeof(char)*(len+1));
  snprintf(coercestr, len+1, "(coerce %s 'string)", fixnumstr);

  TEST(coercestr);
  fail_unless(TYPE(ret) == T_STRING);
  fail_unless(arc_strcmp(c, ret, arc_mkstringc(c, fixnumstr)) == 0);

  len = snprintf(NULL, 0, "%ld", FIXNUM_MIN);
  fixnumstr = alloca(sizeof(char)*(len+1));
  snprintf(fixnumstr, len+1, "%ld", FIXNUM_MIN);

  len = snprintf(NULL, 0, "(coerce %s 'string)", fixnumstr);
  coercestr = alloca(sizeof(char)*(len+1));
  snprintf(coercestr, len+1, "(coerce %s 'string)", fixnumstr);

  TEST(coercestr);
  fail_unless(TYPE(ret) == T_STRING);
  fail_unless(arc_strcmp(c, ret, arc_mkstringc(c, fixnumstr)) == 0);

  /* different bases */

  TEST("(coerce 233495534 'string 16)");
  fail_unless(TYPE(ret) == T_STRING);
  fail_unless(arc_strcmp(c, ret, arc_mkstringc(c, "deadbee")) == 0);

  TEST("(coerce 1678244 'string 36)");
  fail_unless(TYPE(ret) == T_STRING);
  fail_unless(arc_strcmp(c, ret, arc_mkstringc(c, "zyxw")) == 0);

}
END_TEST

static int rel_compare(double a, double b, double maxdiff)
{
  double diff = fabs(a - b);
  double largest;

  a = fabs(a);
  b = fabs(b);
  largest = (a > b) ? a : b;
  if (diff <= largest * maxdiff)
    return(1);
  return(0);
}

#ifdef HAVE_GMP_H

START_TEST(test_coerce_bignum)
{
  value thr, cctx, clos, code, ret, expected;
  static const char *expected_str = "39402006196394479212279040100143613805079739270465446667948293404245721771497210611414266254884915640806627990306816";
  static const char *b36str = "2op9vv3r85y0ag8ukw7bqnnknjigy9r4407r3dbiq68kv8h2zuqyon925oqg0whhftleubw3g1s";

  expected = arc_mkbignuml(c, 0L);
  mpz_set_str(REPBNUM(expected), expected_str, 10);
  thr = arc_mkthread(c);

  TEST("(coerce 39402006196394479212279040100143613805079739270465446667948293404245721771497210611414266254884915640806627990306816 'fixnum)");
  fail_unless(TYPE(ret) == T_BIGNUM);
  fail_unless(arc_is2(c, ret, expected) == CTRUE);

  TEST("(coerce 39402006196394479212279040100143613805079739270465446667948293404245721771497210611414266254884915640806627990306816 'int)");
  fail_unless(TYPE(ret) == T_BIGNUM);
  fail_unless(arc_is2(c, ret, expected) == CTRUE);

  TEST("(coerce 39402006196394479212279040100143613805079739270465446667948293404245721771497210611414266254884915640806627990306816 'bignum)");
  fail_unless(TYPE(ret) == T_BIGNUM);
  fail_unless(arc_is2(c, ret, expected) == CTRUE);

  TEST("(coerce 39402006196394479212279040100143613805079739270465446667948293404245721771497210611414266254884915640806627990306816 'rational)");
  fail_unless(TYPE(ret) == T_BIGNUM);
  fail_unless(arc_is2(c, ret, expected) == CTRUE);

  TEST("(coerce 39402006196394479212279040100143613805079739270465446667948293404245721771497210611414266254884915640806627990306816 'flonum)");
  fail_unless(TYPE(ret) == T_FLONUM);
  fail_unless(rel_compare(REPFLO(ret), 3.940200619639448e+115, 1e-6));

  TEST("(coerce 39402006196394479212279040100143613805079739270465446667948293404245721771497210611414266254884915640806627990306816 'complex)");
  fail_unless(TYPE(ret) == T_FLONUM);
  fail_unless(rel_compare(REPFLO(ret), 3.940200619639448e+115, 1e-6));

  TEST("(coerce 39402006196394479212279040100143613805079739270465446667948293404245721771497210611414266254884915640806627990306816 'string)");
  fail_unless(TYPE(ret) == T_STRING);
  fail_unless(arc_strcmp(c, ret, arc_mkstringc(c, expected_str)) == 0);

  TEST("(coerce 39402006196394479212279040100143613805079739270465446667948293404245721771497210611414266254884915640806627990306816 'string 36)");
  fail_unless(TYPE(ret) == T_STRING);
  fail_unless(arc_strcmp(c, ret, arc_mkstringc(c, b36str)) == 0);

}
END_TEST

START_TEST(test_coerce_rational)
{

  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);

  TEST("(coerce 3/2 'fixnum)");
  fail_unless(TYPE(ret) == T_FIXNUM);
  fail_unless(ret == INT2FIX(2));

  TEST("(coerce 5/2 'fixnum)");
  fail_unless(TYPE(ret) == T_FIXNUM);
  fail_unless(ret == INT2FIX(2));

  TEST("(coerce 7/2 'fixnum)");
  fail_unless(TYPE(ret) == T_FIXNUM);
  fail_unless(ret == INT2FIX(4));

  TEST("(coerce 9/2 'fixnum)");
  fail_unless(TYPE(ret) == T_FIXNUM);
  fail_unless(ret == INT2FIX(4));

  TEST("(coerce 11/2 'fixnum)");
  fail_unless(TYPE(ret) == T_FIXNUM);
  fail_unless(ret == INT2FIX(6));

  TEST("(coerce 11/2 'fixnum)");
  fail_unless(TYPE(ret) == T_FIXNUM);
  fail_unless(ret == INT2FIX(6));

  TEST("(coerce 1/2 'rational)");
  fail_unless(TYPE(ret) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REPRAT(ret), 1, 2) == 0);

  TEST("(coerce 1/2 'flonum)");
  fail_unless(TYPE(ret) == T_FLONUM);
  fail_unless(rel_compare(REPFLO(ret), 0.5, 1e-6));

  TEST("(coerce 1/2 'complex)");
  fail_unless(TYPE(ret) == T_FLONUM);
  fail_unless(rel_compare(REPFLO(ret), 0.5, 1e-6));

  TEST("(coerce 1/2 'string)");
  fail_unless(TYPE(ret) == T_STRING);
  fail_unless(arc_strcmp(c, ret, arc_mkstringc(c, "1/2")) == 0);

  TEST("(coerce 1/2 'cons)");
  fail_unless(TYPE(ret) == T_CONS);
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(cdr(ret) == INT2FIX(2));

}
END_TEST

#endif

START_TEST(test_coerce_flonum)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);

  TEST("(coerce 1.0 'flonum)");
  fail_unless(TYPE(ret) == T_FLONUM);
  fail_unless(rel_compare(REPFLO(ret), 1.0, 1e-6));

  TEST("(coerce 1.0 'complex)");
  fail_unless(TYPE(ret) == T_FLONUM);
  fail_unless(rel_compare(REPFLO(ret), 1.0, 1e-6));

  TEST("(coerce 1234.0 'fixnum)");
  fail_unless(TYPE(ret) == T_FIXNUM);
  fail_unless(ret == INT2FIX(1234));

#ifdef HAVE_GMP_H

  TEST("(coerce 1e100 'fixnum)");
  fail_unless(TYPE(ret) == T_BIGNUM);
  /* I don't think a more exact check is possible: 1e100 has 101 decimal
     digits. */
  fail_unless(mpz_sizeinbase(REPBNUM(ret), 10) == 101);

  TEST("(coerce 1e100 'bignum)");
  fail_unless(TYPE(ret) == T_BIGNUM);
  fail_unless(mpz_sizeinbase(REPBNUM(ret), 10) == 101);

  TEST("(coerce 1e100 'int)");
  fail_unless(TYPE(ret) == T_BIGNUM);
  fail_unless(mpz_sizeinbase(REPBNUM(ret), 10) == 101);

  /* I suppose 0.5 should be exactly enough represented in binary
     floating point. */
  TEST("(coerce 0.5 'rational)");
  fail_unless(TYPE(ret) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REPRAT(ret), 1, 2) == 0);

#endif

  TEST("(coerce 45.23e-5 'string)");
  fail_unless(TYPE(ret) == T_STRING);
  fail_unless(arc_strcmp(c, ret, arc_mkstringc(c, "0.0004523")) == 0);

}
END_TEST


START_TEST(test_coerce_complex)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);

  TEST("(coerce 1.0+2.0i 'flonum)");
  fail_unless(TYPE(ret) == T_FLONUM);
  fail_unless(rel_compare(REPFLO(ret), 1.0, 1e-6));

  TEST("(coerce 1.0+2.0i 'complex)");
  fail_unless(TYPE(ret) == T_COMPLEX);
  fail_unless(rel_compare(creal(REPCPX(ret)), 1.0, 1e-6));
  fail_unless(rel_compare(cimag(REPCPX(ret)), 2.0, 1e-6));

  TEST("(coerce 1234.0+5678.0i 'fixnum)");
  fail_unless(TYPE(ret) == T_FIXNUM);
  fail_unless(ret == INT2FIX(1234));

#ifdef HAVE_GMP_H

  TEST("(coerce 1e100+1e200i 'fixnum)");
  fail_unless(TYPE(ret) == T_BIGNUM);
  /* I don't think a more exact check is possible: 1e100 has 101 decimal
     digits. */
  fail_unless(mpz_sizeinbase(REPBNUM(ret), 10) == 101);

  TEST("(coerce 1e100+1e200i 'bignum)");
  fail_unless(TYPE(ret) == T_BIGNUM);
  fail_unless(mpz_sizeinbase(REPBNUM(ret), 10) == 101);

  TEST("(coerce 1e100+1e200i 'int)");
  fail_unless(TYPE(ret) == T_BIGNUM);
  fail_unless(mpz_sizeinbase(REPBNUM(ret), 10) == 101);

  /* I suppose 0.5 should be exactly enough represented in binary
     floating point. */
  TEST("(coerce 0.5+0.25i 'rational)");
  fail_unless(TYPE(ret) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REPRAT(ret), 1, 2) == 0);

#endif

  TEST("(coerce 45.23e-5+1.23i 'string)");
  fail_unless(TYPE(ret) == T_STRING);
  fail_unless(arc_strcmp(c, ret, arc_mkstringc(c, "0.0004523+1.23i")) == 0);

}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Built-in Functions");
  TCase *tc_bif = tcase_create("Built-in Functions");
  SRunner *sr;

  c = &cc;
  arc_init(c);

  tcase_add_test(tc_bif, test_coerce_fixnum);

#ifdef HAVE_GMP_H
  tcase_add_test(tc_bif, test_coerce_bignum);
  tcase_add_test(tc_bif, test_coerce_rational);
#endif

  tcase_add_test(tc_bif, test_coerce_flonum);
  tcase_add_test(tc_bif, test_coerce_complex);

  suite_add_tcase(s, tc_bif);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

