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
#include <check.h>
#include "../src/arcueid.h"
#include "../src/vmengine.h"
#include "../src/arith.h"

arc cc;
arc *c;

#define CPUSH_(val) CPUSH(thr, val)

#define XCALL(fname, ...) do {			\
    TVALR(thr) = arc_mkaff(c, fname, CNIL);	\
    TARGC(thr) = NARGS(__VA_ARGS__);		\
    FOR_EACH(CPUSH_, __VA_ARGS__);		\
    __arc_thr_trampoline(c, thr, TR_FNAPP);	\
  } while (0)

START_TEST(test_hash_simple)
{
  value hash, thr;

  thr = arc_mkthread(c);
  hash = arc_mkhash(c, 8);

  /* fixnum keys */
  arc_hash_insert(c, hash, INT2FIX(1), INT2FIX(2));
  fail_unless(arc_hash_lookup(c, hash, INT2FIX(1)) == INT2FIX(2));
  XCALL(arc_xhash_lookup, hash, INT2FIX(1));
  fail_unless(TVALR(thr) == INT2FIX(2));

  /* string keys */
  arc_hash_insert(c, hash, arc_mkstringc(c, "foo"), INT2FIX(3));
  fail_unless(arc_hash_lookup(c, hash, arc_mkstringc(c, "foo")) == INT2FIX(3));
  XCALL(arc_xhash_lookup, hash, arc_mkstringc(c, "foo"));
  fail_unless(TVALR(thr) == INT2FIX(3));

  /* symbol keys */
  arc_hash_insert(c, hash, arc_intern_cstr(c, "bar"), INT2FIX(4));
  fail_unless(arc_hash_lookup(c, hash, arc_intern_cstr(c, "bar")) == INT2FIX(4));
  XCALL(arc_xhash_lookup, hash, arc_intern_cstr(c, "bar"));
  fail_unless(TVALR(thr) == INT2FIX(4));
  XCALL(arc_xhash_lookup, hash, arc_intern_cstr(c, "bar"));
  fail_unless(TVALR(thr) == INT2FIX(4));

  /* character keys */
  arc_hash_insert(c, hash, arc_mkchar(c, 'a'), INT2FIX(5));
  fail_unless(arc_hash_lookup(c, hash, arc_mkchar(c, 'a')) == INT2FIX(5));
  XCALL(arc_xhash_lookup, hash, arc_mkchar(c, 'a'));
  fail_unless(TVALR(thr) == INT2FIX(5));

  /* flonum keys */
  arc_hash_insert(c, hash, arc_mkflonum(c, 3.14), INT2FIX(6));
  fail_unless(arc_hash_lookup(c, hash, arc_mkflonum(c, 3.14)) == INT2FIX(6));
  XCALL(arc_xhash_lookup, hash, arc_mkflonum(c, 3.14));
  fail_unless(TVALR(thr) == INT2FIX(6));

  /* complex keys */
  arc_hash_insert(c, hash, arc_mkcomplex(c, 3.14 + I*2.71), INT2FIX(7));
  fail_unless(arc_hash_lookup(c, hash, arc_mkcomplex(c, 3.14 + I*2.71)) == INT2FIX(7));
  XCALL(arc_xhash_lookup, hash, arc_mkcomplex(c, 3.14 + I*2.71));
  fail_unless(TVALR(thr) == INT2FIX(7));

#ifdef HAVE_GMP_H
  {
    value bn;

    /* bignum keys */
    bn = arc_mkbignuml(c, 0L);
    mpz_set_str(REPBNUM(bn), "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", 10);
    arc_hash_insert(c, hash, bn, INT2FIX(8));
    fail_unless(arc_hash_lookup(c, hash, bn) == INT2FIX(8));
    XCALL(arc_xhash_lookup, hash, bn);
    fail_unless(TVALR(thr) == INT2FIX(8));

    /* rational keys */
    arc_hash_insert(c, hash, arc_mkrationall(c, 1, 2), INT2FIX(9));
    fail_unless(arc_hash_lookup(c, hash, arc_mkrationall(c, 1, 2)) == INT2FIX(9));
    XCALL(arc_xhash_lookup, hash, arc_mkrationall(c, 1, 2));
    fail_unless(TVALR(thr) == INT2FIX(9));
  }
#endif

  /* Test xhash_insert */
  XCALL(arc_xhash_insert, hash, INT2FIX(2), INT2FIX(5));
  fail_unless(arc_hash_lookup(c, hash, INT2FIX(2)) == INT2FIX(5));
  XCALL(arc_xhash_lookup, hash, INT2FIX(2));
  fail_unless(TVALR(thr) == INT2FIX(5));

  /* Try overwriting some of the bindings */
  arc_hash_insert(c, hash, INT2FIX(1), INT2FIX(12));
  fail_unless(arc_hash_lookup(c, hash, INT2FIX(1)) == INT2FIX(12));
  XCALL(arc_xhash_lookup, hash, INT2FIX(1));
  fail_unless(TVALR(thr) == INT2FIX(12));
}
END_TEST

START_TEST(test_hash_cons_keys)
{
  value hash, thr;
  value list1 = cons(c, INT2FIX(1), cons(c, INT2FIX(2), cons(c, INT2FIX(3), CNIL)));
  value list2 = cons(c, INT2FIX(2), cons(c, INT2FIX(3), cons(c, INT2FIX(4), CNIL)));

  /* make list2 a circular list */
  car(cdr(cdr(list2))) = list2;

  thr = arc_mkthread(c);
  hash = arc_mkhash(c, 8);

  XCALL(arc_xhash_insert, hash, list1, INT2FIX(20));
  XCALL(arc_xhash_lookup, hash, list1);
  fail_unless(TVALR(thr) == INT2FIX(20));

  XCALL(arc_xhash_insert, hash, list2, INT2FIX(21));
  XCALL(arc_xhash_lookup, hash, list2);
  fail_unless(TVALR(thr) == INT2FIX(21));
}
END_TEST

START_TEST(test_hash_vector_keys)
{
  value hash, thr;
  value vec = arc_mkvector(c, 3);

  VINDEX(vec, 0) = INT2FIX(1);
  VINDEX(vec, 1) = INT2FIX(2);
  VINDEX(vec, 2) = INT2FIX(3);

  thr = arc_mkthread(c);
  hash = arc_mkhash(c, 8);

  XCALL(arc_xhash_insert, hash, vec, INT2FIX(22));
  XCALL(arc_xhash_lookup, hash, vec);
  fail_unless(TVALR(thr) == INT2FIX(22));
}
END_TEST

START_TEST(test_hash_hash_keys)
{
  value hash, thr;
  value hashkey = arc_mkhash(c, 8);

  arc_hash_insert(c, hashkey, INT2FIX(1), INT2FIX(2));
  thr = arc_mkthread(c);
  hash = arc_mkhash(c, 8);

  XCALL(arc_xhash_insert, hash, hashkey, INT2FIX(23));
  XCALL(arc_xhash_lookup, hash, hashkey);
  fail_unless(TVALR(thr) == INT2FIX(23));
}
END_TEST

#define EXPANSION_LIMIT 65536

START_TEST(test_hash_expansion)
{
  value hash;
  int i;

  /* start off with a ridiculously small hash size. Two bits is only 4
     elements, so when we try to squeeze 65536 values into it, the size
     would have doubled at least 17 times.  All the values should still
     be available after. */
  hash = arc_mkhash(c, 2);
  for (i=0; i<EXPANSION_LIMIT; i++)
    arc_hash_insert(c, hash, INT2FIX(i), INT2FIX(i+1));
  for (i=0; i<EXPANSION_LIMIT; i++)
    fail_unless(arc_hash_lookup(c, hash, INT2FIX(i)) == INT2FIX(i+1));

  fail_unless(arc_hash_length(c, hash) == EXPANSION_LIMIT);
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Hash Tables");
  TCase *tc_hash = tcase_create("Hash Tables");
  SRunner *sr;

  c = &cc;
  arc_init(c);

  tcase_add_test(tc_hash, test_hash_simple);
  tcase_add_test(tc_hash, test_hash_cons_keys);
  tcase_add_test(tc_hash, test_hash_vector_keys);
  tcase_add_test(tc_hash, test_hash_hash_keys);
  tcase_add_test(tc_hash, test_hash_expansion);

  suite_add_tcase(s, tc_hash);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

