/* 
  Copyright (C) 2017 Rafael R. Sevilla

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
#include "../config.h"
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include <math.h>
#include <stdio.h>
#include "../src/arcueid.h"
#include "../src/alloc.h"
#include "../src/gc.h"

static value rootval;

static void test_markroots(arc *c, void (*marker)(struct arc *, value))
{
  marker(c, rootval);
}

START_TEST(test_gc_cons)
{
  value r;
  arc cc;
  arc *c = &cc;
  struct gc_ctx *gcc;
  struct GChdr *ptr;
  int count, i;

  arc_init(c);
  c->markroots = test_markroots;
  gcc = (struct gc_ctx *)c->gc_ctx;

  r = cons(c, CNIL, CNIL);
  count = 0;
  for (ptr = gcc->gcobjects; ptr; ptr = ptr->next)
    count++;
  ck_assert_int_eq(count, 1);
  ck_assert(arc_type(r) == &__arc_cons_t);
  ck_assert(car(r) == CNIL);
  ck_assert(cdr(r) == CNIL);

  rootval = r;
  while (__arc_gc(c) == 0)
    ;
  while (__arc_gc(c) == 0)
    ;
  while (__arc_gc(c) == 0)
    ;
  count = 0;
  for (ptr = gcc->gcobjects; ptr; ptr = ptr->next)
    count++;
  ck_assert_int_eq(count, 1);
  ck_assert(arc_type(r) == &__arc_cons_t);
  ck_assert(car(r) == CNIL);
  ck_assert(cdr(r) == CNIL);

  rootval = CNIL;
  while (__arc_gc(c) == 0)
    ;
  while (__arc_gc(c) == 0)
    ;
  while (__arc_gc(c) == 0)
    ;
  count = 0;
  for (ptr = gcc->gcobjects; ptr; ptr = ptr->next)
    count++;
  ck_assert_int_eq(count, 0);

  /* Make a longer, branched cons */
  r = cons(c, cons(c, INT2FIX(1), CNIL),
	   cons(c, INT2FIX(2), cons(c, INT2FIX(3), CNIL)));
  count = 0;
  for (ptr = gcc->gcobjects; ptr; ptr = ptr->next)
    count++;
  ck_assert_int_eq(count, 4);

  rootval = r;
  for (i=0; i<10; i++) {
    while (__arc_gc(c) == 0)
      ;
  }
  count = 0;
  for (ptr = gcc->gcobjects; ptr; ptr = ptr->next)
    count++;
  ck_assert_int_eq(count, 4);

  rootval = CNIL;
  for (i=0; i<10; i++) {
    while (__arc_gc(c) == 0)
      ;
  }
  count = 0;
  for (ptr = gcc->gcobjects; ptr; ptr = ptr->next)
    count++;
  ck_assert_int_eq(count, 0);
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Garbage Collection");
  TCase *tc_gc = tcase_create("Garbage Collection");
  SRunner *sr;

  tcase_add_test(tc_gc, test_gc_cons);

  suite_add_tcase(s, tc_gc);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
