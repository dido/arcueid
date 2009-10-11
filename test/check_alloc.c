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

extern Hhdr *__carc_get_heap_start(void);

carc c;

START_TEST(test_alloc)
{
  char *ptr1, *ptr2, *ptr3, *ptr4, *ptr5, *ptr6, *ptr7, *ptr8;
  int i;
  Hhdr *h;
  Bhdr *b;

  c.minexp = 8192;		/* for testing */
 
  /* Allocate blocks of memory */
  ptr1 = c.get_block(&c, 992);
  fail_if(ptr1 == NULL);
  memset(ptr1, 1, 992);

  ptr2 = c.get_block(&c, 992);
  fail_if(ptr2 == NULL);
  memset(ptr2, 2, 992);

  ptr3 = c.get_block(&c, 992);
  fail_if(ptr3 == NULL);
  memset(ptr3, 3, 992);

  ptr4 = c.get_block(&c, 992);
  fail_if(ptr4 == NULL);
  memset(ptr4, 4, 992);

  ptr5 = c.get_block(&c, 992);
  fail_if(ptr5 == NULL);
  memset(ptr5, 5, 992);

  ptr6 = c.get_block(&c, 992);
  fail_if(ptr6 == NULL);
  memset(ptr6, 6, 992);

  ptr7 = c.get_block(&c, 992);
  fail_if(ptr7 == NULL);
  memset(ptr7, 7, 992);

  ptr8 = c.get_block(&c, 992);
  fail_if(ptr8 == NULL);
  memset(ptr8, 8, 992);

  memset(ptr1, 1, 992);
  memset(ptr2, 2, 992);
  memset(ptr3, 3, 992);
  memset(ptr4, 4, 992);
  memset(ptr5, 5, 992);
  memset(ptr6, 6, 992);
  memset(ptr7, 7, 992);
  memset(ptr8, 8, 992);

  for (i=0; i<992; i++) {
    fail_unless(*(ptr1 + i) == 1);
    fail_unless(*(ptr2 + i) == 2);
    fail_unless(*(ptr3 + i) == 3);
    fail_unless(*(ptr4 + i) == 4);
    fail_unless(*(ptr5 + i) == 5);
    fail_unless(*(ptr6 + i) == 6);
    fail_unless(*(ptr7 + i) == 7);
    fail_unless(*(ptr8 + i) == 8);
  }

  /* At the end of this, we should be able to see all eight
     memory blocks in a single heap chunk. */
  h = __carc_get_heap_start();
  b = (Bhdr *)((char *)h + sizeof(Hhdr));
  fail_unless(h->next == NULL);
  i=0;
  while (b->magic != MAGIC_E) {
    i++;
    fail_unless(b->magic == MAGIC_A);
    fail_unless(b->size == 992);
    b = B2NB(b);
  }
  fail_unless(i == 8);

  /* Free the blocks in an order designed to put the freeing
     routine through its paces. */
  c.free_block(&c, ptr6);
  c.free_block(&c, ptr1);
  c.free_block(&c, ptr3);
  c.free_block(&c, ptr2);
  c.free_block(&c, ptr4);
  c.free_block(&c, ptr8);
  c.free_block(&c, ptr5);
  c.free_block(&c, ptr7);

  /* This freeing should leave the heap with a single chunk
     of free memory which should be exactly 8192-32 = 8160 bytes
     in size */
  h = __carc_get_heap_start();
  b = (Bhdr *)((char *)h + sizeof(Hhdr));
  fail_unless(h->next == NULL);
  fail_unless(b->magic == MAGIC_F);
  fail_unless(b->u.next == NULL); /* no other free blocks besides me */
  fail_unless(b->size == 8160);
  b = B2NB(b);
  fail_unless(b->magic == MAGIC_E); /* make sure it's the last block */

  ptr1 = c.get_block(&c, 992);
  fail_if(ptr1 == NULL);
  ptr2 = c.get_block(&c, 992);
  fail_if(ptr2 == NULL);
  ptr3 = c.get_block(&c, 992);
  fail_if(ptr3 == NULL);
  ptr4 = c.get_block(&c, 992);
  fail_if(ptr4 == NULL);
  ptr5 = c.get_block(&c, 992);
  fail_if(ptr5 == NULL);
  ptr6 = c.get_block(&c, 992);
  fail_if(ptr6 == NULL);
  ptr7 = c.get_block(&c, 992);
  fail_if(ptr7 == NULL);
  ptr8 = c.get_block(&c, 992);
  fail_if(ptr8 == NULL);

  /* At the end of this, we should again be able to see all eight
     memory blocks in a single heap chunk. */
  h = __carc_get_heap_start();
  b = (Bhdr *)((char *)h + sizeof(Hhdr));
  fail_unless(h->next == NULL);
  i=0;
  while (b->magic != MAGIC_E) {
    i++;
    fail_unless(b->magic == MAGIC_A);
    fail_unless(b->size == 992);
    b = B2NB(b);
  }
  fail_unless(i == 8);

  /* Another permutation */
  c.free_block(&c, ptr8);
  c.free_block(&c, ptr1);
  c.free_block(&c, ptr4);
  c.free_block(&c, ptr6);
  c.free_block(&c, ptr5);
  c.free_block(&c, ptr2);
  c.free_block(&c, ptr7);
  c.free_block(&c, ptr3);

  h = __carc_get_heap_start();
  b = (Bhdr *)((char *)h + sizeof(Hhdr));
  fail_unless(h->next == NULL);
  fail_unless(b->magic == MAGIC_F);
  fail_unless(b->u.next == NULL); /* no other free blocks besides me */
  fail_unless(b->size == 8160);
  b = B2NB(b);
  fail_unless(b->magic == MAGIC_E); /* make sure it's the last block */

  /* Try to allocate something big */
  ptr1 = c.get_block(&c, 16384);
  fail_if(ptr1 == NULL);
  c.free_block(&c, ptr1);
}
END_TEST

extern unsigned long long gcepochs;

START_TEST(test_gc)
{
  value list=CNIL, list2=CNIL;
  int i, count;
  Hhdr *h;
  Bhdr *b;
  unsigned long long oldepoch;

  /* Create a two small lists */
  for (i=0; i<4; i++)
    list=cons(&c, INT2FIX(i), list);
  for (i=4; i<8; i++) 
    list2=cons(&c, INT2FIX(i), list);

  count = 0;
  for (h = __carc_get_heap_start(); h; h = h->next) {
    for (b = (Bhdr *)((char *)h + sizeof(Hhdr)); b->magic != MAGIC_E;
	 b = B2NB(b)) {
      if (b->magic == MAGIC_A)
	count++;
    }
  }
  fail_unless(count == 8);
  /* Mark [list] with a propagator, but not [list2] */
  D2B(b, (void *)list);
  oldepoch = gcepochs;
  b->color = 3;	       /* mark [list] with propagator color */
  while (gcepochs == oldepoch) {
    c.rungc(&c);
  }

  oldepoch = gcepochs;
  b->color = 3;	       /* mark [list] with propagator color */
  while (gcepochs == oldepoch) {
    c.rungc(&c);
  }

  oldepoch = gcepochs;
  b->color = 3;	       /* mark [list] with propagator color */
  while (gcepochs == oldepoch) {
    c.rungc(&c);
  }

  /* After three epochs of the garbage collector, [list2], whose head was
     never marked as a propagator, should have been garbage collected,
     and the number of allocated blocks in memory must be only ten now. */
  count = 0;
  for (h = __carc_get_heap_start(); h; h = h->next) {
    for (b = (Bhdr *)((char *)h + sizeof(Hhdr)); b->magic != MAGIC_E;
	 b = B2NB(b)) {
      if (b->magic == MAGIC_A)
	count++;
    }
  }
  fail_unless(count == 4);

}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Memory Allocation");
  TCase *tc_alloc = tcase_create("Allocation");
  TCase *tc_gc = tcase_create("Garbage Collection");
  SRunner *sr;

  carc_set_memmgr(&c);

  tcase_add_test(tc_alloc, test_size_rounding);
  tcase_add_test(tc_alloc, test_alloc);
  tcase_add_test(tc_gc, test_gc);

  suite_add_tcase(s, tc_alloc);
  suite_add_tcase(s, tc_gc);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
