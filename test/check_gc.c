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
  /* After three epochs, the object should have been collected */
  count = 0;
  for (ptr = gcc->gcobjects; ptr; ptr = ptr->next)
    count++;
  ck_assert_int_eq(count, 0);
}
END_TEST

START_TEST(test_gc_write_barrier)
{
  value r, b;
  arc cc;
  arc *c = &cc;
  struct gc_ctx *gcc;
  struct GChdr *ptr, *ptrb;
  int count, i;

  arc_init(c);
  /* Set the markroots function to our test_markroots function above */
  c->markroots = test_markroots;
  gcc = (struct gc_ctx *)c->gc_ctx;

  /* Create a test cons */
  r = cons(c, cons(c, INT2FIX(1), CNIL),
	   cons(c, INT2FIX(2), cons(c, INT2FIX(3), CNIL)));
  /* Place it in the root set */
  rootval = r;
  count = 0;
  for (ptr = gcc->gcobjects; ptr; ptr = ptr->next)
    count++;
  ck_assert_int_eq(count, 4);
  /* Since the object is part of the root set, we can go through as
     many garbage collection cycles as needed and it should not be
     collected. */
  for (i=0; i<10; i++) {
    while (__arc_gc(c) == 0)
      ;
  }
  /* Now, we remove the element linked to the car. It should
     be marked with the propagator colour. */
  b = car(r);
  V2GCH(ptr, r);
  ck_assert_int_eq(ptr->colour, PROPAGATOR);
  V2GCH(ptrb, b);
  ck_assert_int_eq(ptrb->colour, gcc->marker);
  scar(c, r, CNIL);
  ck_assert(NILP(car(r)));
  ck_assert_int_eq(ptrb->colour, PROPAGATOR);
  /* After one epoch, the colour should move forwards */
  while (__arc_gc(c) == 0)
    ;
  ck_assert_int_eq(ptr->colour, PROPAGATOR);
  ck_assert_int_eq(ptrb->colour, gcc->marker);
  while (__arc_gc(c) == 0)
    ;
  ck_assert_int_eq(ptr->colour, PROPAGATOR);
  ck_assert_int_eq(ptrb->colour, gcc->sweeper);
  while (__arc_gc(c) == 0)
    ;
  /* The unlinked piece should have been collected */
  count = 0;
  for (ptr = gcc->gcobjects; ptr; ptr = ptr->next)
    count++;
  ck_assert_int_eq(count, 3);
  /* Unlink the object and collect everything */
  rootval = CNIL;
  for (i=0; i<3; i++) {
    while (__arc_gc(c) == 0)
      ;
  }
  count = 0;
  for (ptr = gcc->gcobjects; ptr; ptr = ptr->next)
    count++;
  ck_assert_int_eq(count, 0);

  /* Next try to do the same for scdr */
  r = cons(c, cons(c, INT2FIX(1), CNIL),
	   cons(c, INT2FIX(2), cons(c, INT2FIX(3), CNIL)));
  rootval = r;
  for (i=0; i<10; i++) {
    while (__arc_gc(c) == 0)
      ;
  }

  /* Now we try to do scdr */
  V2GCH(ptr, r);
  ck_assert_int_eq(ptr->colour, PROPAGATOR);
  b = cdr(r);
  V2GCH(ptrb, b);
  ck_assert_int_eq(ptrb->colour, gcc->marker);
  scdr(c, r, CNIL);
  ck_assert(NILP(cdr(r)));
  ck_assert_int_eq(ptrb->colour, PROPAGATOR);
  while (__arc_gc(c) == 0)
    ;
  ck_assert_int_eq(ptr->colour, PROPAGATOR);
  ck_assert_int_eq(ptrb->colour, gcc->marker);
  while (__arc_gc(c) == 0)
    ;
  ck_assert_int_eq(ptr->colour, PROPAGATOR);
  ck_assert_int_eq(ptrb->colour, gcc->sweeper);
  while (__arc_gc(c) == 0)
    ;
  ck_assert_int_eq(ptr->colour, PROPAGATOR);
  count = 0;
  /* Only the root cons and the one linked to the car should remain
     because they were part of the root */
  for (ptr = gcc->gcobjects; ptr; ptr = ptr->next)
    count++;
  ck_assert_int_eq(count, 2);
  rootval = CNIL;
  for (i=0; i<3; i++) {
    while (__arc_gc(c) == 0)
      ;
  }
  count = 0;
  for (ptr = gcc->gcobjects; ptr; ptr = ptr->next)
    count++;
  ck_assert_int_eq(count, 0);  
}
END_TEST

START_TEST(test_gc_flonum)
{
  value r;
  arc cc;
  arc *c = &cc;
  struct gc_ctx *gcc;
  struct GChdr *ptr;
  int count;

  arc_init(c);
  /* Set the markroots function to our test_markroots function above */
  c->markroots = test_markroots;
  gcc = (struct gc_ctx *)c->gc_ctx;

  /* Create a single flonum */
  r = arc_flonum_new(c, 3.1416);

  /* Test that there should be one item in the list of all allocated
     objects and that it should be a flonum and the flonum which
     we made. */
  count = 0;
  for (ptr = gcc->gcobjects; ptr; ptr = ptr->next)
    count++;
  ck_assert_int_eq(count, 1);
  ck_assert(arc_type(r) == &__arc_flonum_t);
  ck_assert(fabs(arc_flonum(r) - 3.1416) < 1e-6);

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

  /* Since the flonum we created above is part of the root set, it
     should not have been collected and should still be present */
  count = 0;
  for (ptr = gcc->gcobjects; ptr; ptr = ptr->next)
    count++;
  ck_assert_int_eq(count, 1);
  ck_assert(arc_type(r) == &__arc_flonum_t);
  ck_assert(fabs(arc_flonum(r) - 3.1416) - 1e-6);

  /* Remove the above created flonum from the root set */
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
}
END_TEST

#define TEST_VEC_LEN 8

START_TEST(test_gc_vector)
{
  value vec;
  arc cc;
  arc *c = &cc;
  struct gc_ctx *gcc;
  struct GChdr *ptr;
  int count, i;

  arc_init(c);
  /* Set the markroots function to our test_markroots function above */
  c->markroots = test_markroots;
  gcc = (struct gc_ctx *)c->gc_ctx;

  /* Create a single vector */
  vec = arc_vector_new(c, TEST_VEC_LEN);

  /* Test that there should be one item in the list of all allocated
     objects and that it should be a cons and the cons which we made. */
  count = 0;
  for (ptr = gcc->gcobjects; ptr; ptr = ptr->next)
    count++;
  ck_assert_int_eq(count, 1);
  ck_assert(arc_type(vec) == &__arc_vector_t);
  ck_assert_int_eq(VLEN(vec), TEST_VEC_LEN);
  for (i=0; i<TEST_VEC_LEN; i++)
    ck_assert(NILP(VIDX(vec, i)));

  /* Add some things into the vector, conses and flonums */
  for (i=0; i<TEST_VEC_LEN; i++) {
    SVIDX(c, vec, i, (i & 0x1) == 0 ? cons(c, CNIL, CNIL) :
	  arc_flonum_new(c, (double)i));
  }

  /* Place it in the root set */
  rootval = vec;
  /* Call __arc_gc repeatedly until the end of an epoch. After three
     epochs elapse, any object not in the root set which was mutator
     colour ought to be collected and freed.  However, since the
     object is part of the root set, it should have propagator colour
     at the end of every epoch. */
  V2GCH(ptr, vec);
  while (__arc_gc(c) == 0)
    ;
  ck_assert_int_eq(ptr->colour, PROPAGATOR);
  while (__arc_gc(c) == 0)
    ;
  ck_assert_int_eq(ptr->colour, PROPAGATOR);
  while (__arc_gc(c) == 0)
    ;
  ck_assert_int_eq(ptr->colour, PROPAGATOR);

  /* Since the vector we created above is part of the root set, it
     should not have been collected and should still be present, along with
     all the values it has in it. */
  count = 0;
  for (ptr = gcc->gcobjects; ptr; ptr = ptr->next)
    count++;
  ck_assert_int_eq(count, TEST_VEC_LEN + 1);
  ck_assert(arc_type(vec) == &__arc_vector_t);
  /* Add some things into the vector, conses and flonums */
  for (i=0; i<TEST_VEC_LEN; i++) {
    value r = VIDX(vec, i);
    if ((i & 0x1) == 0) {
      ck_assert(arc_type(r) == &__arc_cons_t);
    } else {
      ck_assert(arc_type(r) == &__arc_flonum_t);
      ck_assert(fabs(arc_flonum(r) - (double)i) < 1e-6);
    }
  }

  /* Remove the above created flonum from the root set */
  rootval = CNIL;
  /* Again, let three epochs elapse. */
  while (__arc_gc(c) == 0)
    ;
  /* After one epoch, the colour should change to marker */
  V2GCH(ptr, vec);
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
}
END_TEST

START_TEST(test_gc_wref)
{
  value wr, f, p, wr2;
  arc cc;
  arc *c = &cc;
  struct gc_ctx *gcc;
  struct GChdr *ptr;
  int count;

  arc_init(c);
  /* Set the markroots function to our test_markroots function above */
  c->markroots = test_markroots;
  gcc = (struct gc_ctx *)c->gc_ctx;

  /* Create a single flonum */
  f = arc_flonum_new(c, 3.1416);
  /* Create a weak reference to this flonum */
  wr = arc_wref_new(c, f);
  /* Create a cons cell containing the weak reference and the flonum */
  p = cons(c, wr, f);
  V2GCH(ptr, f);
  ck_assert(ptr->wref == wr);
  /* Try to create a second weak reference to f. This should be the
     same as the first, since weak references should be singletons. */
  wr2 = arc_wref_new(c, f);
  ck_assert(wr == wr2);
  
  /* Place this cons at the root */
  rootval = p;
  /* Call __arc_gc repeatedly until the end of an epoch. After three
     epochs elapse, any object not in the root set which was mutator
     colour ought to be collected and freed.  However, since the
     object is part of the root set, it should have propagator colour
     at the end of every epoch. */
  while (__arc_gc(c) == 0)
    ;
  while (__arc_gc(c) == 0)
    ;
  while (__arc_gc(c) == 0)
    ;

  /* All three objects above must still remain at the end of all this
     and must remain unchanged. */
  count = 0;
  for (ptr = gcc->gcobjects; ptr; ptr = ptr->next)
    count++;
  ck_assert_int_eq(count, 3);
  ck_assert(arc_type(f) == &__arc_flonum_t);
  ck_assert(fabs(arc_flonum(f) - 3.1416) - 1e-6);
  ck_assert(arc_type(wr) == &__arc_wref_t);
  ck_assert(arc_wrefv(wr) == f);
  ck_assert(arc_type(p) == &__arc_cons_t);

  /* Next, we set the cdr of p to nil, so that the only reference to
     f is via the weak reference wr. */
  scdr(c, p, CNIL);
  /* Now, three GC epochs later f should become garbage despite the
     reference. */

  while (__arc_gc(c) == 0)
    ;
  ck_assert(arc_wrefv(wr) == f);
  while (__arc_gc(c) == 0)
    ;
  ck_assert(arc_wrefv(wr) == f);

  while (__arc_gc(c) == 0)
    ;
  ck_assert(NILP(arc_wrefv(wr)));

  /* Now, we clear out the old weakref. */
  scar(c, p, CNIL);
  while (__arc_gc(c) == 0)
    ;
  while (__arc_gc(c) == 0)
    ;
  while (__arc_gc(c) == 0)
    ;
  /* Create a new wref and flonum */
  f = arc_flonum_new(c, 3.1416);
  V2GCH(ptr, f);
  ck_assert(NILP(ptr->wref));
  /* Create a weak reference to this flonum */
  wr = arc_wref_new(c, f);
  ck_assert(ptr->wref == wr);
  scar(c, p, wr);
  scdr(c, p, f);
  /* Next, remove the weak reference from the root so that it gets
     garbage collected. */
  scar(c, p, CNIL);
  ck_assert(!NILP(ptr->wref));
  while (__arc_gc(c) == 0)
    ;
  while (__arc_gc(c) == 0)
    ;
  while (__arc_gc(c) == 0)
    ;
  /* The weak reference should be cleared out at the end. */
  ck_assert(NILP(ptr->wref));
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Garbage Collection");
  TCase *tc_gc = tcase_create("Garbage Collection");
  SRunner *sr;

  tcase_add_test(tc_gc, test_gc_cons);
  tcase_add_test(tc_gc, test_gc_write_barrier);
  tcase_add_test(tc_gc, test_gc_flonum);
  tcase_add_test(tc_gc, test_gc_vector);
  tcase_add_test(tc_gc, test_gc_wref);

  suite_add_tcase(s, tc_gc);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
