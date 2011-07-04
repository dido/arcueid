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

#define ITEST_HEADER(nlits) \
  value cctx, thr, func; \
  int i; \
  cctx = arc_mkcctx(&c, 1, nlits)

#define ITEST_FOOTER(nlits) \
  func = arc_mkcode(&c, CCTX_VCODE(cctx), arc_mkstringc(&c, "test"), CNIL, nlits); \
  for (i=0; i<nlits; i++) \
    CODE_LITERAL(func, i) = VINDEX(CCTX_LITS(cctx), i); \
  thr = arc_mkthread(&c, func, 2048, 0); \
  arc_vmengine(&c, thr, 1000)

START_TEST(test_vm_push)
{
  ITEST_HEADER(0);
  arc_gcode(&c, cctx, inil);
  arc_gcode(&c, cctx, ipush);
  arc_gcode(&c, cctx, itrue);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, ipush);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(*(TSP(thr)+1) == INT2FIX(31337));
  fail_unless(*(TSP(thr)+2) == CTRUE);
  fail_unless(*(TSP(thr)+3) == CNIL);
}
END_TEST

START_TEST(test_vm_pop)
{
  ITEST_HEADER(0);
  arc_gcode(&c, cctx, inil);
  arc_gcode(&c, cctx, ipush);
  arc_gcode(&c, cctx, itrue);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, ipush);
  arc_gcode(&c, cctx, ipop);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == INT2FIX(31337));
  fail_unless(*(TSP(thr)+1) == CTRUE);
  fail_unless(*(TSP(thr)+2) == CNIL);
}
END_TEST

START_TEST(test_vm_ldi)
{
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, inop);
  arc_gcode(&c, cctx, ihlt);
  func = arc_mkcode(&c, CCTX_VCODE(cctx), arc_mkstringc(&c, "test"), CNIL, 0);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == INT2FIX(31337));
}
END_TEST

START_TEST(test_vm_ldl)
{
  ITEST_HEADER(1);
  VINDEX(CCTX_LITS(cctx), 0) = arc_mkflonum(&c, 3.1415926535);

  arc_gcode1(&c, cctx, ildl, 0);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(1);
  fail_unless(TYPE(TVALR(thr)) == T_FLONUM);
  fail_unless(fabs(REP(TVALR(thr))._flonum - 3.1415926535) < 1e-6);
}
END_TEST

START_TEST(test_vm_ldg)
{
  value sym;
  ITEST_HEADER(1);
  sym = arc_intern_cstr(&c, "foo");
  VINDEX(CCTX_LITS(cctx), 0) = sym;
  arc_hash_insert(&c, c.genv, sym, INT2FIX(31337));
  arc_gcode1(&c, cctx, ildg, 0);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(1);
  fail_unless(TYPE(TVALR(thr)) == T_FIXNUM);
  fail_unless(TVALR(thr) == INT2FIX(31337));
}
END_TEST

START_TEST(test_vm_stg)
{
  value sym;
  ITEST_HEADER(1);
  sym = arc_intern_cstr(&c, "foo");
  VINDEX(CCTX_LITS(cctx), 0) = sym;
  arc_hash_insert(&c, c.genv, sym, INT2FIX(0));
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode1(&c, cctx, istg, 0);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(1);
  fail_unless(arc_hash_lookup(&c, c.genv, sym) == INT2FIX(31337));
}
END_TEST

/* environment instructions */
START_TEST(test_vm_envs)
{
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ienv, 2);
  arc_gcode(&c, cctx, itrue);
  arc_gcode2(&c, cctx, iste, 0, 0);
  arc_gcode1(&c, cctx, ildi, INT2FIX(1234));
  arc_gcode2(&c, cctx, iste, 0, 1);
  arc_gcode1(&c, cctx, ienv, 2);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode2(&c, cctx, iste, 0, 0);
  arc_gcode1(&c, cctx, ildi, INT2FIX(73313));
  arc_gcode2(&c, cctx, iste, 0, 1);
  arc_gcode2(&c, cctx, ilde, 1, 1);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == INT2FIX(1234));
  fail_unless(ENV_VALUE(car(TENVR(thr)), 0) == INT2FIX(31337));
  fail_unless(ENV_VALUE(car(TENVR(thr)), 1) == INT2FIX(73313));
}
END_TEST

static int error = 0;

static void signal_error_test(struct arc *c, const char *fmt, ...)
{
  error = 1;
}

START_TEST(test_vm_mvarg)
{
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ienv, 2);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31338));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, imvarg, 0);
  arc_gcode1(&c, cctx, imvarg, 1);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(ENV_VALUE(car(TENVR(thr)), 0) == INT2FIX(31338));
  fail_unless(ENV_VALUE(car(TENVR(thr)), 1) == INT2FIX(31337));
}
END_TEST

START_TEST(test_vm_mvarg_fail)
{
  /* test failure */
  c.signal_error = signal_error_test;
  error = 0;
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ienv, 2);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, imvarg, 0);
  arc_gcode1(&c, cctx, imvarg, 1);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(error == 1);
}
END_TEST

START_TEST(test_vm_mvoarg)
{
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ienv, 2);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, imvoarg, 0);
  arc_gcode1(&c, cctx, imvoarg, 1);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(ENV_VALUE(car(TENVR(thr)), 0) == INT2FIX(31337));
  fail_unless(ENV_VALUE(car(TENVR(thr)), 1) == CNIL);
}
END_TEST

START_TEST(test_vm_mvrarg)
{
  value rarg;

  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ienv, 1);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31338));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, imvrarg, 0);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  rarg = ENV_VALUE(car(TENVR(thr)), 0);
  fail_unless(car(rarg) == INT2FIX(31337));
  fail_unless(car(cdr(rarg)) == INT2FIX(31338));
  fail_unless(cdr(cdr(rarg)) == CNIL);
}
END_TEST

START_TEST(test_vm_true)
{
  ITEST_HEADER(0);
  arc_gcode(&c, cctx, itrue);
  arc_gcode(&c, cctx, inop);
  arc_gcode(&c, cctx, ihlt);
  func = arc_mkcode(&c, CCTX_VCODE(cctx), arc_mkstringc(&c, "test"), CNIL, 0);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == CTRUE);
}
END_TEST

START_TEST(test_vm_nil)
{
  ITEST_HEADER(0);
  arc_gcode(&c, cctx, inil);
  arc_gcode(&c, cctx, inop);
  arc_gcode(&c, cctx, ihlt);
  func = arc_mkcode(&c, CCTX_VCODE(cctx), arc_mkstringc(&c, "test"), CNIL, 0);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == CNIL);
}
END_TEST

START_TEST(test_vm_apply)
{
  value cctx, cctx2, func, func2, thr;
  int contofs, base;

  cctx = arc_mkcctx(&c, 1, 0);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, iret);
  func = arc_mkcode(&c, CCTX_VCODE(cctx), arc_mkstringc(&c, "test"), CNIL, 0);

  cctx2 = arc_mkcctx(&c, 1, 1);
  VINDEX(CCTX_LITS(cctx2), 0) = func;
  base = FIX2INT(CCTX_VCPTR(cctx2));
  arc_gcode1(&c, cctx2, ildi, INT2FIX(0xf1e));
  arc_gcode(&c, cctx2, ipush);
  arc_gcode1(&c, cctx2, icont, 0);
  contofs = FIX2INT(CCTX_VCPTR(cctx2)) - 1;
  arc_gcode1(&c, cctx2, ildl, 0);
  arc_gcode(&c, cctx2, icls);
  arc_gcode1(&c, cctx2, iapply, 0);
  VINDEX(CCTX_VCODE(cctx2), contofs) = FIX2INT(CCTX_VCPTR(cctx2)) - base;
  arc_gcode(&c, cctx2, ihlt);
  func2 = arc_mkcode(&c, CCTX_VCODE(cctx2), arc_mkstringc(&c, "test2"), CNIL, 1);
  CODE_LITERAL(func2, 0) = VINDEX(CCTX_LITS(cctx2), 0);
  thr = arc_mkthread(&c, func2, 2048, 0);
  arc_vmengine(&c, thr, 1000);
  fail_unless(TVALR(thr) == INT2FIX(31337));
  fail_unless(*(TSP(thr)+1) == INT2FIX(0xf1e));
}
END_TEST

START_TEST(test_vm_jmp)
{
  int jmpofs;

  ITEST_HEADER(0);
  arc_gcode(&c, cctx, inil);
  arc_gcode1(&c, cctx, ijmp, 0);
  jmpofs = FIX2INT(CCTX_VCPTR(cctx)) - 1;
  arc_gcode(&c, cctx, itrue);
  arc_gcode(&c, cctx, ihlt);
  VINDEX(CCTX_VCODE(cctx), jmpofs) = FIX2INT(CCTX_VCPTR(cctx)) - (jmpofs-1);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, ihlt);  
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == INT2FIX(31337));
}
END_TEST

START_TEST(test_vm_jt_true)
{
  int jmpofs;

  ITEST_HEADER(0);
  arc_gcode(&c, cctx, itrue);
  arc_gcode1(&c, cctx, ijt, 0);
  jmpofs = FIX2INT(CCTX_VCPTR(cctx)) - 1;
  arc_gcode(&c, cctx, inil);
  arc_gcode(&c, cctx, ihlt);
  VINDEX(CCTX_VCODE(cctx), jmpofs) = FIX2INT(CCTX_VCPTR(cctx)) - (jmpofs-1);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, ihlt);  
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == INT2FIX(31337));
}
END_TEST

START_TEST(test_vm_jt_false)
{
  int jmpofs;

  ITEST_HEADER(0);
  arc_gcode(&c, cctx, inil);
  arc_gcode1(&c, cctx, ijt, 0);
  jmpofs = FIX2INT(CCTX_VCPTR(cctx)) - 1;
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, ihlt);
  VINDEX(CCTX_VCODE(cctx), jmpofs) = FIX2INT(CCTX_VCPTR(cctx)) - (jmpofs-1);
  arc_gcode1(&c, cctx, ildi, INT2FIX(0));
  arc_gcode(&c, cctx, ihlt);  
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == INT2FIX(31337));
}
END_TEST

START_TEST(test_vm_jf_true)
{
  int jmpofs;

  ITEST_HEADER(0);
  arc_gcode(&c, cctx, inil);
  arc_gcode1(&c, cctx, ijf, 0);
  jmpofs = FIX2INT(CCTX_VCPTR(cctx)) - 1;
  arc_gcode(&c, cctx, inil);
  arc_gcode(&c, cctx, ihlt);
  VINDEX(CCTX_VCODE(cctx), jmpofs) = FIX2INT(CCTX_VCPTR(cctx)) - (jmpofs-1);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, ihlt);  
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == INT2FIX(31337));
}
END_TEST

START_TEST(test_vm_jf_false)
{
  int jmpofs;

  ITEST_HEADER(0);
  arc_gcode(&c, cctx, itrue);
  arc_gcode1(&c, cctx, ijf, 0);
  jmpofs = FIX2INT(CCTX_VCPTR(cctx)) - 1;
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, ihlt);
  VINDEX(CCTX_VCODE(cctx), jmpofs) = FIX2INT(CCTX_VCPTR(cctx)) - (jmpofs-1);
  arc_gcode1(&c, cctx, ildi, INT2FIX(0));
  arc_gcode(&c, cctx, ihlt);  
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == INT2FIX(31337));
}
END_TEST

START_TEST(test_vm_add)
{
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31330));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(7));
  arc_gcode(&c, cctx, iadd);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == INT2FIX(31337));
}
END_TEST

START_TEST(test_vm_sub)
{
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ildi, INT2FIX(7));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31344));
  arc_gcode(&c, cctx, isub);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == INT2FIX(31337));
}
END_TEST

START_TEST(test_vm_mul)
{
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ildi, INT2FIX(2));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, imul);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == INT2FIX(62674));
}
END_TEST

START_TEST(test_vm_div)
{
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ildi, INT2FIX(2));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(62674));
  arc_gcode(&c, cctx, idiv);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == INT2FIX(31337));
}
END_TEST

START_TEST(test_vm_cons)
{
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ildi, INT2FIX(62674));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, icons);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(CONS_P(TVALR(thr)));
  fail_unless(car(TVALR(thr)) == INT2FIX(31337));
  fail_unless(cdr(TVALR(thr)) == INT2FIX(62674));
}
END_TEST

START_TEST(test_vm_car)
{
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ildi, INT2FIX(62674));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, icons);
  arc_gcode(&c, cctx, icar);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == INT2FIX(31337));
}
END_TEST

START_TEST(test_vm_cdr)
{
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ildi, INT2FIX(62674));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, icons);
  arc_gcode(&c, cctx, icdr);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == INT2FIX(62674));
}
END_TEST

START_TEST(test_vm_scar)
{
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(62674));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(0));
  arc_gcode(&c, cctx, icons);
  arc_gcode(&c, cctx, iscar);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(car(TVALR(thr)) == INT2FIX(31337));
}
END_TEST

START_TEST(test_vm_scdr)
{
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ildi, INT2FIX(62674));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(0));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(31337));
  arc_gcode(&c, cctx, icons);
  arc_gcode(&c, cctx, iscdr);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(cdr(TVALR(thr)) == INT2FIX(62674));
}
END_TEST

START_TEST(test_vm_spl)
{
  /* This is code generated by the compiler for`(1 ,@'(2 3) 4) */
  ITEST_HEADER(0);
  arc_gcode(&c, cctx, inil);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(4));
  arc_gcode(&c, cctx, icons);
  arc_gcode(&c, cctx, ipush);
  arc_gcode(&c, cctx, inil);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(3));
  arc_gcode(&c, cctx, icons);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(2));
  arc_gcode(&c, cctx, icons);
  arc_gcode(&c, cctx, ispl);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(1));
  arc_gcode(&c, cctx, icons);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(CONS_P(TVALR(thr)));
  fail_unless(car(TVALR(thr)) == INT2FIX(1));
  fail_unless(car(cdr(TVALR(thr))) == INT2FIX(2));
  fail_unless(car(cdr(cdr(TVALR(thr)))) == INT2FIX(3));
  fail_unless(car(cdr(cdr(cdr(TVALR(thr))))) == INT2FIX(4));
}
END_TEST

START_TEST(test_vm_is_t)
{
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ildi, INT2FIX(1));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(1));
  arc_gcode(&c, cctx, iis);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == CTRUE);
}
END_TEST

START_TEST(test_vm_is_nil)
{
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ildi, INT2FIX(1));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(2));
  arc_gcode(&c, cctx, iis);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == CNIL);
}
END_TEST

START_TEST(test_vm_iso_t)
{
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ildi, INT2FIX(1));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(1));
  arc_gcode(&c, cctx, iiso);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == CTRUE);
}
END_TEST

START_TEST(test_vm_iso_nil)
{
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ildi, INT2FIX(1));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(2));
  arc_gcode(&c, cctx, iiso);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == CNIL);
}
END_TEST

START_TEST(test_vm_gt_t)
{
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ildi, INT2FIX(1));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(2));
  arc_gcode(&c, cctx, igt);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == CTRUE);
}
END_TEST

START_TEST(test_vm_gt_nil)
{
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ildi, INT2FIX(2));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(1));
  arc_gcode(&c, cctx, igt);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == CNIL);
}
END_TEST

START_TEST(test_vm_lt_t)
{
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ildi, INT2FIX(2));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(1));
  arc_gcode(&c, cctx, ilt);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == CTRUE);
}
END_TEST

START_TEST(test_vm_lt_nil)
{
  ITEST_HEADER(0);
  arc_gcode1(&c, cctx, ildi, INT2FIX(1));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(2));
  arc_gcode(&c, cctx, ilt);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == CNIL);
}
END_TEST

START_TEST(test_vm_dsb)
{
  value cctx, func, func2, thr;
  int base, contofs;

  /* This code is the code generated by the compiler for the function

     (fn (a (b c) d) (cons a (cons b (cons c (cons d nil)))))

     This performs a destructuring bind of its arguments. */
  cctx = arc_mkcctx(&c, 1, 0);
  arc_gcode1(&c, cctx, ienv, 4);
  arc_gcode1(&c, cctx, imvarg, 0);
  arc_gcode(&c, cctx, idup);
  arc_gcode(&c, cctx, icar);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, imvarg, 1);
  arc_gcode(&c, cctx, idup);
  arc_gcode(&c, cctx, icdr);
  arc_gcode(&c, cctx, icar);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, imvarg, 2);
  arc_gcode(&c, cctx, ipop);
  arc_gcode1(&c, cctx, imvarg, 3);
  arc_gcode2(&c, cctx, ilde, 0, 0);
  arc_gcode(&c, cctx, ipush);
  arc_gcode2(&c, cctx, ilde, 0, 1);
  arc_gcode(&c, cctx, ipush);
  arc_gcode2(&c, cctx, ilde, 0, 2);
  arc_gcode(&c, cctx, ipush);
  arc_gcode2(&c, cctx, ilde, 0, 3);
  arc_gcode(&c, cctx, ipush);
  arc_gcode(&c, cctx, inil);
  arc_gcode(&c, cctx, iconsr);
  arc_gcode(&c, cctx, iconsr);
  arc_gcode(&c, cctx, iconsr);
  arc_gcode(&c, cctx, iconsr);
  arc_gcode(&c, cctx, iret);
  func = arc_mkcode(&c, CCTX_VCODE(cctx), arc_mkstringc(&c, "test"), CNIL, 0);

  /* Attempt to apply the arguments 1 (2 3) 4 to the above destructuring
     bind function.   This should result in the list (1 2 3 4) if it works
     properly. */
  cctx = arc_mkcctx(&c, 1, 1);
  VINDEX(CCTX_LITS(cctx), 0) = func;
  base = FIX2INT(CCTX_VCPTR(cctx));
  arc_gcode1(&c, cctx, icont, 0);
  contofs = FIX2INT(CCTX_VCPTR(cctx)) - 1;
  arc_gcode1(&c, cctx, ildi, INT2FIX(4));
  arc_gcode(&c, cctx, ipush);
  arc_gcode(&c, cctx, inil);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(3));
  arc_gcode(&c, cctx, icons);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(2));
  arc_gcode(&c, cctx, icons);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(1));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildl, 0);
  arc_gcode(&c, cctx, icls);
  arc_gcode1(&c, cctx, iapply, 3);
  VINDEX(CCTX_VCODE(cctx), contofs) = FIX2INT(CCTX_VCPTR(cctx)) - base;
  arc_gcode(&c, cctx, ihlt);
  func2 = arc_mkcode(&c, CCTX_VCODE(cctx), arc_mkstringc(&c, "test2"), CNIL, 1);
  CODE_LITERAL(func2, 0) = VINDEX(CCTX_LITS(cctx), 0);
  thr = arc_mkthread(&c, func2, 2048, 0);
  arc_vmengine(&c, thr, 1000);
  fail_unless(CONS_P(TVALR(thr)));
  fail_unless(car(TVALR(thr)) == INT2FIX(1));
  fail_unless(car(cdr(TVALR(thr))) == INT2FIX(2));
  fail_unless(car(cdr(cdr(TVALR(thr)))) == INT2FIX(3));
  fail_unless(car(cdr(cdr(cdr(TVALR(thr))))) == INT2FIX(4));
}
END_TEST

START_TEST(test_vm_if_compile1)
{
  ITEST_HEADER(0);
  arc_gcode(&c, cctx, inil);
  arc_gcode1(&c, cctx, ijf, 6);
  arc_gcode1(&c, cctx, ildi, 3);
  arc_gcode1(&c, cctx, ijmp, 4);
  arc_gcode1(&c, cctx, ildi, 5);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == INT2FIX(2));
}
END_TEST

START_TEST(test_vm_if_compile2)
{
  ITEST_HEADER(0);
  arc_gcode(&c, cctx, itrue);
  arc_gcode1(&c, cctx, ijf, 6);
  arc_gcode1(&c, cctx, ildi, 3);
  arc_gcode1(&c, cctx, ijmp, 4);
  arc_gcode1(&c, cctx, ildi, 5);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == INT2FIX(1));
}
END_TEST

START_TEST(test_vm_oarg)
{
  value cctx, func, func2, thr;
  int base, contofs;

  /* This code is the code generated by the compiler for the function

     (fn ((o x 1) (o y 2)) (+ x y))

     With an optional argument having a default value */
  cctx = arc_mkcctx(&c, 1, 0);
  arc_gcode1(&c, cctx, ienv, 2);
  arc_gcode1(&c, cctx, imvoarg, 0);
  arc_gcode2(&c, cctx, ilde, 0, 0);
  arc_gcode1(&c, cctx, ijt, 7);
  arc_gcode1(&c, cctx, ildi, 3);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, imvoarg, 0);
  arc_gcode1(&c, cctx, imvoarg, 1);
  arc_gcode2(&c, cctx, ilde, 0, 1);
  arc_gcode1(&c, cctx, ijt, 7);
  arc_gcode1(&c, cctx, ildi, 5);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, imvoarg, 1);
  arc_gcode1(&c, cctx, ildi, 1);
  arc_gcode(&c, cctx, ipush);
  arc_gcode2(&c, cctx, ilde, 0, 0);
  arc_gcode(&c, cctx, iadd);
  arc_gcode(&c, cctx, ipush);
  arc_gcode2(&c, cctx, ilde, 0, 1);
  arc_gcode(&c, cctx, iadd);
  arc_gcode(&c, cctx, iret);
  func = arc_mkcode(&c, CCTX_VCODE(cctx), arc_mkstringc(&c, "test"), CNIL, 0);

  /* Attempt to apply the argument 3 to the above function. Should result in 5 */
  cctx = arc_mkcctx(&c, 1, 1);
  VINDEX(CCTX_LITS(cctx), 0) = func;
  base = FIX2INT(CCTX_VCPTR(cctx));
  arc_gcode1(&c, cctx, icont, 10); /* computed offset by compiler */
  contofs = FIX2INT(CCTX_VCPTR(cctx)) - 1;
  arc_gcode1(&c, cctx, ildi, 7);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildl, 0);
  arc_gcode(&c, cctx, icls);
  arc_gcode1(&c, cctx, iapply, 1);
  fail_unless(VINDEX(CCTX_VCODE(cctx), contofs) == FIX2INT(CCTX_VCPTR(cctx)) - base);
  arc_gcode(&c, cctx, ihlt);
  func2 = arc_mkcode(&c, CCTX_VCODE(cctx), arc_mkstringc(&c, "test2"), CNIL, 1);
  CODE_LITERAL(func2, 0) = VINDEX(CCTX_LITS(cctx), 0);
  thr = arc_mkthread(&c, func2, 2048, 0);
  arc_vmengine(&c, thr, 1000);
  fail_unless(TVALR(thr) == INT2FIX(5));
}
END_TEST

static value test_cfunc(arc *c, value arg)
{
  return(INT2FIX(FIX2INT(arg) + 2));
}

START_TEST(test_vm_ffi_cc1)
{
  int base, contofs;
  /* Attempt to apply the argument 3 to the above function. Should result in 5 */
  ITEST_HEADER(1);
  VINDEX(CCTX_LITS(cctx), 0) = arc_mkccode(&c, 1, test_cfunc);
  base = FIX2INT(CCTX_VCPTR(cctx));
  arc_gcode1(&c, cctx, icont, 9); /* computed offset by compiler */
  contofs = FIX2INT(CCTX_VCPTR(cctx)) - 1;
  arc_gcode1(&c, cctx, ildi, 7);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildl, 0);
  arc_gcode1(&c, cctx, iapply, 1);
  fail_unless(VINDEX(CCTX_VCODE(cctx), contofs) == FIX2INT(CCTX_VCPTR(cctx)) - base);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(1);
  fail_unless(TVALR(thr) == INT2FIX(5));
}
END_TEST

static value test_cfunc2(arc *c, int argc, value *argv)
{
  fail_unless(argc == 2);
  return(INT2FIX(FIX2INT(argv[0]) + FIX2INT(argv[1])));
}

START_TEST(test_vm_ffi_cc2)
{
  int base, contofs;
  /* Attempt to apply the arguments 3 and 7 to the above function. Should
     result in 10 */
  ITEST_HEADER(1);
  VINDEX(CCTX_LITS(cctx), 0) = arc_mkccode(&c, -1, test_cfunc2);
  base = FIX2INT(CCTX_VCPTR(cctx));
  arc_gcode1(&c, cctx, icont, 0);
  contofs = FIX2INT(CCTX_VCPTR(cctx)) - 1;
  arc_gcode1(&c, cctx, ildi, INT2FIX(3));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(7));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildl, 0);
  arc_gcode1(&c, cctx, iapply, 2);
  VINDEX(CCTX_VCODE(cctx), contofs) = FIX2INT(CCTX_VCPTR(cctx)) - base;
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(1);
  fail_unless(TVALR(thr) == INT2FIX(10));
}
END_TEST

static value test_cfunc3(arc *c, value argv)
{
  fail_unless(VECLEN(argv) == 2);
  return(INT2FIX(FIX2INT(VINDEX(argv, 0)) + FIX2INT(VINDEX(argv, 1))));
}

START_TEST(test_vm_ffi_cc3)
{
  int base, contofs;
  /* Attempt to apply the arguments 3 and 7 to the above function. Should
     result in 10 */
  ITEST_HEADER(1);
  VINDEX(CCTX_LITS(cctx), 0) = arc_mkccode(&c, -2, test_cfunc3);
  base = FIX2INT(CCTX_VCPTR(cctx));
  arc_gcode1(&c, cctx, icont, 0);
  contofs = FIX2INT(CCTX_VCPTR(cctx)) - 1;
  arc_gcode1(&c, cctx, ildi, INT2FIX(3));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(7));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildl, 0);
  arc_gcode1(&c, cctx, iapply, 2);
  VINDEX(CCTX_VCODE(cctx), contofs) = FIX2INT(CCTX_VCPTR(cctx)) - base;
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(1);
  fail_unless(TVALR(thr) == INT2FIX(10));
}
END_TEST

START_TEST(test_vm_apply_list)
{
  int base, contofs;
  ITEST_HEADER(0);
  base = FIX2INT(CCTX_VCPTR(cctx));
  arc_gcode1(&c, cctx, icont, 20); /* computed offset by compiler */
  contofs = FIX2INT(CCTX_VCPTR(cctx)) - 1;
  arc_gcode1(&c, cctx, ildi, INT2FIX(2));
  arc_gcode(&c, cctx, ipush);
  arc_gcode(&c, cctx, inil);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, 7);
  arc_gcode(&c, cctx, icons);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, 5);
  arc_gcode(&c, cctx, icons);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, 3);
  arc_gcode(&c, cctx, icons);
  arc_gcode1(&c, cctx, iapply, 1);
  fail_unless(VINDEX(CCTX_VCODE(cctx), contofs) == FIX2INT(CCTX_VCPTR(cctx)) - base);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == INT2FIX(3));
}
END_TEST

static void signal_error_multiargs(struct arc *c, const char *fmt, ...)
{
  va_list ap;

  fail_unless(strcmp(fmt, "list application expects 1 argument, given %d") == 0);
  va_start(ap, fmt);
  fail_unless(va_arg(ap, value) == INT2FIX(2));
  va_end(ap);
}

START_TEST(test_vm_apply_list_err1)
{
  int base, contofs;

  c.signal_error = signal_error_multiargs;
  ITEST_HEADER(0);
  base = FIX2INT(CCTX_VCPTR(cctx));
  arc_gcode1(&c, cctx, icont, 23); /* computed offset by compiler */
  contofs = FIX2INT(CCTX_VCPTR(cctx)) - 1;
  arc_gcode1(&c, cctx, ildi, 7);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, 5);
  arc_gcode(&c, cctx, ipush);
  arc_gcode(&c, cctx, inil);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, 7);
  arc_gcode(&c, cctx, icons);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, 5);
  arc_gcode(&c, cctx, icons);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, 3);
  arc_gcode(&c, cctx, icons);
  arc_gcode1(&c, cctx, iapply, 2);
  fail_unless(VINDEX(CCTX_VCODE(cctx), contofs) == FIX2INT(CCTX_VCPTR(cctx)) - base);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == CNIL);
}
END_TEST

static void signal_error_negative(struct arc *c, const char *fmt, ...)
{
  va_list ap;

  fail_unless(strcmp(fmt, "list application expects non-negative exact integer argument, given object of type %d") == 0);
  va_start(ap, fmt);
  fail_unless(va_arg(ap, value) == INT2FIX(2));
  va_end(ap);
}

START_TEST(test_vm_apply_list_err2)
{

  int base, contofs;

  c.signal_error = signal_error_negative;
  ITEST_HEADER(0);
  base = FIX2INT(CCTX_VCPTR(cctx));
  arc_gcode1(&c, cctx, icont, 20); /* computed offset by compiler */
  contofs = FIX2INT(CCTX_VCPTR(cctx)) - 1;
  arc_gcode1(&c, cctx, ildi, INT2FIX(-1));
  arc_gcode(&c, cctx, ipush);
  arc_gcode(&c, cctx, inil);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, 7);
  arc_gcode(&c, cctx, icons);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, 5);
  arc_gcode(&c, cctx, icons);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, 3);
  arc_gcode(&c, cctx, icons);
  arc_gcode1(&c, cctx, iapply, 1);
  fail_unless(VINDEX(CCTX_VCODE(cctx), contofs) == FIX2INT(CCTX_VCPTR(cctx)) - base);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == CNIL);
}
END_TEST

static void signal_error_oob(struct arc *c, const char *fmt, ...)
{
  va_list ap;

  fail_unless(strcmp(fmt, "index %d too large for list") == 0);
  va_start(ap, fmt);
  fail_unless(va_arg(ap, value) == INT2FIX(100));
  va_end(ap);
}

START_TEST(test_vm_apply_list_err3)
{

  int base, contofs;

  c.signal_error = signal_error_oob;
  ITEST_HEADER(0);
  base = FIX2INT(CCTX_VCPTR(cctx));
  arc_gcode1(&c, cctx, icont, 20); /* computed offset by compiler */
  contofs = FIX2INT(CCTX_VCPTR(cctx)) - 1;
  arc_gcode1(&c, cctx, ildi, INT2FIX(100));
  arc_gcode(&c, cctx, ipush);
  arc_gcode(&c, cctx, inil);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, 7);
  arc_gcode(&c, cctx, icons);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, 5);
  arc_gcode(&c, cctx, icons);
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, 3);
  arc_gcode(&c, cctx, icons);
  arc_gcode1(&c, cctx, iapply, 1);
  fail_unless(VINDEX(CCTX_VCODE(cctx), contofs) == FIX2INT(CCTX_VCPTR(cctx)) - base);
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(0);
  fail_unless(TVALR(thr) == CNIL);
}
END_TEST

START_TEST(test_vm_apply_vec)
{
  int base, contofs;
  value vec;

  ITEST_HEADER(1);
  vec = arc_mkvector(&c, 3);
  VINDEX(vec, 0) = INT2FIX(1);
  VINDEX(vec, 1) = INT2FIX(2);
  VINDEX(vec, 2) = INT2FIX(3);
  VINDEX(CCTX_LITS(cctx), 0) = vec;
  base = FIX2INT(CCTX_VCPTR(cctx));
  arc_gcode1(&c, cctx, icont, 0);
  contofs = FIX2INT(CCTX_VCPTR(cctx)) - 1;
  arc_gcode1(&c, cctx, ildi, INT2FIX(1));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildl, 0);
  arc_gcode1(&c, cctx, iapply, 1);
  VINDEX(CCTX_VCODE(cctx), contofs) = FIX2INT(CCTX_VCPTR(cctx)) - base;
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(1);
  fail_unless(TVALR(thr) == INT2FIX(2));
}
END_TEST

static void signal_error_multiargs_vec(struct arc *c, const char *fmt, ...)
{
  va_list ap;

  fail_unless(strcmp(fmt, "vector application expects 1 argument, given %d") == 0);
  va_start(ap, fmt);
  fail_unless(va_arg(ap, value) == INT2FIX(2));
  va_end(ap);
}

START_TEST(test_vm_apply_vec_err1)
{
  int base, contofs;
  value vec;

  c.signal_error = signal_error_multiargs_vec;
  ITEST_HEADER(1);
  vec = arc_mkvector(&c, 3);
  VINDEX(vec, 0) = INT2FIX(1);
  VINDEX(vec, 1) = INT2FIX(2);
  VINDEX(vec, 2) = INT2FIX(3);
  VINDEX(CCTX_LITS(cctx), 0) = vec;
  base = FIX2INT(CCTX_VCPTR(cctx));
  arc_gcode1(&c, cctx, icont, 0);
  contofs = FIX2INT(CCTX_VCPTR(cctx)) - 1;
  arc_gcode1(&c, cctx, ildi, INT2FIX(1));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildi, INT2FIX(2));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildl, 0);
  arc_gcode1(&c, cctx, iapply, 2);
  VINDEX(CCTX_VCODE(cctx), contofs) = FIX2INT(CCTX_VCPTR(cctx)) - base;
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(1);
  fail_unless(TVALR(thr) == CNIL);
}
END_TEST

static void signal_error_negative_vec(struct arc *c, const char *fmt, ...)
{
  va_list ap;

  fail_unless(strcmp(fmt, "vector application expects non-negative fixnum argument, given object of type %d") == 0);
  va_start(ap, fmt);
  fail_unless(va_arg(ap, value) == INT2FIX(2));
  va_end(ap);
}

START_TEST(test_vm_apply_vec_err2)
{
  int base, contofs;
  value vec;

  c.signal_error = signal_error_negative_vec;
  ITEST_HEADER(1);
  vec = arc_mkvector(&c, 3);
  VINDEX(vec, 0) = INT2FIX(1);
  VINDEX(vec, 1) = INT2FIX(2);
  VINDEX(vec, 2) = INT2FIX(3);
  VINDEX(CCTX_LITS(cctx), 0) = vec;
  base = FIX2INT(CCTX_VCPTR(cctx));
  arc_gcode1(&c, cctx, icont, 0);
  contofs = FIX2INT(CCTX_VCPTR(cctx)) - 1;
  arc_gcode1(&c, cctx, ildi, INT2FIX(-1));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildl, 0);
  arc_gcode1(&c, cctx, iapply, 1);
  VINDEX(CCTX_VCODE(cctx), contofs) = FIX2INT(CCTX_VCPTR(cctx)) - base;
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(1);
  fail_unless(TVALR(thr) == CNIL);
}
END_TEST

static void signal_error_oob_vec(struct arc *c, const char *fmt, ...)
{
  va_list ap;

  fail_unless(strcmp(fmt, "index %d too large for vector") == 0);
  va_start(ap, fmt);
  fail_unless(va_arg(ap, value) == INT2FIX(100));
  va_end(ap);
}

START_TEST(test_vm_apply_vec_err3)
{
  int base, contofs;
  value vec;

  c.signal_error = signal_error_oob_vec;
  ITEST_HEADER(1);
  vec = arc_mkvector(&c, 3);
  VINDEX(vec, 0) = INT2FIX(1);
  VINDEX(vec, 1) = INT2FIX(2);
  VINDEX(vec, 2) = INT2FIX(3);
  VINDEX(CCTX_LITS(cctx), 0) = vec;
  base = FIX2INT(CCTX_VCPTR(cctx));
  arc_gcode1(&c, cctx, icont, 0);
  contofs = FIX2INT(CCTX_VCPTR(cctx)) - 1;
  arc_gcode1(&c, cctx, ildi, INT2FIX(100));
  arc_gcode(&c, cctx, ipush);
  arc_gcode1(&c, cctx, ildl, 0);
  arc_gcode1(&c, cctx, iapply, 1);
  VINDEX(CCTX_VCODE(cctx), contofs) = FIX2INT(CCTX_VCPTR(cctx)) - base;
  arc_gcode(&c, cctx, ihlt);
  ITEST_FOOTER(1);
  fail_unless(TVALR(thr) == CNIL);
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Virtual Machine");
  TCase *tc_vm = tcase_create("Virtual Machine");
  SRunner *sr;

  arc_set_memmgr(&c);
  arc_init_reader(&c);
  c.genv = arc_mkhash(&c, 8);

  tcase_add_test(tc_vm, test_vm_push);
  tcase_add_test(tc_vm, test_vm_pop);
  tcase_add_test(tc_vm, test_vm_ldi);
  tcase_add_test(tc_vm, test_vm_ldl);
  tcase_add_test(tc_vm, test_vm_ldg);
  tcase_add_test(tc_vm, test_vm_stg);
  tcase_add_test(tc_vm, test_vm_envs);
  tcase_add_test(tc_vm, test_vm_mvarg);
  tcase_add_test(tc_vm, test_vm_mvarg_fail);
  tcase_add_test(tc_vm, test_vm_mvoarg);
  tcase_add_test(tc_vm, test_vm_mvrarg);
  tcase_add_test(tc_vm, test_vm_apply);
  tcase_add_test(tc_vm, test_vm_jmp);
  tcase_add_test(tc_vm, test_vm_jt_true);
  tcase_add_test(tc_vm, test_vm_jt_false);
  tcase_add_test(tc_vm, test_vm_jf_true);
  tcase_add_test(tc_vm, test_vm_jf_false);
  tcase_add_test(tc_vm, test_vm_true);
  tcase_add_test(tc_vm, test_vm_nil);
  tcase_add_test(tc_vm, test_vm_add);
  tcase_add_test(tc_vm, test_vm_sub);
  tcase_add_test(tc_vm, test_vm_mul);
  tcase_add_test(tc_vm, test_vm_div);
  tcase_add_test(tc_vm, test_vm_cons);
  tcase_add_test(tc_vm, test_vm_car);
  tcase_add_test(tc_vm, test_vm_cdr);
  tcase_add_test(tc_vm, test_vm_scar);
  tcase_add_test(tc_vm, test_vm_scdr);
  tcase_add_test(tc_vm, test_vm_spl);
  tcase_add_test(tc_vm, test_vm_is_t);
  tcase_add_test(tc_vm, test_vm_is_nil);
  tcase_add_test(tc_vm, test_vm_iso_t);
  tcase_add_test(tc_vm, test_vm_iso_nil);
  tcase_add_test(tc_vm, test_vm_gt_t);
  tcase_add_test(tc_vm, test_vm_gt_nil);
  tcase_add_test(tc_vm, test_vm_lt_t);
  tcase_add_test(tc_vm, test_vm_lt_nil);
  tcase_add_test(tc_vm, test_vm_dsb);
  tcase_add_test(tc_vm, test_vm_if_compile1);
  tcase_add_test(tc_vm, test_vm_if_compile2);
  tcase_add_test(tc_vm, test_vm_oarg);
  tcase_add_test(tc_vm, test_vm_ffi_cc1);
  tcase_add_test(tc_vm, test_vm_ffi_cc2);
  tcase_add_test(tc_vm, test_vm_ffi_cc3);
  tcase_add_test(tc_vm, test_vm_apply_list);
  tcase_add_test(tc_vm, test_vm_apply_list_err1);
  tcase_add_test(tc_vm, test_vm_apply_list_err2);
  tcase_add_test(tc_vm, test_vm_apply_list_err3);
  tcase_add_test(tc_vm, test_vm_apply_vec);
  tcase_add_test(tc_vm, test_vm_apply_vec_err1);
  tcase_add_test(tc_vm, test_vm_apply_vec_err2);
  tcase_add_test(tc_vm, test_vm_apply_vec_err3);

  suite_add_tcase(s, tc_vm);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

