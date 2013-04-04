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

  TEST("(coerce 1234 'num)");
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

  TEST("(coerce 39402006196394479212279040100143613805079739270465446667948293404245721771497210611414266254884915640806627990306816 'num)");
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

  TEST("(coerce 1/2 'num)");
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

  TEST("(coerce 1.0 'num)");
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

  TEST("(coerce 1.0+2.0i 'complex)");
  fail_unless(TYPE(ret) == T_COMPLEX);
  fail_unless(rel_compare(creal(REPCPX(ret)), 1.0, 1e-6));
  fail_unless(rel_compare(cimag(REPCPX(ret)), 2.0, 1e-6));

  TEST("(coerce 1.0+2.0i 'num)");
  fail_unless(TYPE(ret) == T_COMPLEX);
  fail_unless(rel_compare(creal(REPCPX(ret)), 1.0, 1e-6));
  fail_unless(rel_compare(cimag(REPCPX(ret)), 2.0, 1e-6));

  TEST("(coerce 45.23e-5+1.23i 'string)");
  fail_unless(TYPE(ret) == T_STRING);
  fail_unless(arc_strcmp(c, ret, arc_mkstringc(c, "0.0004523+1.23i")) == 0);

  TEST("(coerce 45.23e-5+1.23i 'cons)");
  fail_unless(TYPE(ret) == T_CONS);
  fail_unless(rel_compare(REPFLO(car(ret)), 0.0004523, 1e-6));
  fail_unless(rel_compare(REPFLO(cdr(ret)), 1.23, 1e-6));
}
END_TEST

START_TEST(test_coerce_char)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(coerce #\\a 'char)");
  fail_unless(TYPE(ret) == T_CHAR);
  fail_unless(arc_char2rune(c, ret) == 'a');

  TEST("(coerce #\\a 'fixnum)");
  fail_unless(TYPE(ret) == T_FIXNUM);
  fail_unless(ret == INT2FIX('a'));

  TEST("(coerce #\\a 'bignum)");
  fail_unless(TYPE(ret) == T_FIXNUM);
  fail_unless(ret == INT2FIX('a'));

  TEST("(coerce #\\a 'int)");
  fail_unless(TYPE(ret) == T_FIXNUM);
  fail_unless(ret == INT2FIX('a'));

  TEST("(coerce #\\a 'string)");
  fail_unless(TYPE(ret) == T_STRING);
  fail_unless(arc_strcmp(c, ret, arc_mkstringc(c, "a")) == 0);
}
END_TEST

START_TEST(test_coerce_string)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(coerce \"foo\" 'string)");
  fail_unless(TYPE(ret) == T_STRING);
  fail_unless(arc_strcmp(c, ret, arc_mkstringc(c, "foo")) == 0);

  TEST("(coerce \"foo\" 'sym)");
  fail_unless(TYPE(ret) == T_SYMBOL);
  fail_unless(ret == arc_intern_cstr(c, "foo"));

  TEST("(coerce \"bar\" 'cons)");
  fail_unless(TYPE(ret) == T_CONS);
  fail_unless(arc_char2rune(c, car(ret)) == 'b');
  fail_unless(arc_char2rune(c, cadr(ret)) == 'a');
  fail_unless(arc_char2rune(c, car(cddr(ret))) == 'r');

  TEST("(coerce \"31337\" 'fixnum)");
  fail_unless(ret == INT2FIX(31337));

  TEST("(coerce \"deadbee\" 'fixnum 16)");
  fail_unless(ret == INT2FIX(233495534));

  TEST("(coerce \"zyxw\" 'int 36)");
  fail_unless(ret == INT2FIX(1678244));

  TEST("(coerce \"101E100\" 'num 2)");
  fail_unless(rel_compare(REPFLO(ret), 80.0, 1e-6));

  TEST("(coerce \"101E100\" 'int 2)");
  fail_unless(ret == INT2FIX(80));

  TEST("(coerce \"FF\" 'int 16)");
  fail_unless(ret == INT2FIX(255));

  TEST("(coerce \"10.11E1010\" 'num 3)");
  fail_unless(rel_compare(REPFLO(ret), 709180566103791.0, 1e-6));

  TEST("(coerce \"10.11E1010\" 'flonum 3)");
  fail_unless(rel_compare(REPFLO(ret), 709180566103791.0, 1e-6));

  TEST("(coerce \"1\" 'flonum)");
  fail_unless(TYPE(ret) == T_FLONUM);
  fail_unless(rel_compare(REPFLO(ret), 1.0, 1e-6));

  TEST("(coerce \"1+2i\" 'num)");
  fail_unless(TYPE(ret) == T_COMPLEX);
  fail_unless(rel_compare(cabs(REPCPX(ret)), cabs(1.0+I*2.0), 1e-6));

  TEST("(coerce \"1+2i\" 'complex)");
  fail_unless(TYPE(ret) == T_COMPLEX);
  fail_unless(rel_compare(cabs(REPCPX(ret)), cabs(1.0+I*2.0), 1e-6));

#ifdef HAVE_GMP_H
  {
    static const char *expected_str = "39402006196394479212279040100143613805079739270465446667948293404245721771497210611414266254884915640806627990306816";
    value expected;

    expected = arc_mkbignuml(c, 0L);
    mpz_set_str(REPBNUM(expected), expected_str, 10);

    TEST("(coerce \"39402006196394479212279040100143613805079739270465446667948293404245721771497210611414266254884915640806627990306816\" 'fixnum)");
    fail_unless(TYPE(ret) == T_BIGNUM);
    fail_unless(arc_is2(c, ret, expected) == CTRUE);

   TEST("(coerce \"39402006196394479212279040100143613805079739270465446667948293404245721771497210611414266254884915640806627990306816\" 'bignum)");
    fail_unless(TYPE(ret) == T_BIGNUM);
    fail_unless(arc_is2(c, ret, expected) == CTRUE);

   TEST("(coerce \"39402006196394479212279040100143613805079739270465446667948293404245721771497210611414266254884915640806627990306816\" 'int)");
    fail_unless(TYPE(ret) == T_BIGNUM);
    fail_unless(arc_is2(c, ret, expected) == CTRUE);

    TEST("(coerce \"2op9vv3r85y0ag8ukw7bqnnknjigy9r4407r3dbiq68kv8h2zuqyon925oqg0whhftleubw3g1s\" 'int 36)");
    fail_unless(TYPE(ret) == T_BIGNUM);
    fail_unless(arc_is2(c, ret, expected) == CTRUE);

    TEST("(coerce \"1/2\" 'rational)");
    fail_unless(TYPE(ret) == T_RATIONAL);
    fail_unless(mpq_cmp_si(REPRAT(ret), 1, 2) == 0);

    TEST("(coerce \"1/2\" 'num)");
    fail_unless(TYPE(ret) == T_RATIONAL);
    fail_unless(mpq_cmp_si(REPRAT(ret), 1, 2) == 0);

  }
#endif


}
END_TEST

START_TEST(test_coerce_symbol)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(coerce 'foo 'sym)");
  fail_unless(TYPE(ret) == T_SYMBOL);
  fail_unless(ret == arc_intern_cstr(c, "foo"));

  TEST("(coerce 'foo 'string)");
  fail_unless(TYPE(ret) == T_STRING);
  fail_unless(arc_strcmp(c, ret, arc_mkstringc(c, "foo")) == 0);

  TEST("(coerce nil 'string)");
  fail_unless(TYPE(ret) == T_STRING);
  fail_unless(arc_strlen(c, ret) == 0);

  TEST("(coerce t 'string)");
  fail_unless(TYPE(ret) == T_STRING);
  fail_unless(arc_strcmp(c, ret, arc_mkstringc(c, "t")) == 0);
}
END_TEST

START_TEST(test_coerce_cons)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(coerce '(foo bar) 'cons)");
  fail_unless(TYPE(ret) == T_CONS);
  fail_unless(car(ret) == arc_intern_cstr(c, "foo"));
  fail_unless(cadr(ret) == arc_intern_cstr(c, "bar"));

  TEST("(coerce '(1 2 3 4) 'string)");
  fail_unless(TYPE(ret) == T_STRING);
  fail_unless(arc_strcmp(c, ret, arc_mkstringc(c, "1234")) == 0);

  /* EXT - invalid in reference arc */
  TEST("(coerce '(10 11 12 13 14 15) 'string 16)");
  fail_unless(TYPE(ret) == T_STRING);
  fail_unless(arc_strcmp(c, ret, arc_mkstringc(c, "abcdef")) == 0);

  TEST("(coerce '(1 2 3 4) 'vector)");
  fail_unless(TYPE(ret) == T_VECTOR);
  fail_unless(VECLEN(ret) == 4);
  fail_unless(VINDEX(ret, 0) == INT2FIX(1));
  fail_unless(VINDEX(ret, 1) == INT2FIX(2));
  fail_unless(VINDEX(ret, 2) == INT2FIX(3));
  fail_unless(VINDEX(ret, 3) == INT2FIX(4));

  TEST("(coerce '(1 2 3 . 4) 'vector)");
  fail_unless(TYPE(ret) == T_VECTOR);
  fail_unless(VECLEN(ret) == 4);
  fail_unless(VINDEX(ret, 0) == INT2FIX(1));
  fail_unless(VINDEX(ret, 1) == INT2FIX(2));
  fail_unless(VINDEX(ret, 2) == INT2FIX(3));
  fail_unless(VINDEX(ret, 3) == INT2FIX(4));


  TEST("(coerce '((1 . 2) (2 . 3) (3 . 4)) 'table)");
  fail_unless(TYPE(ret) == T_TABLE);
  fail_unless(arc_hash_lookup(c, ret, INT2FIX(1)) == INT2FIX(2));
  fail_unless(arc_hash_lookup(c, ret, INT2FIX(2)) == INT2FIX(3));
  fail_unless(arc_hash_lookup(c, ret, INT2FIX(3)) == INT2FIX(4));
}
END_TEST

START_TEST(test_coerce_table)
{
  value thr, cctx, clos, code, ret, tbl;

  thr = arc_mkthread(c);
  tbl = arc_mkhash(c, ARC_HASHBITS);
  arc_bindsym(c, arc_intern_cstr(c, "myhash"), tbl);
  arc_hash_insert(c, tbl, INT2FIX(1), INT2FIX(2));
  arc_hash_insert(c, tbl, INT2FIX(2), INT2FIX(3));
  arc_hash_insert(c, tbl, INT2FIX(3), INT2FIX(4));

  TEST("(coerce myhash 'table)");
  fail_unless(TYPE(ret) == T_TABLE);
  fail_unless(arc_hash_lookup(c, ret, INT2FIX(1)) == INT2FIX(2));
  fail_unless(arc_hash_lookup(c, ret, INT2FIX(2)) == INT2FIX(3));
  fail_unless(arc_hash_lookup(c, ret, INT2FIX(3)) == INT2FIX(4));

  TEST("(coerce myhash 'cons)");
  fail_unless(TYPE(ret) == T_CONS);
  /* the order in which these pairs occur is essentially random */
  fail_unless(TYPE(car(car(ret))) == T_FIXNUM);
  fail_unless(TYPE(cdr(car(ret))) == T_FIXNUM);
  fail_unless(TYPE(car(cadr(ret))) == T_FIXNUM);
  fail_unless(TYPE(cdr(cadr(ret))) == T_FIXNUM);
  fail_unless(TYPE(car(car(cddr(ret)))) == T_FIXNUM);
  fail_unless(TYPE(cdr(car(cddr(ret)))) == T_FIXNUM);
  fail_unless(NIL_P(cdr(cddr(ret))));
}
END_TEST

START_TEST(test_coerce_vector)
{
  value thr, cctx, clos, code, ret, vec;

  thr = arc_mkthread(c);
  vec = arc_mkvector(c, 3);
  VINDEX(vec, 0) = FIX2INT(1);
  VINDEX(vec, 1) = FIX2INT(2);
  VINDEX(vec, 2) = FIX2INT(3);
  arc_bindsym(c, arc_intern_cstr(c, "myvec"), vec);

  TEST("(coerce myvec 'vector)");
  fail_unless(TYPE(ret) == T_VECTOR);
  fail_unless(VINDEX(ret, 0) == FIX2INT(1));
  fail_unless(VINDEX(ret, 1) == FIX2INT(2));
  fail_unless(VINDEX(ret, 2) == FIX2INT(3));

  TEST("(coerce myvec 'cons)");
  fail_unless(TYPE(ret) == T_CONS);
  fail_unless(car(ret) == FIX2INT(1));
  fail_unless(cadr(ret) == FIX2INT(2));
  fail_unless(car(cddr(ret)) == FIX2INT(3));
  fail_unless(NIL_P(cdr(cddr(ret))));
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

  tcase_add_test(tc_bif, test_coerce_char);
  tcase_add_test(tc_bif, test_coerce_string);
  tcase_add_test(tc_bif, test_coerce_symbol);
  tcase_add_test(tc_bif, test_coerce_cons);
  tcase_add_test(tc_bif, test_coerce_table);
  tcase_add_test(tc_bif, test_coerce_vector);

  suite_add_tcase(s, tc_bif);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

