/* 
  Copyright (C) 2010 Rafael R. Sevilla

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
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA
  02110-1301 USA.
*/
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include <math.h>
#include <stdio.h>
#include "../src/arcueid.h"
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

extern Hhdr *__arc_get_heap_start(void);

arc c;

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
  h = __arc_get_heap_start();
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
  h = __arc_get_heap_start();
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
  h = __arc_get_heap_start();
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

  h = __arc_get_heap_start();
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
  value listsym1, listsym2, lss1, lss2, hash1, hash2, vec1, vec2;
  int i, count, startcount;
  Hhdr *h;
  Bhdr *b;
  unsigned long long oldepoch;

  arc_init_reader(&c);

  /* Create a two small lists */
  for (i=0; i<4; i++)
    list=cons(&c, INT2FIX(i), list);
  for (i=4; i<8; i++) 
    list2=cons(&c, INT2FIX(i), list);

  /* Add a symbol at the end of each list */
  lss1 = arc_mkstringc(&c, "list1");
  lss2 = arc_mkstringc(&c, "list2");
  listsym1 = arc_intern(&c, lss1);
  listsym2 = arc_intern(&c, lss2);
  list = cons(&c, listsym1, list);
  list2 = cons(&c, listsym2, list2);

  /* Add a string to the end of each list */
  list = cons(&c, arc_mkstringc(&c, "foo"), list);
  list2 = cons(&c, arc_mkstringc(&c, "bar"), list);

  /* Add a flonum to the end of each list */
  list = cons(&c, arc_mkflonum(&c, 1.234), list);
  list2 = cons(&c, arc_mkflonum(&c, 5.678), list);

#ifdef HAVE_GMP_H
  /* Add a bignum and a rational to the end of each list */
  list = cons(&c, arc_mkbignuml(&c, FIXNUM_MAX + 1), list);
  list2 = cons(&c, arc_mkbignuml(&c, FIXNUM_MAX + 2), list);

  list = cons(&c, arc_mkrationall(&c, 1, 4), list);
  list2 = cons(&c, arc_mkrationall(&c, 3, 4), list);
#endif

  /* Add a character to the end of each list */
  list = cons(&c, arc_mkchar(&c, 0x86df), list);
  list2 = cons(&c, arc_mkchar(&c, 0x9f8d), list2);

  /* Add a vector to each list, and populate it with fixnums and a string */
  vec1 = arc_mkvector(&c, 10);
  vec2 = arc_mkvector(&c, 10);

  for (i=0; i<9; i++) {
    VINDEX(vec1, i) = INT2FIX(i);
    VINDEX(vec2, i) = INT2FIX(i);
  }

  VINDEX(vec1, 9) = arc_mkstringc(&c, "foo");
  VINDEX(vec2, 9) = arc_mkstringc(&c, "bar");
  list = cons(&c, vec1, list);
  list2 = cons(&c, vec2, list2);

  /* Create two hash tables, put a fixnum and a string mapping in
     each, and add them to each list. */
  hash1 = arc_mkhash(&c, 4);
  hash2 = arc_mkhash(&c, 4);
  arc_hash_insert(&c, hash1, INT2FIX(1), INT2FIX(2));
  arc_hash_insert(&c, hash1, INT2FIX(3), arc_mkstringc(&c, "three"));
  arc_hash_insert(&c, hash2, INT2FIX(5), INT2FIX(6));
  arc_hash_insert(&c, hash2, INT2FIX(7), arc_mkstringc(&c, "seven"));
  list = cons(&c, hash1, list);
  list2 = cons(&c, hash2, list2);

  /* Cons up 128 more values to the lists just to make them longer
     and ensure that the incremental collection works properly. */
  for (i=0; i<128; i++) {
    list=cons(&c, INT2FIX(i), list);
    list2=cons(&c, INT2FIX(i), list2);
  }

  count = 0;
  for (h = __arc_get_heap_start(); h; h = h->next) {
    for (b = (Bhdr *)((char *)h + sizeof(Hhdr)); b->magic != MAGIC_E;
	 b = B2NB(b)) {
      if (b->magic == MAGIC_A)
	count++;
    }
  }
  startcount = count;

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
     never marked as a propagator, should have been garbage collected. */
  count = 0;
  for (h = __arc_get_heap_start(); h; h = h->next) {
    for (b = (Bhdr *)((char *)h + sizeof(Hhdr)); b->magic != MAGIC_E;
	 b = B2NB(b)) {
      if (b->magic == MAGIC_A)
	count++;
    }
  }

  /*  printf("count = %d\n", startcount - count); */

  /* There are 150 objects (154 if we have the bignum and the
     rational) represented by list2, and they must all be collected. */
#ifdef HAVE_GMP_H
  fail_unless(startcount - count == 154);
#else
  fail_unless(startcount - count == 150);
#endif
  /* check if the symbol is still there */
  fail_unless(arc_sym2name(&c, listsym2) == CUNBOUND);
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Memory Allocation");
  TCase *tc_alloc = tcase_create("Allocation");
  TCase *tc_gc = tcase_create("Garbage Collection");
  SRunner *sr;

  arc_set_memmgr(&c);

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
