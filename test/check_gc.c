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
#include <math.h>
#include <stdio.h>
#include "../src/arcueid.h"
#include "../src/alloc.h"
#include "../config.h"

arc cc;
arc *c;

#define MAX_BIBOP 512

struct mm_ctx {
  /* The BiBOP free list */
  Bhdr *bibop_fl[MAX_BIBOP+1];

  /* The allocated list */
  Bhdr *alloc_head;
  int nprop;
};


/* Do nothing: root marking is done by individual tests */
static void markroots(arc *c)
{
}

START_TEST(test_gc_cons)
{
  value list=CNIL;
  int i, count;
  struct mm_ctx *mmctx = (struct mm_ctx *)c->alloc_ctx;
  Bhdr *ptr;

  for (i=0; i<4; i++)
    list=cons(c, INT2FIX(i), list);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 4);

  __arc_markprop(c, list);
  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 4);

  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 0);

}
END_TEST


int main(void)
{
  int number_failed;
  Suite *s = suite_create("Garbage Collection");
  TCase *tc_gc = tcase_create("Garbage Collection");
  SRunner *sr;

  c = &cc;
  arc_set_memmgr(c);
  c->markroots = markroots;

  tcase_add_test(tc_gc, test_gc_cons);

  suite_add_tcase(s, tc_gc);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
