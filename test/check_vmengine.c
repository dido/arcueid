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

arc cc;
arc *c;

#define QUANTA 128

#define CPUSH_(val) CPUSH(thr, val)

#define XCALL0(clos) do {			\
    TQUANTA(thr) = QUANTA;			\
    TVALR(thr) = clos;				\
    TARGC(thr) = 0;				\
    __arc_thr_trampoline(c, thr, TR_FNAPP);	\
  } while (0)


#define XCALL(clos, ...) do {			\
    TQUANTA(thr) = QUANTA;			\
    TVALR(thr) = clos;				\
    TARGC(thr) = NARGS(__VA_ARGS__);		\
    FOR_EACH(CPUSH_, __VA_ARGS__);		\
    __arc_thr_trampoline(c, thr, TR_FNAPP);	\
  } while (0)

START_TEST(test_nop)
{
  value cctx, code, clos;
  value thr;

  cctx = arc_mkcctx(c);
  arc_emit(c, cctx, inop);
  arc_emit(c, cctx, ihlt);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  thr = arc_mkthread(c);
  XCALL0(clos);
  fail_unless(TQUANTA(thr) == QUANTA-1);
  fail_unless(TSTATE(thr) == Trelease);
  fail_unless(TVALR(thr) == clos);
}
END_TEST

START_TEST(test_push)
{
  value cctx, code, clos;
  value thr;

  cctx = arc_mkcctx(c);
  arc_emit(c, cctx, inil);
  arc_emit(c, cctx, ipush);
  arc_emit(c, cctx, itrue);
  arc_emit(c, cctx, ipush);
  arc_emit1(c, cctx, ildi, INT2FIX(31337));
  arc_emit(c, cctx, ipush);
  arc_emit(c, cctx, ihlt);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  thr = arc_mkthread(c);
  XCALL0(clos);
  fail_unless(TQUANTA(thr) == QUANTA-6);
  fail_unless(TSTATE(thr) == Trelease);
  fail_unless(TVALR(thr) == INT2FIX(31337));
  fail_unless(*(TSP(thr)+1) == INT2FIX(31337));
  fail_unless(*(TSP(thr)+2) == CTRUE);
  fail_unless(*(TSP(thr)+3) == CNIL);
}
END_TEST

START_TEST(test_pop)
{
  value cctx, code, clos;
  value thr;

  cctx = arc_mkcctx(c);
  arc_emit(c, cctx, inil);
  arc_emit(c, cctx, ipush);
  arc_emit(c, cctx, itrue);
  arc_emit(c, cctx, ipush);
  arc_emit1(c, cctx, ildi, INT2FIX(31337));
  arc_emit(c, cctx, ipush);
  arc_emit(c, cctx, ipop);
  arc_emit(c, cctx, ihlt);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  thr = arc_mkthread(c);
  XCALL0(clos);
  fail_unless(TVALR(thr) == INT2FIX(31337));
  fail_unless(*(TSP(thr)+1) == CTRUE);
  fail_unless(*(TSP(thr)+2) == CNIL);
}
END_TEST

START_TEST(test_ldi)
{
  value cctx, code, clos;
  value thr;

  cctx = arc_mkcctx(c);
  arc_emit1(c, cctx, ildi, INT2FIX(31337));
  arc_emit(c, cctx, ihlt);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  thr = arc_mkthread(c);
  XCALL0(clos);
  fail_unless(TQUANTA(thr) == QUANTA-1);
  fail_unless(TSTATE(thr) == Trelease);
  fail_unless(TYPE(TVALR(thr)) == T_FIXNUM);
  fail_unless(TVALR(thr) == INT2FIX(31337));
}
END_TEST

START_TEST(test_ldl)
{
  value cctx, code, clos;
  value thr;
  int lptr;

  cctx = arc_mkcctx(c);
  lptr = arc_literal(c, cctx, arc_mkflonum(c, 3.1415926535));
  arc_emit1(c, cctx, ildl, INT2FIX(lptr));
  arc_emit(c, cctx, ihlt);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);

  thr = arc_mkthread(c);
  XCALL0(clos);
  fail_unless(TQUANTA(thr) == QUANTA-1);
  fail_unless(TSTATE(thr) == Trelease);
  fail_unless(TYPE(TVALR(thr)) == T_FLONUM);
  fail_unless(fabs(REPFLO(TVALR(thr)) - 3.1415926535) < 1e-6);

}
END_TEST

START_TEST(test_ldg)
{
  value cctx, code, clos, thr;
  value sym = arc_intern_cstr(c, "foo");
  int lptr;

  cctx = arc_mkcctx(c);
  lptr = arc_literal(c, cctx, sym);
  arc_hash_insert(c, c->genv, sym, INT2FIX(31337));
  arc_emit1(c, cctx, ildg, INT2FIX(lptr));
  arc_emit(c, cctx, ihlt);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  thr = arc_mkthread(c);
  XCALL0(clos);
  fail_unless(TQUANTA(thr) == QUANTA-1);
  fail_unless(TSTATE(thr) == Trelease);
  fail_unless(TYPE(TVALR(thr)) == T_FIXNUM);
  fail_unless(TVALR(thr) == INT2FIX(31337));
}
END_TEST

START_TEST(test_stg)
{
  value cctx, code, clos, thr;
  value sym = arc_intern_cstr(c, "foo");
  int lptr;

  cctx = arc_mkcctx(c);
  lptr = arc_literal(c, cctx, sym);
  arc_hash_insert(c, c->genv, sym, INT2FIX(0));
  arc_emit1(c, cctx, ildi, INT2FIX(31337));
  arc_emit1(c, cctx, istg, INT2FIX(lptr));
  arc_emit(c, cctx, ihlt);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  thr = arc_mkthread(c);
  XCALL0(clos);
  fail_unless(TQUANTA(thr) == QUANTA-2);
  fail_unless(TSTATE(thr) == Trelease);
  fail_unless(arc_hash_lookup(c, c->genv, sym) == INT2FIX(31337));
}
END_TEST

/* Environment instructions */
START_TEST(test_envs)
{
  value cctx, code, clos, thr;

  cctx = arc_mkcctx(c);
  arc_emit3(c, cctx, ienv, INT2FIX(3), INT2FIX(0), INT2FIX(2));
  arc_emit(c, cctx, itrue);
  arc_emit2(c, cctx, iste, INT2FIX(0), INT2FIX(3));
  arc_emit2(c, cctx, ilde, INT2FIX(0), INT2FIX(2));
  arc_emit(c, cctx, ipush);
  arc_emit2(c, cctx, ilde, INT2FIX(0), INT2FIX(1));
  arc_emit(c, cctx, ipush);
  arc_emit2(c, cctx, ilde, INT2FIX(0), INT2FIX(0));
  arc_emit(c, cctx, ipush);
  arc_emit3(c, cctx, ienv, INT2FIX(3), INT2FIX(0), INT2FIX(1));
  arc_emit2(c, cctx, ilde, INT2FIX(0), INT2FIX(0));
  arc_emit(c, cctx, ipush);
  arc_emit2(c, cctx, ilde, INT2FIX(0), INT2FIX(1));
  arc_emit(c, cctx, iadd);
  arc_emit(c, cctx, ipush);
  arc_emit2(c, cctx, ilde, INT2FIX(1), INT2FIX(2));
  arc_emit(c, cctx, iadd);
  arc_emit2(c, cctx, iste, INT2FIX(0), INT2FIX(2));
  arc_emit(c, cctx, ihlt);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  thr = arc_mkthread(c);
  XCALL(clos, INT2FIX(1), INT2FIX(2), INT2FIX(3));
  fail_unless(*__arc_getenv(c, thr, 0, 0) == INT2FIX(3));
  fail_unless(*__arc_getenv(c, thr, 0, 1) == INT2FIX(2));
  fail_unless(*__arc_getenv(c, thr, 0, 2) == INT2FIX(8));
  fail_unless(*__arc_getenv(c, thr, 1, 0) == INT2FIX(1));
  fail_unless(*__arc_getenv(c, thr, 1, 1) == INT2FIX(2));
  fail_unless(*__arc_getenv(c, thr, 1, 2) == INT2FIX(3));
  fail_unless(*__arc_getenv(c, thr, 1, 3) == CTRUE);
  fail_unless(*__arc_getenv(c, thr, 1, 4) == CUNBOUND);
}
END_TEST

START_TEST(test_jmp)
{
  value cctx, code, clos;
  value thr;
  int ptr, ptr2;

  cctx = arc_mkcctx(c);
  arc_emit1(c, cctx, ildi, INT2FIX(1234));
  ptr = FIX2INT(CCTX_VCPTR(cctx));
  arc_emit1(c, cctx, ijmp, 0);
  arc_emit1(c, cctx, ildi, INT2FIX(5678));
  ptr2 = FIX2INT(CCTX_VCPTR(cctx));
  VINDEX(CCTX_VCODE(cctx), ptr+1) = INT2FIX(ptr2 - ptr);
  arc_emit(c, cctx, ihlt);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  thr = arc_mkthread(c);
  XCALL0(clos);
  fail_unless(TQUANTA(thr) == QUANTA-2);
  fail_unless(TVALR(thr) == INT2FIX(1234));
}
END_TEST

START_TEST(test_true)
{
  value cctx, code, clos;
  value thr;

  cctx = arc_mkcctx(c);
  arc_emit(c, cctx, itrue);
  arc_emit(c, cctx, ihlt);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  thr = arc_mkthread(c);
  XCALL0(clos);
  fail_unless(TQUANTA(thr) == QUANTA-1);
  fail_unless(TSTATE(thr) == Trelease);
  fail_unless(TYPE(TVALR(thr)) == T_TRUE);
  fail_unless(TVALR(thr) == CTRUE);
}
END_TEST

START_TEST(test_nil)
{
  value cctx, code, clos;
  value thr;

  cctx = arc_mkcctx(c);
  arc_emit(c, cctx, inil);
  arc_emit(c, cctx, ihlt);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  thr = arc_mkthread(c);
  XCALL0(clos);
  fail_unless(TQUANTA(thr) == QUANTA-1);
  fail_unless(TSTATE(thr) == Trelease);
  fail_unless(TYPE(TVALR(thr)) == T_NIL);
  fail_unless(NIL_P(TVALR(thr)));
}
END_TEST

START_TEST(test_hlt)
{
  value cctx, code, clos;
  value thr;

  cctx = arc_mkcctx(c);
  arc_emit(c, cctx, ihlt);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  thr = arc_mkthread(c);
  XCALL0(clos);
  fail_unless(TQUANTA(thr) == QUANTA);
  fail_unless(TSTATE(thr) == Trelease);
  fail_unless(TVALR(thr) == clos);
}
END_TEST

START_TEST(test_add)
{
  value cctx, code, clos;
  value thr;

  cctx = arc_mkcctx(c);
  arc_emit1(c, cctx, ildi, INT2FIX(2));
  arc_emit(c, cctx, ipush);
  arc_emit1(c, cctx, ildi, INT2FIX(3));
  arc_emit(c, cctx, iadd);
  arc_emit(c, cctx, ihlt);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  thr = arc_mkthread(c);
  XCALL0(clos);
  fail_unless(TQUANTA(thr) == QUANTA-4);
  fail_unless(TSTATE(thr) == Trelease);
  fail_unless(TVALR(thr) == INT2FIX(5));
}
END_TEST

START_TEST(test_sub)
{
  value cctx, code, clos;
  value thr;

  cctx = arc_mkcctx(c);
  arc_emit1(c, cctx, ildi, INT2FIX(2));
  arc_emit(c, cctx, ipush);
  arc_emit1(c, cctx, ildi, INT2FIX(3));
  arc_emit(c, cctx, isub);
  arc_emit(c, cctx, ihlt);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  thr = arc_mkthread(c);
  XCALL0(clos);
  fail_unless(TQUANTA(thr) == QUANTA-4);
  fail_unless(TSTATE(thr) == Trelease);
  fail_unless(TVALR(thr) == INT2FIX(-1));
}
END_TEST

START_TEST(test_mul)
{
  value cctx, code, clos;
  value thr;

  cctx = arc_mkcctx(c);
  arc_emit1(c, cctx, ildi, INT2FIX(2));
  arc_emit(c, cctx, ipush);
  arc_emit1(c, cctx, ildi, INT2FIX(3));
  arc_emit(c, cctx, imul);
  arc_emit(c, cctx, ihlt);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  thr = arc_mkthread(c);
  XCALL0(clos);
  fail_unless(TQUANTA(thr) == QUANTA-4);
  fail_unless(TSTATE(thr) == Trelease);
  fail_unless(TVALR(thr) == INT2FIX(6));
}
END_TEST

START_TEST(test_div)
{
  value cctx, code, clos;
  value thr;

  cctx = arc_mkcctx(c);
  arc_emit1(c, cctx, ildi, INT2FIX(4));
  arc_emit(c, cctx, ipush);
  arc_emit1(c, cctx, ildi, INT2FIX(2));
  arc_emit(c, cctx, idiv);
  arc_emit(c, cctx, ihlt);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  thr = arc_mkthread(c);
  XCALL0(clos);
  fail_unless(TQUANTA(thr) == QUANTA-4);
  fail_unless(TSTATE(thr) == Trelease);
  fail_unless(TVALR(thr) == INT2FIX(2));
}
END_TEST

START_TEST(test_cons)
{
  value cctx, code, clos;
  value thr;

  cctx = arc_mkcctx(c);
  arc_emit1(c, cctx, ildi, INT2FIX(4));
  arc_emit(c, cctx, ipush);
  arc_emit(c, cctx, inil);
  arc_emit(c, cctx, icons);
  arc_emit(c, cctx, ihlt);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  thr = arc_mkthread(c);
  XCALL0(clos);
  fail_unless(TQUANTA(thr) == QUANTA-4);
  fail_unless(TSTATE(thr) == Trelease);
  fail_unless(TYPE(TVALR(thr)) == T_CONS);
  fail_unless(car(TVALR(thr)) == INT2FIX(4));
  fail_unless(NIL_P(cdr(TVALR(thr))));
}
END_TEST

START_TEST(test_car)
{
  value cctx, code, clos;
  value thr;

  cctx = arc_mkcctx(c);
  arc_emit1(c, cctx, ildi, INT2FIX(4));
  arc_emit(c, cctx, ipush);
  arc_emit1(c, cctx, ildi, INT2FIX(8));
  arc_emit(c, cctx, icons);
  arc_emit(c, cctx, icar);
  arc_emit(c, cctx, ihlt);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  thr = arc_mkthread(c);
  XCALL0(clos);
  fail_unless(TQUANTA(thr) == QUANTA-5);
  fail_unless(TSTATE(thr) == Trelease);
  fail_unless(TYPE(TVALR(thr)) == T_FIXNUM);
  fail_unless(TVALR(thr) == INT2FIX(4));
}
END_TEST

START_TEST(test_cdr)
{
  value cctx, code, clos;
  value thr;

  cctx = arc_mkcctx(c);
  arc_emit1(c, cctx, ildi, INT2FIX(4));
  arc_emit(c, cctx, ipush);
  arc_emit1(c, cctx, ildi, INT2FIX(8));
  arc_emit(c, cctx, icons);
  arc_emit(c, cctx, icdr);
  arc_emit(c, cctx, ihlt);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  thr = arc_mkthread(c);
  XCALL0(clos);
  fail_unless(TQUANTA(thr) == QUANTA-5);
  fail_unless(TSTATE(thr) == Trelease);
  fail_unless(TYPE(TVALR(thr)) == T_FIXNUM);
  fail_unless(TVALR(thr) == INT2FIX(8));
}
END_TEST
START_TEST(test_scar)
{
  value cctx, code, clos;
  value thr;
  int lptr;

  cctx = arc_mkcctx(c);
  lptr = arc_literal(c, cctx, cons(c, INT2FIX(4), INT2FIX(8)));
  arc_emit1(c, cctx, ildl, INT2FIX(lptr));
  arc_emit(c, cctx, ipush);
  arc_emit1(c, cctx, ildi, INT2FIX(2));
  arc_emit(c, cctx, iscar);
  arc_emit(c, cctx, ihlt);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  thr = arc_mkthread(c);
  XCALL0(clos);
  fail_unless(TQUANTA(thr) == QUANTA-4);
  fail_unless(TSTATE(thr) == Trelease);
  fail_unless(TYPE(TVALR(thr)) == T_FIXNUM);
  fail_unless(TVALR(thr) == INT2FIX(2));
  fail_unless(TYPE(CODE_LITERAL(CLOS_CODE(TFUNR(thr)), lptr)) == T_CONS);
  fail_unless(car(CODE_LITERAL(CLOS_CODE(TFUNR(thr)), lptr)) == INT2FIX(2));
  fail_unless(cdr(CODE_LITERAL(CLOS_CODE(TFUNR(thr)), lptr)) == INT2FIX(8));
}
END_TEST

START_TEST(test_scdr)
{
  value cctx, code, clos;
  value thr;
  int lptr;

  cctx = arc_mkcctx(c);
  lptr = arc_literal(c, cctx, cons(c, INT2FIX(4), INT2FIX(8)));
  arc_emit1(c, cctx, ildl, INT2FIX(lptr));
  arc_emit(c, cctx, ipush);
  arc_emit1(c, cctx, ildi, INT2FIX(2));
  arc_emit(c, cctx, iscdr);
  arc_emit(c, cctx, ihlt);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  thr = arc_mkthread(c);
  XCALL0(clos);
  fail_unless(TQUANTA(thr) == QUANTA-4);
  fail_unless(TSTATE(thr) == Trelease);
  fail_unless(TYPE(TVALR(thr)) == T_FIXNUM);
  fail_unless(TVALR(thr) == INT2FIX(2));
  fail_unless(TYPE(CODE_LITERAL(CLOS_CODE(TFUNR(thr)), lptr)) == T_CONS);
  fail_unless(car(CODE_LITERAL(CLOS_CODE(TFUNR(thr)), lptr)) == INT2FIX(4));
  fail_unless(cdr(CODE_LITERAL(CLOS_CODE(TFUNR(thr)), lptr)) == INT2FIX(2));
}
END_TEST



int main(void)
{
  int number_failed;
  Suite *s = suite_create("Arcueid's Virtual Machine");
  TCase *tc_vm = tcase_create("Arcueid's Virtual Machine");
  SRunner *sr;

  c = &cc;
  arc_init(c);

  tcase_add_test(tc_vm, test_nop);
  tcase_add_test(tc_vm, test_ldi);
  tcase_add_test(tc_vm, test_ldl);
  tcase_add_test(tc_vm, test_ldg);
  tcase_add_test(tc_vm, test_stg);
  tcase_add_test(tc_vm, test_push);
  tcase_add_test(tc_vm, test_pop);
  tcase_add_test(tc_vm, test_envs);
  tcase_add_test(tc_vm, test_jmp);
  tcase_add_test(tc_vm, test_true);
  tcase_add_test(tc_vm, test_nil);
  tcase_add_test(tc_vm, test_hlt);
  tcase_add_test(tc_vm, test_add);
  tcase_add_test(tc_vm, test_sub);
  tcase_add_test(tc_vm, test_mul);
  tcase_add_test(tc_vm, test_div);
  tcase_add_test(tc_vm, test_cons);
  tcase_add_test(tc_vm, test_car);
  tcase_add_test(tc_vm, test_cdr);
  tcase_add_test(tc_vm, test_scar);
  tcase_add_test(tc_vm, test_scdr);

  suite_add_tcase(s, tc_vm);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}


