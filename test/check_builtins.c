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
#include "../src/arith.h"
#include "../config.h"
#include "../src/vmengine.h"
#include "../src/symbols.h"

arc c, *cc;

static value test_builtin(const char *symname, int argc, ...)
{
  value sym, cctx, thr, func;;
  int contofs, base, i;
  va_list ap;

  cctx = arc_mkcctx(&c, INT2FIX(1), INT2FIX(1));
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
  func = arc_mkcode(&c, CCTX_VCODE(cctx), 1);
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
#ifdef HAVE_GMP_H
  value val;
#endif

  fail_unless(test_builtin("exact", 1, INT2FIX(31337)) == CTRUE);
#ifdef HAVE_GMP_H
  fail_unless(test_builtin("exact", 1, arc_mkrationall(&c, 1, 2)) == CTRUE);
#endif
  fail_unless(test_builtin("exact", 1, arc_mkflonum(&c, 3.14159)) == CNIL);
#ifdef HAVE_GMP_H
  val = test_builtin("expt", 2, INT2FIX(1024), INT2FIX(2));
  fail_unless(test_builtin("exact", 1, val) == CTRUE);
#endif

}
END_TEST

START_TEST(test_builtin_spaceship)
{
  fail_unless(test_builtin("<=>", 2, INT2FIX(31338), INT2FIX(31337)) == INT2FIX(-1));
  fail_unless(test_builtin("<=>", 2, INT2FIX(31337), INT2FIX(31338)) == INT2FIX(1));
  fail_unless(test_builtin("<=>", 2, INT2FIX(31337), INT2FIX(31337)) == INT2FIX(0));
}
END_TEST

START_TEST(test_builtin_fixnump)
{
  fail_unless(test_builtin("fixnump", 1, INT2FIX(31337)) == CTRUE);
  fail_unless(test_builtin("fixnump", 1, arc_mkflonum(&c, 3.14159)) == CNIL);
}
END_TEST

START_TEST(test_builtin_expt)
{
  value val;
#ifdef HAVE_GMP_H
  mpz_t expected;
#endif

#ifdef HAVE_GMP_H
  fail_unless(test_builtin("expt", 2, INT2FIX(24), INT2FIX(2)) == INT2FIX(16777216));
#else
  val = test_builtin("expt", 2, INT2FIX(24), INT2FIX(2));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(REP(val)._flonum - 16777216.0) < 1e-6);
#endif
  val = test_builtin("expt", 2, INT2FIX(24), arc_mkflonum(&c, 2));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(16777216.0 - REP(val)._flonum) < 1e-6);

  val = test_builtin("expt", 2, arc_mkflonum(&c, 24.0), INT2FIX(2));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(16777216.0 - REP(val)._flonum) < 1e-6);

  val = test_builtin("expt", 2, arc_mkflonum(&c, 24.0), arc_mkflonum(&c, 2.0));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(16777216.0 - REP(val)._flonum) < 1e-6);

  val = test_builtin("expt", 2, arc_mkflonum(&c, 0.5), arc_mkflonum(&c, -1));
  fail_unless(TYPE(val) == T_COMPLEX);
  fail_unless(fabs(REP(val)._complex.re) < 1e-6);
  fail_unless(fabs(1.0 - REP(val)._complex.im) < 1e-6);

  val = test_builtin("expt", 2, arc_mkcomplex(&c, 2.0, 2.0),
		     arc_mkcomplex(&c, 1.0, 1.0));
  fail_unless(TYPE(val) == T_COMPLEX);
  fail_unless(fabs(-0.26565399849241 - REP(val)._complex.re) < 1e-6);
  fail_unless(fabs(0.319818113856136 - REP(val)._complex.im) < 1e-6);

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

START_TEST(test_builtin_type)
{
  fail_unless(test_builtin("type", 1,
			   INT2FIX(100)) == arc_intern_cstr(&c, "fixnum"));
  fail_unless(test_builtin("type", 1, arc_mkflonum(&c, 3.1416))
	      == arc_intern_cstr(&c, "flonum"));
  fail_unless(test_builtin("type", 1, arc_mkcomplex(&c, 3.1416, 2.718))
	      == arc_intern_cstr(&c, "complex"));
  fail_unless(test_builtin("type", 1, arc_mkstringc(&c, "foo"))
	      == arc_intern_cstr(&c, "string"));
  fail_unless(test_builtin("type", 1, arc_intern_cstr(&c, "foo"))
	      == arc_intern_cstr(&c, "sym"));
  fail_unless(test_builtin("type", 1, cons(&c, INT2FIX(1), CNIL))
	      == arc_intern_cstr(&c, "cons"));
  fail_unless(test_builtin("type", 1, arc_mkvector(&c, 1))
	      == arc_intern_cstr(&c, "vector"));
}
END_TEST

START_TEST(test_builtin_coerce_int)
{
  value val;

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "int"),
		     arc_mkchar(&c, 0x86df));
  fail_unless(TYPE(val) == T_FIXNUM);
  fail_unless(val == INT2FIX(0x86df));

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "fixnum"),
		     arc_mkchar(&c, 0x86df));
  fail_unless(TYPE(val) == T_FIXNUM);
  fail_unless(val == INT2FIX(0x86df));

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "bignum"),
		     arc_mkchar(&c, 0x86df));
  fail_unless(TYPE(val) == T_FIXNUM);
  fail_unless(val == INT2FIX(0x86df));

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "int"),
		     arc_mkflonum(&c, 3.1416));
  fail_unless(TYPE(val) == T_FIXNUM);
  fail_unless(val == INT2FIX(3));
#ifdef HAVE_GMP_H
  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "int"),
		     arc_mkflonum(&c, 1.0e100));
  fail_unless(TYPE(val) == T_BIGNUM);
  /* Since mpz_set_d has unpredictable results in the lower order digits
     when converting a very large float to a bignum, we try to convert the
     result back to a double and do our comparisons of the logarithms of
     each. */
  {
    double d;

    d = mpz_get_d(REP(val)._bignum);
    fail_unless(fabs(log(d) - log(1.0e100)) < 1e-6);
  }
#else
  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "int"),
		     arc_mkflonum(&c, 1.0e100));
  fail_unless(TYPE(val) == T_FIXNUM);
  fail_unless(val == INT2FIX(FIXNUM_MAX));
#endif

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "int"),
		     arc_mkcomplex(&c, 3.1416, 2.718));
  fail_unless(TYPE(val) == T_FIXNUM);
  fail_unless(val == INT2FIX(3));

  val = test_builtin("coerce", 3, arc_intern_cstr(&c, "im"),
		     arc_intern_cstr(&c, "int"),
		     arc_mkcomplex(&c, 3.1416, 2.718));
  fail_unless(TYPE(val) == T_FIXNUM);
  fail_unless(val == INT2FIX(2));

  val = test_builtin("coerce", 3, arc_intern_cstr(&c, "re"),
		     arc_intern_cstr(&c, "int"),
		     arc_mkcomplex(&c, 3.1416, 2.718));
  fail_unless(TYPE(val) == T_FIXNUM);
  fail_unless(val == INT2FIX(3));

#ifdef HAVE_GMP_H
  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "int"),
		     arc_mkrationall(&c, 1, 2));
  fail_unless(TYPE(val) == T_FIXNUM);
  fail_unless(val == INT2FIX(0));

  {
    value v1, v2;
    mpz_t expected;

    v1 = arc_mkrationall(&c, 3, 2);
    v2 = arc_mkbignuml(&c, 0);
    mpz_set_str(REP(v2)._bignum, "100000000000000000000000000000", 10);
    val = __arc_add2(&c, v1, v2);
    val = test_builtin("coerce", 2, arc_intern_cstr(&c, "int"), val);
    fail_unless(TYPE(val) == T_BIGNUM);
    mpz_init(expected);
    mpz_set_str(expected, "100000000000000000000000000001", 10);
    fail_unless(mpz_cmp(expected, REP(val)._bignum) == 0);
    mpz_clear(expected);
  }

#endif
  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "int"),
		     arc_mkstringc(&c, "31337"));
  fail_unless(TYPE(val) == T_FIXNUM);
  fail_unless(val = INT2FIX(31337));

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "int"),
		     arc_mkstringc(&c, "-31337"));
  fail_unless(TYPE(val) == T_FIXNUM);
  fail_unless(val = INT2FIX(-31337));

  val = test_builtin("coerce", 3, INT2FIX(16), arc_intern_cstr(&c, "int"),
		     arc_mkstringc(&c, "1234abcd"));
  fail_unless(TYPE(val) == T_FIXNUM);
  fail_unless(val = INT2FIX(305441741));

  val = test_builtin("coerce", 3, INT2FIX(36), arc_intern_cstr(&c, "int"),
		     arc_mkstringc(&c, "1z2y"));
  fail_unless(TYPE(val) == T_FIXNUM);
  fail_unless(val = INT2FIX(2662));

#ifdef HAVE_GMP_H
  {
    mpz_t expected;
    val = test_builtin("coerce", 2, arc_intern_cstr(&c, "int"),
		       arc_mkstringc(&c, "100000000000000000000000000000"));
    fail_unless(TYPE(val) == T_BIGNUM);
    mpz_init(expected);
    mpz_set_str(expected, "100000000000000000000000000000", 10);
    fail_unless(mpz_cmp(expected, REP(val)._bignum) == 0);
    mpz_clear(expected);
  }
#endif
}
END_TEST

START_TEST(test_builtin_coerce_flonum)
{
  value val;

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "flonum"),
		     INT2FIX(31337));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(REP(val)._flonum - 31337.0) < 1e-6);

#ifdef HAVE_GMP_H
  val = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(val)._bignum, "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", 10);
  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "flonum"), val);
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(log(REP(val)._flonum) - log(1e100)) < 1e-6);

  val = arc_mkrationall(&c, 47627751, 15160384);
  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "flonum"), val);
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(REP(val)._flonum - 3.1415926535897771) < 1e-6);
#endif

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "flonum"),
		     arc_mkcomplex(&c, 3.1416, 2.718));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(REP(val)._flonum - 3.1416) < 1e-6);

  val = test_builtin("coerce", 3, arc_intern_cstr(&c, "im"),
		     arc_intern_cstr(&c, "flonum"),
		     arc_mkcomplex(&c, 3.1416, 2.718));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(REP(val)._flonum - 2.718) < 1e-6);

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "flonum"),
		     arc_mkstringc(&c, "3.1416"));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(REP(val)._flonum - 3.1416) < 1e-6);

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "flonum"),
		     arc_mkstringc(&c, "+3.1416"));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(REP(val)._flonum - 3.1416) < 1e-6);

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "flonum"),
		     arc_mkstringc(&c, "-3.1416"));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(REP(val)._flonum - -3.1416) < 1e-6);

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "flonum"),
		     arc_mkstringc(&c, "3.1416e2"));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(REP(val)._flonum - 3.1416e2) < 1e-6);

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "flonum"),
		     arc_mkstringc(&c, "31.416e2"));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(REP(val)._flonum - 31.416e2) < 1e-6);

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "flonum"),
		     arc_mkstringc(&c, "3.1416e-2"));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(REP(val)._flonum - 3.1416e-2) < 1e-6);

  val = test_builtin("coerce", 3, INT2FIX(8), arc_intern_cstr(&c, "flonum"),
		     arc_mkstringc(&c, "1.23e4"));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(REP(val)._flonum - 5312.0) < 1e-6);

  val = test_builtin("coerce", 3, INT2FIX(16), arc_intern_cstr(&c, "flonum"),
		     arc_mkstringc(&c, "1.23e4"));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(REP(val)._flonum - 1.14019775390625) < 1e-6);

  val = test_builtin("coerce", 3, INT2FIX(16), arc_intern_cstr(&c, "flonum"),
		     arc_mkstringc(&c, "1.23e4p2"));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(REP(val)._flonum - 291.890625) < 1e-6);

  val = test_builtin("coerce", 3, INT2FIX(16), arc_intern_cstr(&c, "flonum"),
		     arc_mkstringc(&c, "1.23e4&3"));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(REP(val)._flonum - 4670.25) < 1e-6);

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "flonum"),
		     arc_mkstringc(&c, "Inf"));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(isinf(REP(val)._flonum));

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "flonum"),
		     arc_mkstringc(&c, "-Inf"));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(isinf(REP(val)._flonum));

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "flonum"),
		     arc_mkstringc(&c, "NaN"));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(isnan(REP(val)._flonum));

}
END_TEST

START_TEST(test_builtin_coerce_rational)
{
#ifdef HAVE_GMP_H
  value val, val2;

  val2 = test_builtin("coerce", 2, arc_intern_cstr(&c, "rational"),
		      arc_mkstringc(&c, "1/2"));
  fail_unless(TYPE(val2) == T_RATIONAL);
  fail_unless(mpz_get_ui(mpq_numref(REP(val2)._rational)) == 1);
  fail_unless(mpz_get_ui(mpq_denref(REP(val2)._rational)) == 2);

  val = arc_mkflonum(&c, 0.5);
  val2 = test_builtin("coerce", 2, arc_intern_cstr(&c, "rational"), val);
  fail_unless(TYPE(val2) == T_RATIONAL);
  fail_unless(mpz_get_ui(mpq_numref(REP(val2)._rational)) == 1);
  fail_unless(mpz_get_ui(mpq_denref(REP(val2)._rational)) == 2);

  val = arc_mkcomplex(&c, 0.5, 1.5);
  val2 = test_builtin("coerce", 2, arc_intern_cstr(&c, "rational"), val);
  fail_unless(TYPE(val2) == T_RATIONAL);
  fail_unless(mpz_get_ui(mpq_numref(REP(val2)._rational)) == 1);
  fail_unless(mpz_get_ui(mpq_denref(REP(val2)._rational)) == 2);

  val2 = test_builtin("coerce", 3, arc_intern_cstr(&c, "im"),
		      arc_intern_cstr(&c, "rational"), val);
  fail_unless(TYPE(val2) == T_RATIONAL);
  fail_unless(mpz_get_ui(mpq_numref(REP(val2)._rational)) == 3);
  fail_unless(mpz_get_ui(mpq_denref(REP(val2)._rational)) == 2);

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "rational"),
		     INT2FIX(31337));
  fail_unless(TYPE(val) == T_FIXNUM);
  fail_unless(val == INT2FIX(31337));

  val = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(val)._bignum, "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", 10);
  val2 = test_builtin("coerce", 2, arc_intern_cstr(&c, "rational"), val);
  fail_unless(TYPE(val) == T_BIGNUM);
  fail_unless(mpz_cmp(REP(val2)._bignum, REP(val)._bignum) == 0);

  val = arc_mkrationall(&c, 1, 2);
  val2 = test_builtin("coerce", 2, arc_intern_cstr(&c, "rational"), val);
  fail_unless(TYPE(val2) == T_RATIONAL);
  fail_unless(mpq_cmp(REP(val2)._rational, REP(val)._rational) == 0);
#endif
}
END_TEST

START_TEST(test_builtin_coerce_complex)
{
  value val;

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "complex"),
		     arc_mkstringc(&c, "1e-3+2e+2i"));
  fail_unless(TYPE(val) == T_COMPLEX);
  fail_unless(fabs(REP(val)._complex.re - 1e-3) < 1e-6);
  fail_unless(fabs(REP(val)._complex.im - 2e+2) < 1e-6);

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "complex"),
		     arc_mkstringc(&c, "1-2e+2i"));
  fail_unless(TYPE(val) == T_COMPLEX);
  fail_unless(fabs(REP(val)._complex.re - 1.0) < 1e-6);
  fail_unless(fabs(REP(val)._complex.im - -2e+2) < 1e-6);

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "complex"),
		     arc_mkstringc(&c, "15p3-2&-2j"));
  fail_unless(TYPE(val) == T_COMPLEX);
  fail_unless(fabs(REP(val)._complex.re - 15.0e3) < 1e-6);
  fail_unless(fabs(REP(val)._complex.im - -2e-2) < 1e-6);

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "complex"),
		     cons(&c, INT2FIX(1), INT2FIX(2)));
  fail_unless(TYPE(val) == T_COMPLEX);
  fail_unless(fabs(REP(val)._complex.re - 1.0) < 1e-6);
  fail_unless(fabs(REP(val)._complex.im - 2.0) < 1e-6);
}
END_TEST

START_TEST(test_builtin_coerce_string)
{
  value val;
  char resstr[2000];		/* the longest string below is 308 digits */

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "string"),
		     arc_mkchar(&c, 0x86df));
  fail_unless(TYPE(val) == T_STRING);
  fail_unless(arc_strlen(&c, val) == 1);
  fail_unless(arc_strindex(&c, val, 0) == 0x86df);

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "string"),
		     INT2FIX(31337));
  fail_unless(TYPE(val) == T_STRING);
  arc_str2cstr(&c, val, resstr);
  fail_unless(strcmp(resstr, "31337") == 0);

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "string"),
		     INT2FIX(-31337));
  fail_unless(TYPE(val) == T_STRING);
  arc_str2cstr(&c, val, resstr);
  fail_unless(strcmp(resstr, "-31337") == 0);

  val = test_builtin("coerce", 3, INT2FIX(16), arc_intern_cstr(&c, "string"),
		     INT2FIX(0xdeadbeef));
  fail_unless(TYPE(val) == T_STRING);
  arc_str2cstr(&c, val, resstr);
  fail_unless(strcmp(resstr, "deadbeef") == 0);

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "string"),
		     arc_mkflonum(&c, 3.1416));
  fail_unless(TYPE(val) == T_STRING);
  arc_str2cstr(&c, val, resstr);
  fail_unless(strcmp(resstr, "3.141600") == 0);

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "string"),
		     arc_mkcomplex(&c, 3.1416, 2.718));
  fail_unless(TYPE(val) == T_STRING);
  arc_str2cstr(&c, val, resstr);
  fail_unless(strcmp(resstr, "3.141600+2.718000i") == 0);

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "string"),
		     arc_mkcomplex(&c, 3.1416, -2.718));
  fail_unless(TYPE(val) == T_STRING);
  arc_str2cstr(&c, val, resstr);
  fail_unless(strcmp(resstr, "3.141600-2.718000i") == 0);

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "string"),
		     CNIL);
  fail_unless(TYPE(val) == T_STRING);
  arc_str2cstr(&c, val, resstr);
  fail_unless(strcmp(resstr, "") == 0);

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "string"),
		     cons(&c, arc_mkchar(&c, 'a'),
			  cons(&c, arc_mkchar(&c, 'b'),
			       cons(&c, arc_mkchar(&c, 'c'), CNIL))));
  fail_unless(TYPE(val) == T_STRING);
  arc_str2cstr(&c, val, resstr);
  fail_unless(strcmp(resstr, "abc") == 0);

  {
    value vec;

    vec = arc_mkvector(&c, 3);
    VINDEX(vec, 0) = arc_mkchar(&c, 'd');
    VINDEX(vec, 1) = arc_mkchar(&c, 'e');
    VINDEX(vec, 2) = arc_mkchar(&c, 'f');
    val = test_builtin("coerce", 2, arc_intern_cstr(&c, "string"), vec);
    fail_unless(TYPE(val) == T_STRING);
    arc_str2cstr(&c, val, resstr);
    fail_unless(strcmp(resstr, "def") == 0);

  }

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "string"),
		     arc_intern_cstr(&c, "foofoofoo"));
  fail_unless(TYPE(val) == T_STRING);
  arc_str2cstr(&c, val, resstr);
  fail_unless(strcmp(resstr, "foofoofoo") == 0);

#ifdef HAVE_GMP_H
  val = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(val)._bignum, "17458190341957875493918951399970107227965724009652985941133472959768677340335045964003708354964206045426488855242563656549160032113803576330213012581783662123886415397466019364605427913064340532095949475555876255472947307998020783622905682728630226511251088930521948328443759353461831308036227741295153300603891190455306190345420279908434822902564881835220799113628636704083693542560621786200579311197838701034882688524418388050219583335812792801046560882997965297536760818057792378037613537963856521225603672879430981029134303487133386612767444769978830382787211137708766342100698288963099819638229368253960521425399", 10);
  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "string"), val);
  arc_str2cstr(&c, val, resstr);
  fail_unless(strcmp(resstr, "17458190341957875493918951399970107227965724009652985941133472959768677340335045964003708354964206045426488855242563656549160032113803576330213012581783662123886415397466019364605427913064340532095949475555876255472947307998020783622905682728630226511251088930521948328443759353461831308036227741295153300603891190455306190345420279908434822902564881835220799113628636704083693542560621786200579311197838701034882688524418388050219583335812792801046560882997965297536760818057792378037613537963856521225603672879430981029134303487133386612767444769978830382787211137708766342100698288963099819638229368253960521425399") == 0);

  val = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(val)._bignum, "17458190341957875493918951399970107227965724009652985941133472959768677340335045964003708354964206045426488855242563656549160032113803576330213012581783662123886415397466019364605427913064340532095949475555876255472947307998020783622905682728630226511251088930521948328443759353461831308036227741295153300603891190455306190345420279908434822902564881835220799113628636704083693542560621786200579311197838701034882688524418388050219583335812792801046560882997965297536760818057792378037613537963856521225603672879430981029134303487133386612767444769978830382787211137708766342100698288963099819638229368253960521425399", 10);
  val = test_builtin("coerce", 3, INT2FIX(16),
		     arc_intern_cstr(&c, "string"), val);
  arc_str2cstr(&c, val, resstr);
  fail_unless(strcmp(resstr, "8a4ba652a2cd4f8a3cfb3c2eb9ee0037e09237037f0898b62c6c23ea6170f42ed3cc477486c551f95d0a9df9cefe7b0febc0fab61e040f30668eb1fc46621eb0318225377f8ccdb427ef3e31cd80df9de9175a7805d78a69047280b0e9b57dbcab1b974a8b426a6ba5797da56a2e1cbef32fcea395ad097cfc496476102da987f4b4671a84c926525677cc604448fb0c0e3cefea0902cdb2846ce54a713826475354d52e621428291e8912b7cbef5a24ffbba5b829d7aad12dde62e8d2b7bad7232f96d2bd02c5857b75487af0f6cb7a8c9d18584f7935209a20f8939cff79f88dc9763e337165f1cc37e34d539295cedf059b02ea39290a0b7e39b2c236a1f7") == 0);

  val = arc_mkrationall(&c, 1, 2);
  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "string"), val);
  arc_str2cstr(&c, val, resstr);
  fail_unless(strcmp(resstr, "1/2") == 0);

  val = arc_mkrationall(&c, 1, 2);
  mpq_set_str(REP(val)._rational, "33432311598195032051244152090146991671992470962663769691565787342862839974874/316026798634956773167298493198184647803", 10);
  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "string"), val);
  arc_str2cstr(&c, val, resstr);
  fail_unless(strcmp(resstr, "33432311598195032051244152090146991671992470962663769691565787342862839974874/316026798634956773167298493198184647803") == 0);
#endif
}
END_TEST

START_TEST(test_builtin_coerce_cons)
{
  value val;

  val = arc_mkcomplex(&c, 3.1416, 2.718);
  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "cons"), val);
  fail_unless(TYPE(val) == T_CONS);
  fail_unless(fabs(REP(car(val))._flonum - 3.1416) < 1e-6);
  fail_unless(fabs(REP(cdr(val))._flonum - 2.718) < 1e-6);

  val = arc_mkvector(&c, 3);
  VINDEX(val, 0) = INT2FIX(1);
  VINDEX(val, 1) = INT2FIX(2);
  VINDEX(val, 2) = INT2FIX(3);
  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "cons"), val);
  fail_unless(TYPE(val) == T_CONS);
  fail_unless(car(val) == INT2FIX(1));
  fail_unless(car(cdr(val)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(val))) == INT2FIX(3));
  fail_unless(cdr(cdr(cdr(val))) == CNIL);

  val = arc_mkstringc(&c, "abc");
  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "cons"), val);
  fail_unless(TYPE(val) == T_CONS);
  fail_unless(TYPE(car(val)) == T_CHAR);
  fail_unless(REP(car(val))._char == 'a');
  fail_unless(TYPE(car(cdr(val))) == T_CHAR);
  fail_unless(REP(car(cdr(val)))._char == 'b');
  fail_unless(TYPE(car(cdr(cdr(val)))) == T_CHAR);
  fail_unless(REP(car(cdr(cdr(val))))._char == 'c');

  fail_unless(cdr(cdr(cdr(val))) == CNIL);

#ifdef HAVE_GMP_H
  {
    value expect;

    val = arc_mkrationall(&c, 1, 2);
    val = test_builtin("coerce", 2, arc_intern_cstr(&c, "cons"), val);
    fail_unless(TYPE(val) == T_CONS);
    fail_unless(car(val) == INT2FIX(1));
    fail_unless(cdr(val) == INT2FIX(2));

    val = arc_mkrationall(&c, 1, 2);
    mpq_set_str(REP(val)._rational, "33432311598195032051244152090146991671992470962663769691565787342862839974874/316026798634956773167298493198184647803", 10);
    val = test_builtin("coerce", 2, arc_intern_cstr(&c, "cons"), val);
    fail_unless(TYPE(val) == T_CONS);
    fail_unless(TYPE(car(val)) == T_BIGNUM);
    fail_unless(TYPE(cdr(val)) == T_BIGNUM);
    expect = arc_mkbignuml(&c, 0);
    mpz_set_str(REP(expect)._bignum, "33432311598195032051244152090146991671992470962663769691565787342862839974874", 10);
    fail_unless(arc_cmp(&c, car(val), expect) == INT2FIX(0));
    mpz_set_str(REP(expect)._bignum, "316026798634956773167298493198184647803", 10);
    fail_unless(arc_cmp(&c, cdr(val), expect) == INT2FIX(0));

  }
#endif
}
END_TEST

START_TEST(test_builtin_coerce_sym)
{
  value val;

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "sym"),
		     arc_mkchar(&c, 'a'));
  fail_unless(TYPE(val) == T_SYMBOL);
  fail_unless(val == arc_intern_cstr(&c, "a"));

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "sym"),
		     arc_mkstringc(&c, "foo"));
  fail_unless(TYPE(val) == T_SYMBOL);
  fail_unless(val == arc_intern_cstr(&c, "foo"));

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "sym"),
		     arc_mkstringc(&c, "nil"));
  fail_unless(TYPE(val) == T_NIL);
  fail_unless(val == CNIL);

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "sym"),
		     arc_mkstringc(&c, "t"));
  fail_unless(TYPE(val) == T_TRUE);
  fail_unless(val == CTRUE);

}
END_TEST

START_TEST(test_builtin_coerce_vector)
{
  value val;

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "vector"),
		     arc_mkstringc(&c, "abc"));
  fail_unless(TYPE(val) == T_VECTOR);
  fail_unless(VECLEN(val) == 3);
  fail_unless(TYPE(VINDEX(val, 0)) == T_CHAR);
  fail_unless(REP(VINDEX(val, 0))._char == 'a');
  fail_unless(TYPE(VINDEX(val, 1)) == T_CHAR);
  fail_unless(REP(VINDEX(val, 1))._char == 'b');
  fail_unless(TYPE(VINDEX(val, 2)) == T_CHAR);
  fail_unless(REP(VINDEX(val, 2))._char == 'c');

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "vector"),
		     cons(&c, INT2FIX(1),
			  cons(&c, INT2FIX(2),
			       cons(&c, INT2FIX(3), CNIL))));
  fail_unless(TYPE(val) == T_VECTOR);
  fail_unless(VECLEN(val) == 3);
  fail_unless(TYPE(VINDEX(val, 0)) == T_FIXNUM);
  fail_unless(VINDEX(val, 0) == INT2FIX(1));
  fail_unless(TYPE(VINDEX(val, 1)) == T_FIXNUM);
  fail_unless(VINDEX(val, 1) == INT2FIX(2));
  fail_unless(TYPE(VINDEX(val, 2)) == T_FIXNUM);
  fail_unless(VINDEX(val, 2) == INT2FIX(3));
}
END_TEST

START_TEST(test_builtin_coerce_char)
{
  value val;

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "char"),
		     INT2FIX(0x86df));
  fail_unless(TYPE(val) == T_CHAR);
  fail_unless(REP(val)._char == 0x86df);
}
END_TEST

START_TEST(test_builtin_coerce_num)
{
  value val;

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "num"),
		     arc_mkstringc(&c, "3.1416-2.718i"));
  fail_unless(TYPE(val) == T_COMPLEX);
  fail_unless(fabs(REP(val)._complex.re - 3.1416) < 1e-6);
  fail_unless(fabs(REP(val)._complex.im - -2.718) < 1e-6);

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "num"),
		     arc_mkstringc(&c, "-3.1416+2.718j"));
  fail_unless(TYPE(val) == T_COMPLEX);
  fail_unless(fabs(REP(val)._complex.re - -3.1416) < 1e-6);
  fail_unless(fabs(REP(val)._complex.im - 2.718) < 1e-6);

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "num"),
		     arc_mkstringc(&c, "-3.1416"));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(REP(val)._flonum - -3.1416) < 1e-6);

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "num"),
		     arc_mkstringc(&c, "1e6"));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(REP(val)._flonum - 1e6) < 1e-6);

  /* base overrides the exponential identification */
  val = test_builtin("coerce", 3, INT2FIX(16), arc_intern_cstr(&c, "num"),
		     arc_mkstringc(&c, "1e6"));
  fail_unless(TYPE(val) == T_FIXNUM);
  fail_unless(val == INT2FIX(0x1e6));

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "num"),
		     arc_mkstringc(&c, "1p6"));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(REP(val)._flonum - 1e6) < 1e-6);

  /* base overrides the exponential identification */
  val = test_builtin("coerce", 3, INT2FIX(36), arc_intern_cstr(&c, "num"),
		     arc_mkstringc(&c, "1p6"));
  fail_unless(TYPE(val) == T_FIXNUM);
  fail_unless(val == INT2FIX(2202));

  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "num"),
		     arc_mkstringc(&c, "1&6"));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(REP(val)._flonum - 1e6) < 1e-6);

#ifdef HAVE_GMP_H
  val = test_builtin("coerce", 2, arc_intern_cstr(&c, "num"),
		     arc_mkstringc(&c, "1/2"));
  fail_unless(TYPE(val) == T_RATIONAL);
  fail_unless(mpz_get_ui(mpq_numref(REP(val)._rational)) == 1);
  fail_unless(mpz_get_ui(mpq_denref(REP(val)._rational)) == 2);

  {
    mpz_t expected;
    val = test_builtin("coerce", 2, arc_intern_cstr(&c, "num"),
		       arc_mkstringc(&c, "39174440345711397351903379231183395006881508225168016033485596806965668391752"));
    fail_unless(TYPE(val) == T_BIGNUM);
    mpz_init(expected);
    mpz_set_str(expected, "39174440345711397351903379231183395006881508225168016033485596806965668391752", 10);
    fail_unless(mpz_cmp(expected, REP(val)._bignum) == 0);
    mpz_clear(expected);
  }

#endif

}
END_TEST

/* Tests for annotate, type, and rep */
START_TEST(test_builtin_atr)
{
  value val, tr;

  val = test_builtin("annotate", 2, INT2FIX(31337),
		     arc_intern_cstr(&c, "foo"));
  fail_unless(TYPE(val) == T_TAGGED);

  tr = test_builtin("type", 1, val);
  fail_unless(tr == arc_intern_cstr(&c, "foo"));

  tr = test_builtin("rep", 1, val);
  fail_unless(tr == INT2FIX(31337));
}
END_TEST

START_TEST(test_builtin_abs)
{
  value val;
#ifdef HAVE_GMP_H
  mpz_t expected;
#endif

  fail_unless(test_builtin("abs", 1, INT2FIX(1)) == INT2FIX(1));
  fail_unless(test_builtin("abs", 1, INT2FIX(-1)) == INT2FIX(1));

  val = test_builtin("abs", 1, arc_mkflonum(&c, 1.0));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(REP(val)._flonum - 1.0) < 1e-6);

  val = test_builtin("abs", 1, arc_mkflonum(&c, -1.0));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(REP(val)._flonum - 1.0) < 1e-6);

  val = test_builtin("abs", 1, arc_mkcomplex(&c, 2.0, 2.0));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(REP(val)._flonum - 2.82842712474619) < 1e-6);

#ifdef HAVE_GMP_H
  mpz_init(expected);
  mpz_set_str(expected, "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", 10);
  val = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(val)._bignum, "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", 10);
  val = test_builtin("abs", 1, val);
  fail_unless(TYPE(val) == T_BIGNUM);
  fail_unless(mpz_cmp(expected, REP(val)._bignum) == 0);

  mpz_set_str(REP(val)._bignum, "-10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", 10);
  val = test_builtin("abs", 1, val);
  fail_unless(TYPE(val) == T_BIGNUM);
  fail_unless(mpz_cmp(expected, REP(val)._bignum) == 0);
  mpz_clear(expected);

  val = arc_mkrationall(&c, 1, 2);
  val = test_builtin("abs", 1, val);
  fail_unless(TYPE(val) == T_RATIONAL);
  fail_unless(mpq_cmp_ui(REP(val)._rational, 1, 2) == 0);

  val = arc_mkrationall(&c, -1, 2);
  val = test_builtin("abs", 1, val);
  fail_unless(TYPE(val) == T_RATIONAL);
  fail_unless(mpq_cmp_ui(REP(val)._rational, 1, 2) == 0);

#endif

}
END_TEST

START_TEST(test_builtin_pow)
{
  value val;
#ifdef HAVE_GMP_H
  mpz_t expected;
#endif

#ifdef HAVE_GMP_H
  fail_unless(test_builtin("pow", 2, INT2FIX(24), INT2FIX(2)) == INT2FIX(16777216));
#else
  val = test_builtin("pow", 2, INT2FIX(24), INT2FIX(2));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(REP(val)._flonum - 16777216.0) < 1e-6);
#endif

  val = test_builtin("pow", 2, INT2FIX(24), arc_mkflonum(&c, 2));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(16777216.0 - REP(val)._flonum) < 1e-6);

  val = test_builtin("pow", 2, arc_mkflonum(&c, 24.0), INT2FIX(2));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(16777216.0 - REP(val)._flonum) < 1e-6);

  val = test_builtin("pow", 2, arc_mkflonum(&c, 24.0), arc_mkflonum(&c, 2.0));
  fail_unless(TYPE(val) == T_FLONUM);
  fail_unless(fabs(16777216.0 - REP(val)._flonum) < 1e-6);

  val = test_builtin("pow", 2, arc_mkflonum(&c, 0.5), arc_mkflonum(&c, -1));
  fail_unless(TYPE(val) == T_COMPLEX);
  fail_unless(fabs(REP(val)._complex.re) < 1e-6);
  fail_unless(fabs(1.0 - REP(val)._complex.im) < 1e-6);

  val = test_builtin("pow", 2, arc_mkcomplex(&c, 2.0, 2.0),
		     arc_mkcomplex(&c, 1.0, 1.0));
  fail_unless(TYPE(val) == T_COMPLEX);
  fail_unless(fabs(-0.26565399849241 - REP(val)._complex.re) < 1e-6);
  fail_unless(fabs(0.319818113856136 - REP(val)._complex.im) < 1e-6);

#ifdef HAVE_GMP_H
  val = test_builtin("pow", 2, INT2FIX(1024), INT2FIX(2));
  fail_unless(TYPE(val) == T_BIGNUM);
  mpz_init(expected);
  mpz_set_str(expected, "179769313486231590772930519078902473361797697894230657273430081157732675805500963132708477322407536021120113879871393357658789768814416622492847430639474124377767893424865485276302219601246094119453082952085005768838150682342462881473913110540827237163350510684586298239947245938479716304835356329624224137216", 10);
  fail_unless(mpz_cmp(expected, REP(val)._bignum) == 0);

  val = test_builtin("pow", 2, INT2FIX(2), val);
  fail_unless(TYPE(val) == T_BIGNUM);
  mpz_set_str(expected, "32317006071311007300714876688669951960444102669715484032130345427524655138867890893197201411522913463688717960921898019494119559150490921095088152386448283120630877367300996091750197750389652106796057638384067568276792218642619756161838094338476170470581645852036305042887575891541065808607552399123930385521914333389668342420684974786564569494856176035326322058077805659331026192708460314150258592864177116725943603718461857357598351152301645904403697613233287231227125684710820209725157101726931323469678542580656697935045997268352998638215525166389437335543602135433229604645318478604952148193555853611059596230656", 10);
  fail_unless(mpz_cmp(expected, REP(val)._bignum) == 0);

  mpz_clear(expected);
#endif
}
END_TEST

START_TEST(test_builtin_mod)
{
  /* We don't need to test this one too exhaustively */
  fail_unless(test_builtin("mod", 2, INT2FIX(12), INT2FIX(14)) == INT2FIX(2));

}
END_TEST

START_TEST(test_builtin_idiv)
{
  /* We don't need to test this one too exhaustively */
  fail_unless(test_builtin("idiv", 2, INT2FIX(12), INT2FIX(14)) == INT2FIX(1));
}
END_TEST

START_TEST(test_builtin_rand)
{
  value r;

  /* Maybe we ought to make more meaningful tests... */
  fail_unless(test_builtin("srand", 1, INT2FIX(31337)) == INT2FIX(31337));
  fail_unless(TYPE(test_builtin("rand", 0)) == T_FLONUM);
  r = test_builtin("rand", 1, INT2FIX(65536));
  fail_unless(TYPE(r) == T_FIXNUM);
  fail_unless(r == INT2FIX(17800));
  r = test_builtin("rand", 1, INT2FIX(65536));
  fail_unless(r == INT2FIX(36822));
  r = test_builtin("rand", 1, INT2FIX(65536));
  fail_unless(r == INT2FIX(36591));
  r = test_builtin("rand", 1, INT2FIX(65536));
  fail_unless(r == INT2FIX(41661));
  r = test_builtin("rand", 1, INT2FIX(65536));
  fail_unless(r == INT2FIX(44925));
  r = test_builtin("rand", 1, INT2FIX(65536));
  fail_unless(r == INT2FIX(55506));
  r = test_builtin("rand", 1, INT2FIX(65536));
  fail_unless(r == INT2FIX(1287));
}
END_TEST

static char *names[] = {
  "Arcueid Brunestud",
  "Tohno Shiki",
  "Ciel",
  "Tohno Akiha",
  "Hisui",
  "Kohaku",
  "Aozaki Aoko",
  "Inui Arihiko",
  "Michael Roa Valdamjong",
  "Nero Chaos",
  "Yumizuka Satsuki"
};

START_TEST(test_builtin_table)
{
  value v, v2, sym, func, clos;
  value cctx;
  int i;

  v=test_builtin("table", 0);
  fail_unless(TYPE(v) == T_TABLE);
  for (i=0; i<11; i++) {
    v2 = arc_mkstringc(&c, names[i]);
    arc_hash_insert(&c, v, v2, INT2FIX(i));
  }

  /* Testing maptable is a little tricky.  We need to create a
     function that updates a global variable with the count. */
  sym = arc_intern_cstr(&c, "count");
  arc_hash_insert(&c, c.genv, sym, INT2FIX(0));
  cctx = arc_mkcctx(&c, INT2FIX(1), INT2FIX(1));
  VINDEX(CCTX_LITS(cctx), 0) = sym;
  arc_gcode2(&c, cctx, ienv, 2, -1);
  arc_gcode1(&c, cctx, imvarg, 0);
  arc_gcode1(&c, cctx, imvarg, 1);
  arc_gcode1(&c, cctx, ildg, 0);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(1));
  arc_gcode(&c, cctx, iadd);
  arc_gcode1(&c, cctx, istg, 0);
  arc_gcode(&c, cctx, iret);
  func = arc_mkcode(&c, CCTX_VCODE(cctx), 0);
  CODE_LITERAL(func, 0) = VINDEX(CCTX_LITS(cctx), 0);
  clos = arc_mkclosure(&c, func, CNIL);
  v=test_builtin("maptable", 2, v, clos);
  fail_unless(TYPE(v) == T_TABLE);
  fail_unless(arc_hash_lookup(&c, c.genv, sym) == INT2FIX(11));
}
END_TEST

START_TEST(test_symbols)
{
  fail_unless(arc_hash_lookup(&c, c.genv, arc_intern_cstr(&c, "nil")) == CNIL);
  fail_unless(arc_hash_lookup(&c, c.genv, arc_intern_cstr(&c, "t")) == CTRUE);
  fail_unless(TYPE(arc_hash_lookup(&c, c.genv,
				   arc_intern_cstr(&c, "sig"))) == T_TABLE);
}
END_TEST

START_TEST(test_ssyntax)
{
  value sexpr;

  fail_unless(test_builtin("ssyntax", 1, arc_intern_cstr(&c, "x:~y:z")) == CTRUE);
  fail_unless(test_builtin("ssyntax", 1, arc_intern_cstr(&c, "x&y&z")) == CTRUE);
  fail_unless(test_builtin("ssyntax", 1, arc_intern_cstr(&c, "+.1.2")) == CTRUE);
  fail_unless(test_builtin("ssyntax", 1, arc_intern_cstr(&c, "cons!a!b")) == CTRUE);
  fail_unless(test_builtin("ssyntax", 1, arc_intern_cstr(&c, "foo")) == CNIL);
  fail_unless(test_builtin("ssyntax", 1, arc_intern_cstr(&c, "+")) == CNIL);
  fail_unless(test_builtin("ssyntax", 1, arc_intern_cstr(&c, "++")) == CNIL);
  fail_unless(test_builtin("ssyntax", 1, arc_intern_cstr(&c, "_")) == CNIL);

  sexpr = test_builtin("ssexpand", 1, arc_intern_cstr(&c, "~"));
  fail_unless(TYPE(sexpr) == T_SYMBOL);
  fail_unless(sexpr == ARC_BUILTIN(cc, S_NO));

  sexpr = test_builtin("ssexpand", 1, arc_intern_cstr(&c, "a:"));
  fail_unless(TYPE(sexpr) == T_SYMBOL);
  fail_unless(sexpr == arc_intern(&c, arc_mkstringc(&c, "a")));

  sexpr = test_builtin("ssexpand", 1, arc_intern_cstr(&c, ":a"));
  fail_unless(TYPE(sexpr) == T_SYMBOL);
  fail_unless(sexpr == arc_intern(&c, arc_mkstringc(&c, "a")));

  sexpr = test_builtin("ssexpand", 1, arc_intern_cstr(&c, "a:b"));
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(car(sexpr) == ARC_BUILTIN(cc, S_COMPOSE));
  fail_unless(TYPE(car(cdr(sexpr))) == T_SYMBOL);
  fail_unless(car(cdr(sexpr)) == arc_intern_cstr(&c, "a"));
  fail_unless(TYPE(car(cdr(cdr(sexpr)))) == T_SYMBOL);
  fail_unless(car(cdr(cdr(sexpr))) == arc_intern_cstr(&c, "b"));

  sexpr = test_builtin("ssexpand", 1, arc_intern_cstr(&c, "a:b:c"));
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(car(sexpr) == ARC_BUILTIN(cc, S_COMPOSE));
  fail_unless(TYPE(car(cdr(sexpr))) == T_SYMBOL);
  fail_unless(car(cdr(sexpr)) == arc_intern(&c, arc_mkstringc(&c, "a")));
  fail_unless(TYPE(car(cdr(cdr(sexpr)))) == T_SYMBOL);
  fail_unless(car(cdr(cdr(sexpr))) == arc_intern(&c, arc_mkstringc(&c, "b")));
  fail_unless(TYPE(car(cdr(cdr(cdr(sexpr))))) == T_SYMBOL);
  fail_unless(car(cdr(cdr(cdr(sexpr)))) == arc_intern(&c, arc_mkstringc(&c, "c")));

  sexpr = test_builtin("ssexpand", 1, arc_intern_cstr(&c, "~a"));
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(car(sexpr) == ARC_BUILTIN(cc, S_COMPLEMENT));
  fail_unless(TYPE(car(cdr(sexpr))) == T_SYMBOL);
  fail_unless(car(cdr(sexpr)) == arc_intern(&c, arc_mkstringc(&c, "a")));

  sexpr = test_builtin("ssexpand", 1, arc_intern_cstr(&c, "a:~b:c"));
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(car(sexpr) == ARC_BUILTIN(cc, S_COMPOSE));
  fail_unless(TYPE(car(cdr(sexpr))) == T_SYMBOL);
  fail_unless(car(cdr(sexpr)) == arc_intern(&c, arc_mkstringc(&c, "a")));
  fail_unless(TYPE(car(cdr(cdr(sexpr)))) == T_CONS);
  fail_unless(car(car(cdr(cdr(sexpr)))) == ARC_BUILTIN(cc, S_COMPLEMENT));
  fail_unless(TYPE(car(cdr(car(cdr(cdr(sexpr)))))) == T_SYMBOL);
  fail_unless(car(cdr(car(cdr(cdr(sexpr))))) == arc_intern(&c, arc_mkstringc(&c, "b")));
  fail_unless(TYPE(car(cdr(cdr(cdr(sexpr))))) == T_SYMBOL);
  fail_unless(car(cdr(cdr(cdr(sexpr)))) == arc_intern(&c, arc_mkstringc(&c, "c")));

  sexpr = test_builtin("ssexpand", 1, arc_intern_cstr(&c, "a&"));
  fail_unless(TYPE(sexpr) == T_SYMBOL);
  fail_unless(sexpr == arc_intern(&c, arc_mkstringc(&c, "a")));

  sexpr = test_builtin("ssexpand", 1, arc_intern_cstr(&c, "&a"));
  fail_unless(TYPE(sexpr) == T_SYMBOL);
  fail_unless(sexpr == arc_intern(&c, arc_mkstringc(&c, "a")));

  sexpr = test_builtin("ssexpand", 1, arc_intern_cstr(&c, "a&b"));
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(car(sexpr) == ARC_BUILTIN(cc, S_ANDF));
  fail_unless(TYPE(car(cdr(sexpr))) == T_SYMBOL);
  fail_unless(car(cdr(sexpr)) == arc_intern(&c, arc_mkstringc(&c, "a")));
  fail_unless(TYPE(car(cdr(cdr(sexpr)))) == T_SYMBOL);
  fail_unless(car(cdr(cdr(sexpr))) == arc_intern(&c, arc_mkstringc(&c, "b")));

  sexpr = test_builtin("ssexpand", 1, arc_intern_cstr(&c, "a&b&c"));
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(car(sexpr) == ARC_BUILTIN(cc, S_ANDF));
  fail_unless(TYPE(car(cdr(sexpr))) == T_SYMBOL);
  fail_unless(car(cdr(sexpr)) == arc_intern(&c, arc_mkstringc(&c, "a")));
  fail_unless(TYPE(car(cdr(cdr(sexpr)))) == T_SYMBOL);
  fail_unless(car(cdr(cdr(sexpr))) == arc_intern(&c, arc_mkstringc(&c, "b")));
  fail_unless(TYPE(car(cdr(cdr(cdr(sexpr))))) == T_SYMBOL);
  fail_unless(car(cdr(cdr(cdr(sexpr)))) == arc_intern(&c, arc_mkstringc(&c, "c")));

  sexpr = test_builtin("ssexpand", 1, arc_intern_cstr(&c, "a.b"));
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(TYPE(car(sexpr)) == T_SYMBOL);
  fail_unless(car(sexpr) == arc_intern(&c, arc_mkstringc(&c, "a")));
  fail_unless(car(cdr(sexpr)) == arc_intern(&c, arc_mkstringc(&c, "b")));

  sexpr = test_builtin("ssexpand", 1, arc_intern_cstr(&c, "a.b.c"));
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(TYPE(car(sexpr)) == T_CONS);
  fail_unless(car(car(sexpr)) == arc_intern(&c, arc_mkstringc(&c, "a")));
  fail_unless(car(cdr(car(sexpr))) == arc_intern(&c, arc_mkstringc(&c, "b")));
  fail_unless(car(cdr(sexpr)) == arc_intern(&c, arc_mkstringc(&c, "c")));

  sexpr = test_builtin("ssexpand", 1, arc_intern_cstr(&c, ".a"));
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(TYPE(car(sexpr)) == T_SYMBOL);
  fail_unless(car(sexpr) == ARC_BUILTIN(cc, S_GET));
  fail_unless(car(cdr(sexpr)) == arc_intern(&c, arc_mkstringc(&c, "a")));

  sexpr = test_builtin("ssexpand", 1, arc_intern_cstr(&c, "a!b"));
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(TYPE(car(sexpr)) == T_SYMBOL);
  fail_unless(car(sexpr) == arc_intern(&c, arc_mkstringc(&c, "a")));
  fail_unless(car(cdr(sexpr)) == ARC_BUILTIN(cc, S_QUOTE));
  fail_unless(car(cdr(cdr(sexpr))) == arc_intern(&c, arc_mkstringc(&c, "b")));

  sexpr = test_builtin("ssexpand", 1, arc_intern_cstr(&c, "!a"));
  fail_unless(TYPE(sexpr) == T_CONS);
  fail_unless(TYPE(car(sexpr)) == T_SYMBOL);
  fail_unless(car(sexpr) == ARC_BUILTIN(cc, S_GET));
  fail_unless(car(cdr(sexpr)) == ARC_BUILTIN(cc, S_QUOTE));
  fail_unless(car(cdr(cdr(sexpr))) == arc_intern(&c, arc_mkstringc(&c, "a")));
}
END_TEST

START_TEST(test_builtin_uniq)
{
  value v, v2;

  v = test_builtin("uniq", 0);
  fail_unless(TYPE(v) == T_SYMBOL);
  v2 = test_builtin("uniq", 0);
  fail_unless(v != v2);
}
END_TEST

START_TEST(test_builtin_times)
{
  value v;

  /* Just call the functions and make sure they don't make any errors.
     We have no real way to independenly validate the values they
     produce. */
  fail_unless((v=test_builtin("current-gc-milliseconds", 0)) != CNIL);
  fail_unless((v=test_builtin("current-process-milliseconds", 0)) != CNIL);
  fail_unless((v=test_builtin("seconds", 0)) != CNIL);
  fail_unless((v=test_builtin("msec", 0)) != CNIL);
}
END_TEST

START_TEST(test_builtin_sref)
{
  value val, val2;
  char str[64];

  val2 = arc_mkhash(&c, 8);
  val = test_builtin("sref", 3, INT2FIX(1), INT2FIX(2), val2);
  fail_unless(val == INT2FIX(2));
  fail_unless(arc_hash_lookup(&c, val2, INT2FIX(1)) == INT2FIX(2));

  val2 = arc_mkstringc(&c, "abc");
  val = test_builtin("sref", 3, INT2FIX(1), arc_mkchar(&c, 'z'), val2);
  fail_unless(TYPE(val) == T_CHAR);
  fail_unless(REP(val)._char == 'z');
  arc_str2cstr(&c, val2, str);
  fail_unless(strcmp(str, "azc") == 0);

  val2 = cons(&c, INT2FIX(1), cons(&c, INT2FIX(2), cons(&c, INT2FIX(3), CNIL)));
  val = test_builtin("sref", 3, INT2FIX(1), INT2FIX(31337), val2);
  fail_unless(val == INT2FIX(31337));
  fail_unless(car(val2) == INT2FIX(1));
  fail_unless(car(cdr(val2)) == INT2FIX(31337));
  fail_unless(car(cdr(cdr(val2))) == INT2FIX(3));

  val2 = arc_mkvector(&c, 3);
  VINDEX(val2, 0) = INT2FIX(1);
  VINDEX(val2, 1) = INT2FIX(2);
  VINDEX(val2, 2) = INT2FIX(3);
  val = test_builtin("sref", 3, INT2FIX(1), INT2FIX(31337), val2);
  fail_unless(val == INT2FIX(31337));
  fail_unless(VINDEX(val2, 1) == INT2FIX(31337));
}
END_TEST

START_TEST(test_builtin_len)
{
  value val2;

  val2 = arc_mkhash(&c, 8);
  test_builtin("sref", 3, INT2FIX(1), INT2FIX(2), val2);
  test_builtin("sref", 3, INT2FIX(2), INT2FIX(3), val2);
  test_builtin("sref", 3, INT2FIX(3), INT2FIX(4), val2);
  fail_unless(FIX2INT(test_builtin("len", 1, val2)) == 3);

  val2 = arc_mkstringc(&c, "abcd");
  fail_unless(FIX2INT(test_builtin("len", 1, val2)) == 4);

  val2 = cons(&c, INT2FIX(1), cons(&c, INT2FIX(2), cons(&c, INT2FIX(3), CNIL)));
  fail_unless(FIX2INT(test_builtin("len", 1, val2)) == 3);

  val2 = arc_mkvector(&c, 4);
  VINDEX(val2, 0) = INT2FIX(1);
  VINDEX(val2, 1) = INT2FIX(2);
  VINDEX(val2, 2) = INT2FIX(3);
  VINDEX(val2, 3) = INT2FIX(4);
  fail_unless(FIX2INT(test_builtin("len", 1, val2)) == 4);
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

  cc = &c;

  tcase_add_test(tc_bif, test_builtin_is);
  tcase_add_test(tc_bif, test_builtin_iso);
  tcase_add_test(tc_bif, test_builtin_gt);
  tcase_add_test(tc_bif, test_builtin_lt);
  tcase_add_test(tc_bif, test_builtin_gte);
  tcase_add_test(tc_bif, test_builtin_lte);
  tcase_add_test(tc_bif, test_builtin_bound);
  tcase_add_test(tc_bif, test_builtin_exact);
  tcase_add_test(tc_bif, test_builtin_spaceship);
  tcase_add_test(tc_bif, test_builtin_fixnump);

  tcase_add_test(tc_bif, test_builtin_type);
  tcase_add_test(tc_bif, test_builtin_coerce_int);
  tcase_add_test(tc_bif, test_builtin_coerce_flonum);
  tcase_add_test(tc_bif, test_builtin_coerce_rational);
  tcase_add_test(tc_bif, test_builtin_coerce_complex);
  tcase_add_test(tc_bif, test_builtin_coerce_string);
  tcase_add_test(tc_bif, test_builtin_coerce_cons);
  tcase_add_test(tc_bif, test_builtin_coerce_sym);
  tcase_add_test(tc_bif, test_builtin_coerce_vector);
  tcase_add_test(tc_bif, test_builtin_coerce_char);
  tcase_add_test(tc_bif, test_builtin_coerce_num);
  tcase_add_test(tc_bif, test_builtin_atr);

  tcase_add_test(tc_bif, test_builtin_idiv);
  tcase_add_test(tc_bif, test_builtin_expt);
  tcase_add_test(tc_bif, test_builtin_pow);
  tcase_add_test(tc_bif, test_builtin_mod);
  tcase_add_test(tc_bif, test_builtin_abs);
  tcase_add_test(tc_bif, test_builtin_rand);

  tcase_add_test(tc_bif, test_builtin_table);

  tcase_add_test(tc_bif, test_symbols);
  tcase_add_test(tc_bif, test_ssyntax);
  tcase_add_test(tc_bif, test_builtin_uniq);

  tcase_add_test(tc_bif, test_builtin_times);

  tcase_add_test(tc_bif, test_builtin_sref);
  tcase_add_test(tc_bif, test_builtin_len);

  suite_add_tcase(s, tc_bif);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

