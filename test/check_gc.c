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
  /* Set the markroots function to our test_markroots function above */
  c->markroots = test_markroots;
  gcc = (struct gc_ctx *)c->gc_ctx;

  /* Create a single cons cell */
  r = cons(c, CNIL, CNIL);
  /* When the cons is created, the colour should be mutator colour */
  V2GCH(ptr, r);
  ck_assert_int_eq(ptr->colour, gcc->mutator);

  count = 0;
  /* Test that there should be one item in the list of all allocated
     objects and that it should be a cons and the cons which we made. */
  for (ptr = gcc->gcobjects; ptr; ptr = ptr->next)
    count++;
  ck_assert_int_eq(count, 1);
  ck_assert(arc_type(r) == &__arc_cons_t);
  ck_assert(car(r) == CNIL);
  ck_assert(cdr(r) == CNIL);

  /* Place it in the root set */
  rootval = r;
  /* Call __arc_gc repeatedly until the end of an epoch. After three
     epochs elapse, any object not in the root set which was mutator
     colour ought to be collected and freed.  However, since the
     object is part of the root set, it should have propagator colour
     at the end of every epoch. */
  V2GCH(ptr, r);
  while (__arc_gc(c) == 0)
    ;
  ck_assert_int_eq(ptr->colour, PROPAGATOR);
  while (__arc_gc(c) == 0)
    ;
  ck_assert_int_eq(ptr->colour, PROPAGATOR);
  while (__arc_gc(c) == 0)
    ;
  ck_assert_int_eq(ptr->colour, PROPAGATOR);

  /* Since the cons we created above is part of the root set, it
     should not have been collected and should still be present */
  count = 0;
  for (ptr = gcc->gcobjects; ptr; ptr = ptr->next)
    count++;
  ck_assert_int_eq(count, 1);
  ck_assert(arc_type(r) == &__arc_cons_t);
  ck_assert(car(r) == CNIL);
  ck_assert(cdr(r) == CNIL);

  /* Remove the above created cons from the root set */
  rootval = CNIL;
  /* Again, let three epochs elapse. */
  while (__arc_gc(c) == 0)
    ;
  /* After one epoch, the colour should change to marker */
  V2GCH(ptr, r);
  ck_assert_int_eq(ptr->colour, gcc->marker);
  /* After two epochs, the colour should change to sweeper */
  while (__arc_gc(c) == 0)
    ;
  ck_assert_int_eq(ptr->colour, gcc->sweeper);
  /* After three epochs, the object should be collected */
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

  /* Place it in the root set */
  rootval = r;
  /* Since the object is part of the root set, we can go through as
     many garbage collection cycles as needed and it should not be
     collected. */
  for (i=0; i<10; i++) {
    while (__arc_gc(c) == 0)
      ;
  }
  count = 0;
  for (ptr = gcc->gcobjects; ptr; ptr = ptr->next)
    count++;
  ck_assert_int_eq(count, 4);

  /* Remove it from the root set and let three epochs elapse */
  rootval = CNIL;
  for (i=0; i<3; i++) {
    while (__arc_gc(c) == 0)
      ;
  }
  /* After three epochs, the object should have been colected */
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
