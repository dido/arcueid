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

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Built-in Functions");
  TCase *tc_bif = tcase_create("Built-in Functions");
  SRunner *sr;

  c = &cc;
  arc_init(c);

  tcase_add_test(tc_bif, test_coerce_fixnum);

  suite_add_tcase(s, tc_bif);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

