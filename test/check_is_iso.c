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
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include <stdio.h>
#include "../src/arcueid.h"
#include "../config.h"

arc cc;
arc *c;

/* Do nothing: root marking is done by individual tests */
static void markroots(arc *c)
{
}

START_TEST(test_cons)
{
  value list1, list2;

  list1 = cons(c, INT2FIX(1), cons(c, INT2FIX(2), cons(c, INT2FIX(3), CNIL)));
  list2 = cons(c, INT2FIX(1), cons(c, INT2FIX(2), cons(c, INT2FIX(3), CNIL)));

  fail_unless(arc_is(c, list1, list1) == CTRUE);
  fail_unless(arc_is(c, list2, list2) == CTRUE);
  fail_unless(arc_is(c, list1, list2) == CNIL);
  fail_unless(arc_is(c, list2, list1) == CNIL);

  fail_unless(arc_iso(c, list1, list1, CNIL, CNIL) == CTRUE);
  fail_unless(arc_iso(c, list2, list2, CNIL, CNIL) == CTRUE);
  fail_unless(arc_iso(c, list1, list2, CNIL, CNIL) == CTRUE);
  fail_unless(arc_iso(c, list2, list1, CNIL, CNIL) == CTRUE);
}
END_TEST

START_TEST(test_circ_cons)
{
  value list1, list2, list3;

  list1 = cons(c, INT2FIX(1), cons(c, INT2FIX(2), cons(c, INT2FIX(3), CNIL)));
  list2 = cons(c, INT2FIX(1), cons(c, INT2FIX(2), cons(c, INT2FIX(3), CNIL)));
  list3 = cons(c, INT2FIX(1), cons(c, INT2FIX(4), cons(c, INT2FIX(3), CNIL)));

  car(cdr(cdr(list1))) = list1;
  car(cdr(cdr(list2))) = list2;
  fail_unless(arc_iso(c, list1, list2, CNIL, CNIL) == CTRUE);
  fail_unless(arc_iso(c, list2, list1, CNIL, CNIL) == CTRUE);
  fail_unless(arc_iso(c, list1, list3, CNIL, CNIL) == CNIL);
  fail_unless(arc_iso(c, list2, list3, CNIL, CNIL) == CNIL);
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("is/iso Behaviour");
  TCase *tc_is_iso = tcase_create("is/iso Behaviour");
  SRunner *sr;

  c = &cc;
  arc_set_memmgr(c);
  arc_init_datatypes(c);
  c->markroots = markroots;

  tcase_add_test(tc_is_iso, test_cons);
  tcase_add_test(tc_is_iso, test_circ_cons);

  suite_add_tcase(s, tc_is_iso);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
