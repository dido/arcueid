/* 
  Copyright (C) 2011 Rafael R. Sevilla

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
#include "../src/symbols.h"
#include "../config.h"

arc c;
extern unsigned long long gcepochs;
extern Bhdr *__arc_get_heap_start(void);

START_TEST(test_sym_gc)
{
  int i;
  unsigned long long oldepoch;
  value compose, new_compose, randsym, rsname;
  value propcons, protsym, psname;
  char *cstr;

  /* symbols built into the reader are automatically part of the
     root set and should not be garbage collected. */
  compose = ARC_BUILTIN(&c, S_COMPOSE);
  new_compose = arc_intern_cstr(&c, "compose");
  fail_unless(compose == new_compose);

  /* this symbol should be garbage collected */
  randsym = arc_intern_cstr(&c, "randsym");
  rsname = arc_sym2name(&c, randsym);
  fail_unless(TYPE(rsname) == T_STRING);
  cstr = alloca(sizeof(char)*(FIX2INT(arc_strutflen(&c, rsname)) + 1));
  arc_str2cstr(&c, rsname, cstr);
  fail_unless(strcmp(cstr, "randsym") == 0);

  /* Create a second symbol referenced in a cons cell that is kept
     marked as a propagator.  Both the cons cell and the symbol
     should survive all garbage collection epochs. */
  protsym = arc_intern_cstr(&c, "protsym");
  psname = arc_sym2name(&c, protsym);
  fail_unless(TYPE(psname) == T_STRING);
  cstr = alloca(sizeof(char)*(FIX2INT(arc_strutflen(&c, psname)) + 1));
  arc_str2cstr(&c, psname, cstr);
  fail_unless(strcmp(cstr, "protsym") == 0);
  propcons = cons(&c, protsym, CNIL);
  MARKPROP(propcons);

  oldepoch = gcepochs;
  for (i=0; i<2; i++) {
    while (gcepochs == oldepoch) {
      c.rungc(&c);
    }
    new_compose = arc_intern_cstr(&c, "compose");
    fail_unless(compose == new_compose);
    fail_unless(protsym == arc_intern_cstr(&c, "protsym"));
    MARKPROP(propcons);
    oldepoch = gcepochs;
  }

  /* after two GC epochs, randsym should still exist */
  rsname = arc_sym2name(&c, randsym);
  fail_unless(TYPE(rsname) == T_STRING);
  cstr = alloca(sizeof(char)*(FIX2INT(arc_strutflen(&c, rsname)) + 1));
  arc_str2cstr(&c, rsname, cstr);
  fail_unless(strcmp(cstr, "randsym") == 0);
  while (gcepochs == oldepoch) {
    c.rungc(&c);
  }
  oldepoch = gcepochs;

  /* randsym should no longer exist in the symbol table after the third
     epoch. */
  fail_unless(arc_sym2name(&c, randsym) == CUNBOUND);
  /* but compose should be */
  new_compose = arc_intern_cstr(&c, "compose");
  fail_unless(compose == new_compose);

  /* as should protsym */
  fail_unless(protsym == arc_intern_cstr(&c, "protsym"));

  /* but once we stop putting the propagator mark on propcons, protsym
     should also disappear from the symbol table after three more GC
     epochs elapse. */
  for (i=0; i<3; i++) {
    while (gcepochs == oldepoch) {
      c.rungc(&c);
    }
    oldepoch = gcepochs;
  }
  fail_unless(arc_sym2name(&c, protsym) == CUNBOUND);

  /* but compose should still remain */
  new_compose = arc_intern_cstr(&c, "compose");
  fail_unless(compose == new_compose);
  psname = arc_sym2name(&c, compose);
  fail_unless(TYPE(psname) == T_STRING);
  cstr = alloca(sizeof(char)*(FIX2INT(arc_strutflen(&c, psname)) + 1));
  arc_str2cstr(&c, psname, cstr);
  fail_unless(strcmp(cstr, "compose") == 0);

  /* and all the builtin symbols bound should also be */
  for (i=0; i<S_THE_END; i++)
    fail_if(arc_sym2name(&c, ARC_BUILTIN(&c, i)) == CUNBOUND);

  for (i=0; i<30; i++) {
    while (gcepochs == oldepoch) {
      c.rungc(&c);
    }
    oldepoch = gcepochs;
  }

  /* and after however many GC epochs elapse */
  for (i=0; i<S_THE_END; i++)
    fail_if(arc_sym2name(&c, ARC_BUILTIN(&c, i)) == CUNBOUND);

}
END_TEST

START_TEST(test_gc)
{
  value list=CNIL, list2=CNIL;
  value listsym1, listsym2, lss1, lss2, hash1, hash2, vec1, vec2;
  int i, count, startcount;
  Bhdr *b;
  unsigned long long oldepoch;

  /* Create a two small lists */
  for (i=0; i<4; i++)
    list=cons(&c, INT2FIX(i), list);
  for (i=4; i<8; i++) 
    list2=cons(&c, INT2FIX(i), list);

  /* Add a symbol to each list */
  lss1 = arc_mkstringc(&c, "list1");
  lss2 = arc_mkstringc(&c, "list2");
  listsym1 = arc_intern(&c, lss1);
  listsym2 = arc_intern(&c, lss2);
  list = cons(&c, listsym1, list);
  list2 = cons(&c, listsym2, list2);

  /* Add a string to each list */
  list = cons(&c, arc_mkstringc(&c, "foo"), list);
  list2 = cons(&c, arc_mkstringc(&c, "bar"), list);

  /* Add a flonum to each list */
  list = cons(&c, arc_mkflonum(&c, 1.234), list);
  list2 = cons(&c, arc_mkflonum(&c, 5.678), list);

#ifdef HAVE_GMP_H
  /* Add a bignum and a rational to each list */
  list = cons(&c, arc_mkbignuml(&c, FIXNUM_MAX + 1), list);
  list2 = cons(&c, arc_mkbignuml(&c, FIXNUM_MAX + 2), list);

  list = cons(&c, arc_mkrationall(&c, 1, 4), list);
  list2 = cons(&c, arc_mkrationall(&c, 3, 4), list);
#endif

  /* Add a character to each list */
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

  /* Count all allocated blocks */
  count = 0;
  for (b = __arc_get_heap_start(); b; b = b->next) {
    if (b->magic == MAGIC_A)
      count++;
  }
  startcount = count;

  /* Mark [list] with a propagator, but not [list2] */
  oldepoch = gcepochs;
  MARKPROP(list);
  while (gcepochs == oldepoch) {
    c.rungc(&c);
  }

  oldepoch = gcepochs;
  MARKPROP(list);
  while (gcepochs == oldepoch) {
    c.rungc(&c);
  }

  oldepoch = gcepochs;
  MARKPROP(list);
  while (gcepochs == oldepoch) {
    c.rungc(&c);
  }

  /* After three epochs of the garbage collector, [list2], whose head was
     never marked as a propagator, should have been garbage collected. */
  count = 0;
  for (b = __arc_get_heap_start(); b; b = b->next) {
    if (b->magic == MAGIC_A)
      count++;
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
  Suite *s = suite_create("Garbage Collection");
  TCase *tc_gc = tcase_create("Garbage Collection");
  SRunner *sr;

  arc_set_memmgr(&c);
  arc_init_reader(&c);

  tcase_add_test(tc_gc, test_sym_gc);
  tcase_add_test(tc_gc, test_gc);

  suite_add_tcase(s, tc_gc);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
