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
  tcase_add_test(tc_vm, test_hlt);

  suite_add_tcase(s, tc_vm);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}


