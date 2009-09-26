/* 
  Copyright (C) 2009 Rafael R. Sevilla

  This file is part of CArc

  CArc is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA
  02110-1301 USA.
*/
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include <math.h>
#include <stdio.h>
#include "../src/carc.h"
#include "../src/alloc.h"
#include "../config.h"

START_TEST(test_size_rounding)
{
  size_t ns;
  int i;

  for (i=1; i<=16; i++) {
    ROUNDSIZE(ns, i);
    fail_unless(ns == 16);
  }
  for (; i<=32; i++) {
    ROUNDSIZE(ns, i);
    fail_unless(ns == 32);
  }
}
END_TEST

START_TEST(test_alloc1)
{
  value v1, v2, v3, v4;
  carc c;
  char *ptr1, *ptr2, *ptr3;
  int i;

  carc_set_memmgr(&c);
 
  /* Allocate blocks of memory */
  ptr1 = c.get_block(&c, 1024);
  fail_if(ptr1 == NULL);

  memset(ptr1, 1, 1024);

  ptr2 = c.get_block(&c, 1024);
  fail_if(ptr2 == NULL);
  memset(ptr2, 2, 1024);

  memset(ptr1, 1, 1024);
  memset(ptr2, 2, 1024);

  for (i=0; i<1024; i++) {
    fail_unless(*(ptr1 + i) == 1);
    fail_unless(*(ptr2 + i) == 2);
  }

  /* Do a large allocation */
  ptr3 = c.get_block(&c, DFL_MIN_EXP*4);
  fail_if(ptr3 == NULL);

  v1 = c.get_cell(&c);
  v2 = c.get_cell(&c);
  v3 = c.get_cell(&c);
  v4 = c.get_cell(&c);
  /* These should be descending addresses since the allocator
     allocates blocks of memory by carving out the higher addresses
     from a memory block. */
  fail_unless(v4 < v3);
  fail_unless(v3 < v2);
  fail_unless(v2 < v1);

}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Memory Allocation");
  TCase *tc_alloc = tcase_create("Allocation");
  TCase *tc_gc = tcase_create("Garbage Collection");
  SRunner *sr;

  tcase_add_test(tc_alloc, test_size_rounding);
  tcase_add_test(tc_alloc, test_alloc1);

  suite_add_tcase(s, tc_alloc);
  suite_add_tcase(s, tc_gc);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
