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
#include "../src/compiler.h"
#include "../src/io.h"
#include "../src/hash.h"

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
    c->curthread = thr;				\
    TQUANTA(thr) = QUANTA;			\
    SVALR(thr, clos);				\
    TARGC(thr) = 0;				\
    __arc_thr_trampoline(c, thr, TR_FNAPP);	\
  } while (0)

#define XCALL(fname, ...) do {			\
    SVALR(thr, arc_mkaff(c, fname, CNIL));	\
    c->curthread = thr;				\
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
  WV(sio, arc_instring(c, AV(something), CNIL));
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
  SVINDEX(vec, 0, FIX2INT(1));
  SVINDEX(vec, 1, FIX2INT(2));
  SVINDEX(vec, 2, FIX2INT(3));
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

START_TEST(test_sym)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(sym \"quux\")");
  fail_unless(TYPE(ret) == T_SYMBOL);
  fail_unless(ret == arc_intern_cstr(c, "quux"));
}
END_TEST

START_TEST(test_gt)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(> 4 3 2 1)");
  fail_unless(ret == CTRUE);

  TEST("(> 4 3 2 3)");
  fail_unless(NIL_P(ret));

  TEST("(> 4 4 3 2)");
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_lt)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(< 1 2 3 4)");
  fail_unless(ret == CTRUE);

  TEST("(< 1 2 4 4)");
  fail_unless(NIL_P(ret));

  TEST("(< 3 2 3 4)");
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_spaceship)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  /* fixnum */
  TEST("(<=> 1 1)");
  fail_unless(ret == INT2FIX(0));

  TEST("(<=> 2 1)");
  fail_unless(ret == INT2FIX(1));

  TEST("(<=> 1 2)");
  fail_unless(ret == INT2FIX(-1));

#ifdef HAVE_GMP_H
  /* bignum */
  TEST("(<=> 10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000)");
  fail_unless(ret == INT2FIX(0));

  TEST("(<=> 100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002 100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001)");
  fail_unless(ret == INT2FIX(1));

  TEST("(<=> 100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001 100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002)");
  fail_unless(ret == INT2FIX(-1));

  /* rational */
  TEST("(<=> 1/3 1/3)");
  fail_unless(ret == INT2FIX(0));

  TEST("(<=> 2/3 1/3)");
  fail_unless(ret == INT2FIX(1));

  TEST("(<=> 1/3 2/3)");
  fail_unless(ret == INT2FIX(-1));
#endif

  /* flonum */
  TEST("(<=> 1.0 1.0)");
  fail_unless(ret == INT2FIX(0));

  TEST("(<=> 2.0 1.0)");
  fail_unless(ret == INT2FIX(1));

  TEST("(<=> 1.0 2.0)");
  fail_unless(ret == INT2FIX(-1));

  /* char */
  TEST("(<=> #\\a #\\a)");
  fail_unless(ret == INT2FIX(0));

  TEST("(<=> #\\b #\\a)");
  fail_unless(ret == INT2FIX(1));

  TEST("(<=> #\\a #\\b)");
  fail_unless(ret == INT2FIX(-1));

  /* string */
  TEST("(<=> \"foo\" \"foo\")");
  fail_unless(ret == INT2FIX(0));

  TEST("(<=> \"foo\" \"bar\")");
  fail_unless(ret == INT2FIX(1));

  TEST("(<=> \"bar\" \"foo\")");
  fail_unless(ret == INT2FIX(-1));
}
END_TEST

START_TEST(test_bound)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(bound 'bound)");
  fail_unless(ret == CTRUE);

  TEST("(bound 'xyzzy)");
  fail_unless(ret == CNIL);
}
END_TEST

START_TEST(test_exact)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(exact 31337)");
  fail_unless(ret == CTRUE);

  TEST("(exact 3.14159)");
  fail_unless(ret == CNIL);

  TEST("(exact 3.14159+2.718i)");
  fail_unless(ret == CNIL);

#ifdef HAVE_GMP_H

  TEST("(exact 10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000)");
  fail_unless(ret == CTRUE);

  TEST("(exact 1/2)");
  fail_unless(ret == CTRUE);

#endif
}
END_TEST

START_TEST(test_is)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(is 'a 'a)");
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_iso)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(iso '(1 2 3) '(1 2 3))");
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_div)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(idiv 10 3)");
  fail_unless(ret == INT2FIX(3));
}
END_TEST

START_TEST(test_expt)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(expt 2 24)");
  fail_unless(ret == INT2FIX(16777216));

  TEST("(expt 2 256)");
#ifdef HAVE_GMP_H 
  {
    static const char *expected_str = "115792089237316195423570985008687907853269984665640564039457584007913129639936";
    value expected;

    expected = arc_mkbignuml(c, 0L);
    mpz_set_str(REPBNUM(expected), expected_str, 10);
    fail_unless(TYPE(ret) == T_BIGNUM);
    fail_unless(arc_numcmp(c, ret, expected) == INT2FIX(0));
  }
#else
  fail_unless(TYPE(ret) == T_FLONUM);
  fail_unless(rel_compare(REPFLO(ret), 1.157920892373162e+77, 1e-6));
#endif

  TEST("(expt 2.0 256.0)");
  fail_unless(TYPE(ret) == T_FLONUM);
  fail_unless(rel_compare(REPFLO(ret), 1.157920892373162e+77, 1e-6));

  TEST("(expt -1 0.5)");  
  fail_unless(TYPE(ret) == T_COMPLEX);
  fail_unless(fabs(creal(REPCPX(ret))) < 1e-6);
  fail_unless(rel_compare(cimag(REPCPX(ret)), 1.0, 1e-6));

  TEST("(expt 1+1i 2+2i)");
  fail_unless(TYPE(ret) == T_COMPLEX);
  fail_unless(rel_compare(creal(REPCPX(ret)), -0.26565399849241, 1e-6));
  fail_unless(rel_compare(cimag(REPCPX(ret)), 0.319818113856136, 1e-6));
}
END_TEST

START_TEST(test_mod)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(mod 10 3)");
  fail_unless(ret == INT2FIX(1));

  /* Euclidean modulus */
  TEST("(mod -10 3)");
  fail_unless(ret == INT2FIX(2));
}
END_TEST

START_TEST(test_sqrt)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(sqrt 4)");
  fail_unless(ret == INT2FIX(2));

  thr = arc_mkthread(c);
  TEST("(sqrt 2)");
  fail_unless(TYPE(ret) == T_FLONUM);
  fail_unless(rel_compare(REPFLO(ret), 1.4142135623730951, 1e-6));

  TEST("(sqrt -1)");
  fail_unless(TYPE(ret) == T_COMPLEX);
  fail_unless(fabs(creal(REPCPX(ret))) < 1e-6);
  fail_unless(fabs(cimag(REPCPX(ret)) - 1) < 1e-6);

  TEST("(sqrt 1+1i)");
  fail_unless(TYPE(ret) == T_COMPLEX);
  fail_unless(rel_compare(creal(REPCPX(ret)), 1.0986841134678098, 1e-6));
  fail_unless(rel_compare(cimag(REPCPX(ret)), 0.45508986056222733, 1e-6));

#ifdef HAVE_GMP_H
  TEST("(is (sqrt 10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000) 100000000000000000000000000000000000000000000000000)");
  fail_unless(ret == CTRUE);

  TEST("(sqrt 1000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000)");
  fail_unless(TYPE(ret) == T_FLONUM);
  fail_unless(rel_compare(REPFLO(ret), 3.1622776601683793e+49, 1e-6));

  TEST("(is (sqrt 1/4) 1/2)");
  fail_unless(ret == CTRUE);

  TEST("(sqrt 1/2)");
  fail_unless(TYPE(ret) == T_FLONUM);
  fail_unless(rel_compare(REPFLO(ret), 0.7071067811865476, 1e-6));

#endif
}
END_TEST

START_TEST(test_real)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(real 1.1)");
  fail_unless(TYPE(ret) == T_FLONUM);
  fail_unless(rel_compare(REPFLO(ret), 1.1, 1e-6));

  thr = arc_mkthread(c);
  TEST("(real 1)");
  fail_unless(ret == INT2FIX(1));

  TEST("(real 1.2+2.1i)");
  fail_unless(TYPE(ret) == T_FLONUM);
  fail_unless(rel_compare(REPFLO(ret), 1.2, 1e-6));

}
END_TEST

START_TEST(test_imag)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(imag 1.1)");
  fail_unless(ret == INT2FIX(0));

  thr = arc_mkthread(c);
  TEST("(imag 1)");
  fail_unless(ret == INT2FIX(0));

  TEST("(imag 1.2+2.1i)");
  fail_unless(TYPE(ret) == T_FLONUM);
  fail_unless(rel_compare(REPFLO(ret), 2.1, 1e-6));

}
END_TEST

START_TEST(test_conj)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(conj 1.1)");
  fail_unless(TYPE(ret) == T_FLONUM);
  fail_unless(rel_compare(REPFLO(ret), 1.1, 1e-6));

  thr = arc_mkthread(c);
  TEST("(conj 1)");
  fail_unless(ret == INT2FIX(1));

  TEST("(conj 1.2+2.1i)");
  fail_unless(TYPE(ret) == T_COMPLEX);
  fail_unless(rel_compare(creal(REPCPX(ret)), 1.2, 1e-6));
  fail_unless(rel_compare(cimag(REPCPX(ret)), -2.1, 1e-6));
}
END_TEST

START_TEST(test_arg)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(arg 1.1)");
  fail_unless(ret == INT2FIX(0));

  thr = arc_mkthread(c);
  TEST("(arg 1)");
  fail_unless(ret == INT2FIX(0));

  TEST("(arg 1.2+2.1i)");
  fail_unless(TYPE(ret) == T_FLONUM);
  fail_unless(rel_compare(REPFLO(ret), 1.0516502125483738, 1e-6));
}
END_TEST

START_TEST(test_rand_srand)
{
  value thr, cctx, clos, code, ret;

  /* Note: these tests depend on the fact that Arcueid uses the ISAAC64
     RNG, so they're really a test for the RNG implementation. */
  thr = arc_mkthread(c);
  TEST("(srand 31337)");
  fail_unless(ret == INT2FIX(31337));

  TEST("(rand)");
  fail_unless(TYPE(ret) == T_FLONUM);
  fail_unless(REPFLO(ret) <= 1.0 && REPFLO(ret) >= 0.0);
  fail_unless(rel_compare(REPFLO(ret), 0.875883, 1e-6));

  TEST("(rand 65536)");
  fail_unless(TYPE(ret) == T_FIXNUM);
  fail_unless(ret == INT2FIX(17800));

  TEST("(rand 65536)");
  fail_unless(TYPE(ret) == T_FIXNUM);
  fail_unless(ret == INT2FIX(36822));

  TEST("(rand 65536)");
  fail_unless(TYPE(ret) == T_FIXNUM);
  fail_unless(ret == INT2FIX(36591));

  TEST("(rand 65536)");
  fail_unless(TYPE(ret) == T_FIXNUM);
  fail_unless(ret == INT2FIX(41661));

  TEST("(rand 65536)");
  fail_unless(TYPE(ret) == T_FIXNUM);
  fail_unless(ret == INT2FIX(44925));

  TEST("(rand 65536)");
  fail_unless(TYPE(ret) == T_FIXNUM);
  fail_unless(ret == INT2FIX(55506));

  TEST("(rand 65536)");
  fail_unless(TYPE(ret) == T_FIXNUM);
  fail_unless(ret == INT2FIX(1287));
}
END_TEST

START_TEST(test_trunc)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(trunc 13.0)");
  fail_unless(ret == INT2FIX(13));

  thr = arc_mkthread(c);
  TEST("(trunc 3.1415)");
  fail_unless(ret == INT2FIX(3));

#ifdef HAVE_GMP_H
  TEST("(trunc 11/2)");
  fail_unless(ret == INT2FIX(5));
#endif
}
END_TEST

START_TEST(test_maptable)
{
  value thr, cctx, clos, code, ret, tbl;

  thr = arc_mkthread(c);
  tbl = arc_mkhash(c, ARC_HASHBITS);
  arc_bindsym(c, arc_intern_cstr(c, "mytbl"), tbl);
  arc_hash_insert(c, tbl, INT2FIX(1), INT2FIX(2));
  arc_hash_insert(c, tbl, INT2FIX(2), INT2FIX(3));
  arc_hash_insert(c, tbl, INT2FIX(3), INT2FIX(4));
  TEST("((fn (count tbl) (maptable (fn (k v) (assign count (+ count k)) (assign count (+ count v))) tbl) count) 0 mytbl)");
  fail_unless(ret == INT2FIX(15));
}
END_TEST

START_TEST(test_eval)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(eval '((fn (x) (+ x 1)) 2))");
  fail_unless(ret == INT2FIX(3));
}
END_TEST

START_TEST(test_ssyntax)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(ssyntax 'xyzzy)");
  fail_unless(NIL_P(ret));

  TEST("(ssyntax 'a:b)");
  fail_unless(ret == CTRUE);

}
END_TEST

START_TEST(test_ssexpand)
{
  value thr, cctx, clos, code, ret, sexpr;

  thr = arc_mkthread(c);
  TEST("(ssexpand 'foo:bar:baz)");
  sexpr = ret;
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(car(sexpr) == ARC_BUILTIN(c, S_COMPOSE));
  fail_unless(TYPE(car(cdr(sexpr))) == T_SYMBOL);
  fail_unless(car(cdr(sexpr)) == arc_intern(c, arc_mkstringc(c, "foo")));
  fail_unless(TYPE(car(cdr(cdr(sexpr)))) == T_SYMBOL);
  fail_unless(car(cdr(cdr(sexpr))) == arc_intern(c, arc_mkstringc(c, "bar")));
  fail_unless(TYPE(car(cdr(cdr(cdr(sexpr))))) == T_SYMBOL);
  fail_unless(car(cdr(cdr(cdr(sexpr)))) == arc_intern(c, arc_mkstringc(c, "baz")));
}
END_TEST

/* also tests non-inlined arithmetic operators */
START_TEST(test_apply)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(apply (fn x x))");
  fail_unless(NIL_P(ret));

  TEST("(apply (fn x x) nil)");
  fail_unless(NIL_P(ret));

  TEST("(apply (fn x x) 1 2 3 nil)");
  fail_unless(TYPE(ret) == T_CONS);
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(cadr(ret) == INT2FIX(2));
  fail_unless(car(cddr(ret)) == INT2FIX(3));

  TEST("(apply (fn x x) 1 2 '(3 4))");
  fail_unless(TYPE(ret) == T_CONS);
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(cadr(ret) == INT2FIX(2));
  fail_unless(car(cddr(ret)) == INT2FIX(3));
  fail_unless(cadr(cddr(ret)) == INT2FIX(4));

  TEST("(apply + '(1 2 3))");
  fail_unless(ret == INT2FIX(6));

  TEST("(apply - '(3 2 1))");
  fail_unless(ret == INT2FIX(0));

  TEST("(apply * '(1 2 3 4 5 6 7))");
  fail_unless(ret == INT2FIX(5040));

  TEST("(apply / '(5040 7 6 5 4 3 2))");
  fail_unless(ret == INT2FIX(1));

  /* tests for list operations in a non-functional position */
  TEST("(apply car '((1 2)))");
  fail_unless(ret == INT2FIX(1));

  TEST("(apply cdr '((1 2)))");
  fail_unless(TYPE(ret) == T_CONS);
  fail_unless(car(ret) == INT2FIX(2));

  TEST("(apply cadr '((1 2)))");
  fail_unless(ret == INT2FIX(2));

  TEST("(apply cddr '((1 2)))");
  fail_unless(NIL_P(ret));

  arc_bindsym(c, arc_intern_cstr(c, "asc"),
	      cons(c, INT2FIX(1), cons(c, INT2FIX(2), CNIL)));
  TEST("(apply scar `(,asc 3))");
  fail_unless(ret == INT2FIX(3));

  TEST("asc");
  fail_unless(TYPE(ret) == T_CONS);
  fail_unless(car(ret) == INT2FIX(3));
  fail_unless(cadr(ret) == INT2FIX(2));

  TEST("(apply scdr `(,asc 4))");
  fail_unless(ret == INT2FIX(4));
  TEST("asc");
  fail_unless(TYPE(ret) == T_CONS);
  fail_unless(car(ret) == INT2FIX(3));
  fail_unless(cdr(ret) == INT2FIX(4));
}
END_TEST

START_TEST(test_uniq)
{
  value thr, cctx, clos, code, ret, ret2;

  thr = arc_mkthread(c);
  TEST("(uniq)");
  fail_unless(TYPE(ret) == T_SYMBOL);
  ret2 = ret;
  TEST("(uniq)");
  fail_if(ret == ret2);
}
END_TEST

START_TEST(test_ccc)
{
  value thr, cctx, clos, code, ret;

  thr = arc_mkthread(c);
  TEST("(+ 1 (ccc (fn (c) (c 41) 43)))");
  fail_unless(ret == INT2FIX(42));
}
END_TEST

START_TEST(test_sref)
{
  value thr, cctx, clos, code, ret, val;

  thr = arc_mkthread(c);

  val = arc_mkhash(c, ARC_HASHBITS);
  arc_bindsym(c, arc_intern_cstr(c, "sreftest"), val);
  TEST("(sref sreftest 2 1)");
  fail_unless(ret == INT2FIX(2));
  fail_unless(arc_hash_lookup(c, val, INT2FIX(1)) == INT2FIX(2));

  val = arc_mkstringc(c, "abc");
  arc_bindsym(c, arc_intern_cstr(c, "sreftest"), val);
  TEST("(sref sreftest #\\z 1)");
  fail_unless(TYPE(ret) == T_CHAR);
  fail_unless(arc_char2rune(c, ret) == 'z');
  fail_unless(arc_strcmp(c, val, arc_mkstringc(c, "azc")) == 0);

  val = cons(c, INT2FIX(1), cons(c, INT2FIX(2), cons(c, INT2FIX(3), CNIL)));
  arc_bindsym(c, arc_intern_cstr(c, "sreftest"), val);
  TEST("(sref sreftest 31337 1)");
  fail_unless(ret == INT2FIX(31337));
  fail_unless(car(val) == INT2FIX(1));
  fail_unless(car(cdr(val)) == INT2FIX(31337));
  fail_unless(car(cdr(cdr(val))) == INT2FIX(3));

  val = arc_mkvector(c, 3);
  SVINDEX(val, 0, INT2FIX(1));
  SVINDEX(val, 1, INT2FIX(2));
  SVINDEX(val, 2, INT2FIX(3));
  arc_bindsym(c, arc_intern_cstr(c, "sreftest"), val);
  TEST("(sref sreftest 31337 1)");
  fail_unless(ret == INT2FIX(31337));
  fail_unless(VINDEX(val, 0) == INT2FIX(1));
  fail_unless(VINDEX(val, 1) == INT2FIX(31337));
  fail_unless(VINDEX(val, 2) == INT2FIX(3));
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
  tcase_add_test(tc_bif, test_sym);

  tcase_add_test(tc_bif, test_gt);
  tcase_add_test(tc_bif, test_lt);
  tcase_add_test(tc_bif, test_spaceship);
  tcase_add_test(tc_bif, test_bound);
  tcase_add_test(tc_bif, test_exact);
  tcase_add_test(tc_bif, test_is);
  tcase_add_test(tc_bif, test_iso);

  tcase_add_test(tc_bif, test_div);
  tcase_add_test(tc_bif, test_real);
  tcase_add_test(tc_bif, test_imag);
  tcase_add_test(tc_bif, test_conj);
  tcase_add_test(tc_bif, test_arg);
  tcase_add_test(tc_bif, test_expt);
  tcase_add_test(tc_bif, test_mod);
  tcase_add_test(tc_bif, test_rand_srand);
  tcase_add_test(tc_bif, test_sqrt);
  tcase_add_test(tc_bif, test_trunc);

  tcase_add_test(tc_bif, test_maptable);
  tcase_add_test(tc_bif, test_eval);
  tcase_add_test(tc_bif, test_ssyntax);
  tcase_add_test(tc_bif, test_ssexpand);
  tcase_add_test(tc_bif, test_apply);
  tcase_add_test(tc_bif, test_uniq);
  tcase_add_test(tc_bif, test_ccc);
  tcase_add_test(tc_bif, test_sref);

  suite_add_tcase(s, tc_bif);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

