/* Do not change this comment -- the tests depend on it! */
/* 
  Copyright (C) 2013 Rafael R. Sevilla

  This file is part of Arcueid

  Arcueid is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; either version 3 of the
  License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <check.h>
#include "../src/arcueid.h"
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

extern void __arc_print_string(arc *c, value ppstr);

arc *c, cc;

#define QUANTA 1048576

#define CPUSH_(val) CPUSH(c->curthread, val)

#define XCALL0(clos) do {				\
    TQUANTA(c->curthread) = QUANTA;			\
    TVALR(c->curthread) = clos;				\
    TARGC(c->curthread) = 0;				\
    __arc_thr_trampoline(c, c->curthread, TR_FNAPP);	\
  } while (0)

#define XCALL(fname, ...) do {				\
    TVALR(c->curthread) = arc_mkaff(c, fname, CNIL);	\
    TARGC(c->curthread) = NARGS(__VA_ARGS__);		\
    FOR_EACH(CPUSH_, __VA_ARGS__);			\
    __arc_thr_trampoline(c, c->curthread, TR_FNAPP);	\
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
  cctx = TVALR(c->curthread);			\
  code = arc_cctx2code(c, cctx);		\
  clos = arc_mkclos(c, code, CNIL);		\
  XCALL0(clos);					\
  ret = TVALR(c->curthread)

START_TEST(test_do)
{
  value ret, cctx, code, clos;

  TEST("(do)");
  fail_unless(ret == CNIL);

  TEST("(do 1)");
  fail_unless(ret == INT2FIX(1));

  TEST("(do 1 2)");
  fail_unless(ret == INT2FIX(2));

  TEST("(do 1 2 3)");
  fail_unless(ret == INT2FIX(3));
}
END_TEST

static void errhandler(arc *c, value str)
{
  fprintf(stderr, "Error\n");
  __arc_print_string(c, str);
  abort();
}

int main(void)
{
  int number_failed;
  Suite *s = suite_create("arc.arc");
  TCase *tc_arc = tcase_create("arc.arc");
  SRunner *sr;
  value ret, cctx, code, clos;

  c = &cc;
  c->errhandler = errhandler;
  arc_init(c);
  c->curthread = arc_mkthread(c);
  /* Load arc.arc into our system */
  TEST("(assign initload (infile \"./arc.arc\"))");
  if (NIL_P(ret)) {
    fprintf(stderr, "failed to load arc.arc");
    return(EXIT_FAILURE);
  }
  for (;;) {
    TEST("(assign sexpr (sread initload nil))");
    if (ret == CNIL)
      break;
    TEST("(eval sexpr)");
    c->gc(c);
  }
  TEST("(close initload)");
  c->gc(c);

  tcase_add_test(tc_arc, test_do);

  suite_add_tcase(s, tc_arc);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

