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
#include "../src/arith.h"
#include "../src/builtins.h"
#include "../src/vmengine.h"
#include "../config.h"

arc cc;
arc *c;

#define MAX_BIBOP 512

struct mm_ctx {
  /* The BiBOP free list */
  Bhdr *bibop_fl[MAX_BIBOP+1];
  /* The actual BiBOP pages */
  Bhdr *bibop_pages[MAX_BIBOP+1];

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
  value r;
  struct mm_ctx *mmctx = (struct mm_ctx *)c->alloc_ctx;
  Bhdr *ptr;
  int count;

  r = cons(c, CNIL, CNIL);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 1);
  fail_unless(TYPE(r) == T_CONS);
  fail_unless(NIL_P(car(r)));
  fail_unless(NIL_P(cdr(r)));

  __arc_markprop(c, r);
  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(TYPE(r) == T_CONS);
  fail_unless(NIL_P(car(r)));
  fail_unless(NIL_P(cdr(r)));

  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 0);
}
END_TEST

#ifdef HAVE_GMP_H

START_TEST(test_gc_bignum)
{
  value bn;
  struct mm_ctx *mmctx = (struct mm_ctx *)c->alloc_ctx;
  Bhdr *ptr;
  int count;
  char *str;

  bn = arc_mkbignuml(c, 0);
  mpz_set_str(REPBNUM(bn), "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", 10);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 1);
  fail_unless(TYPE(bn) == T_BIGNUM);
  str = alloca(mpz_sizeinbase(REPBNUM(bn), 10) + 2);
  mpz_get_str(str, 10, REPBNUM(bn));
  fail_unless(strcmp(str, "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000") == 0);

  __arc_markprop(c, bn);
  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 1);
  mpz_get_str(str, 10, REPBNUM(bn));
  fail_unless(strcmp(str, "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000") == 0);

  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 0);
}
END_TEST

START_TEST(test_gc_rational)
{
  value q;
  struct mm_ctx *mmctx = (struct mm_ctx *)c->alloc_ctx;
  Bhdr *ptr;
  int count;

  q = arc_mkrationall(c, 1, 2);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 1);
  fail_unless(TYPE(q) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REPRAT(q), 1, 2) == 0);

  __arc_markprop(c, q);
  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 1);
  fail_unless(TYPE(q) == T_RATIONAL);
  fail_unless(mpq_cmp_si(REPRAT(q), 1, 2) == 0);

  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 0);
}
END_TEST

#endif

START_TEST(test_gc_flonum)
{
  value r;
  struct mm_ctx *mmctx = (struct mm_ctx *)c->alloc_ctx;
  Bhdr *ptr;
  int count;

  r = arc_mkflonum(c, 1.0);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 1);
  fail_unless(TYPE(r) == T_FLONUM);
  fail_unless(REPFLO(r) == 1.0);

  __arc_markprop(c, r);
  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 1);
  fail_unless(TYPE(r) == T_FLONUM);
  fail_unless(REPFLO(r) == 1.0);

  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 0);
}
END_TEST

START_TEST(test_gc_complex)
{
  value z;
  struct mm_ctx *mmctx = (struct mm_ctx *)c->alloc_ctx;
  Bhdr *ptr;
  int count;

  z = arc_mkcomplex(c, 1.0 + I*2.0);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 1);
  fail_unless(TYPE(z) == T_COMPLEX);
  fail_unless(creal(REPCPX(z)) == 1.0);
  fail_unless(cimag(REPCPX(z)) == 2.0);

  __arc_markprop(c, z);
  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 1);
  fail_unless(TYPE(z) == T_COMPLEX);
  fail_unless(creal(REPCPX(z)) == 1.0);
  fail_unless(cimag(REPCPX(z)) == 2.0);

  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 0);
}
END_TEST

START_TEST(test_gc_char)
{
  value ch;
  struct mm_ctx *mmctx = (struct mm_ctx *)c->alloc_ctx;
  Bhdr *ptr;
  int count;

  ch = arc_mkchar(c, 'a');
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 1);
  fail_unless(TYPE(ch) == T_CHAR);
  fail_unless(arc_char2rune(c, ch) == 'a');

  __arc_markprop(c, ch);
  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 1);
  fail_unless(TYPE(ch) == T_CHAR);
  fail_unless(arc_char2rune(c, ch) == 'a');

  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 0);
}
END_TEST

START_TEST(test_gc_string)
{
  value str;
  struct mm_ctx *mmctx = (struct mm_ctx *)c->alloc_ctx;
  Bhdr *ptr;
  int count;

  str = arc_mkstringc(c, "foo");
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 1);
  fail_unless(TYPE(str) == T_STRING);
  fail_unless(arc_strcmp(c, str, arc_mkstringc(c, "foo")) == 0);

  /* This should collect the string we created to do
     the comparison! */
  __arc_markprop(c, str);
  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 1);
  fail_unless(TYPE(str) == T_STRING);
  fail_unless(arc_strcmp(c, str, arc_mkstringc(c, "foo")) == 0);

  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 0);
}
END_TEST

START_TEST(test_gc_symbol)
{
  value sym, conscell;
  struct mm_ctx *mmctx = (struct mm_ctx *)c->alloc_ctx;
  Bhdr *ptr;
  int count;

  /* This is a bit tricky */
  c->lastsym = 0;
  c->symtable = arc_mkwtable(c, ARC_HASHBITS);
  c->rsymtable = arc_mkwtable(c, ARC_HASHBITS);
  c->builtins = arc_mkvector(c, BI_last+1);
  VINDEX(c->builtins, BI_syms) = arc_mkvector(c, S_THE_END);
  ARC_BUILTIN(c, S_NIL) = arc_intern_cstr(c, "nil");
  ARC_BUILTIN(c, S_T) = arc_intern_cstr(c, "t");

  /* The above should have produced four items:
     1. The symbol table
     2. The reverse symbol table
     3. The vector for the symbol table data
     4. The vector for the reverse symbol table
     5. The builtins vector
     6. The builtin symbols vector
     7. The string representation of nil
     8. The string representation of t
     9. The hash bucket in symtable for nil
     10. The hash bucket in rsymtable for nil
     11. The hash bucket in symtable for t
     12. The hash bucket in rsymtable for t
   */
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 12);

  /* Interning a symbol should produce three additional allocated
     blocks:

     1. The string representation of the symbol
     2. The hash bucket in the symbol table for it
     3. The hash bucket in the reverse symbol table for it
  */
  sym = arc_intern_cstr(c, "foo");
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 15);
  fail_unless(TYPE(sym) == T_SYMBOL);

  /* Storing the symbol in a cons brings the total to 16 */
  conscell = cons(c, sym, CNIL);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 16);

  /* Marking the cons should maintain the symbol */
  __arc_markprop(c, conscell);
  __arc_markprop(c, c->symtable);
  __arc_markprop(c, c->rsymtable);
  __arc_markprop(c, c->builtins);
  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 16);
  fail_unless(TYPE(sym) == T_SYMBOL);
  /* The symbol's representation should not change */
  fail_unless(sym == arc_intern_cstr(c, "foo"));

  /* Now, if we do NOT mark the cons cell containing it, that should
     remove the symbol's buckets from the tables in addition to removing
     the cons cell, bringing our allocated total down to 12 again. */
  __arc_markprop(c, c->symtable);
  __arc_markprop(c, c->rsymtable);
  __arc_markprop(c, c->builtins);
  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 12);
  /* Interning the same string once again should produce a different
     representation from the old (now invalid) one. */
  fail_if(sym == arc_intern_cstr(c, "foo"));

  /* new intern of foo should have produced three new items */
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 15);

  /* Marking nothing should clear all the allocated memory completely */
  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 0);
}
END_TEST

START_TEST(test_gc_vector)
{
  value vec;
  struct mm_ctx *mmctx = (struct mm_ctx *)c->alloc_ctx;
  Bhdr *ptr;
  int count, i;

  vec = arc_mkvector(c, 10);
  for (i=0; i<10; i++)
    VINDEX(vec, i) = INT2FIX(i);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 1);
  fail_unless(TYPE(vec) == T_VECTOR);
  for (i=0; i<10; i++)
    fail_unless(VINDEX(vec, i) == INT2FIX(i));

  __arc_markprop(c, vec);
  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 1);
  fail_unless(TYPE(vec) == T_VECTOR);
  for (i=0; i<10; i++)
    fail_unless(VINDEX(vec, i) == INT2FIX(i));

  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 0);
}
END_TEST


START_TEST(test_gc_table)
{
  value tbl;
  struct mm_ctx *mmctx = (struct mm_ctx *)c->alloc_ctx;
  Bhdr *ptr;
  int count, i;

  /* Making a hash produces two blocks */
  tbl = arc_mkhash(c, ARC_HASHBITS);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 2);

  /* each one produces an additional block for the hash bucket */
  for (i=0; i<10; i++)
    arc_hash_insert(c, tbl, INT2FIX(i), INT2FIX(i+1));
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 12);
  fail_unless(TYPE(tbl) == T_TABLE);
  for (i=0; i<10; i++)
    fail_unless(arc_hash_lookup(c, tbl, INT2FIX(i)) == INT2FIX(i+1));

  __arc_markprop(c, tbl);
  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 12);
  fail_unless(TYPE(tbl) == T_TABLE);
  for (i=0; i<10; i++)
    fail_unless(arc_hash_lookup(c, tbl, INT2FIX(i)) == INT2FIX(i+1));

  /* Try removing the bindings */
  for (i=0; i<10; i++)
    arc_hash_delete(c, tbl, INT2FIX(i));
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 12);
  fail_unless(TYPE(tbl) == T_TABLE);
  for (i=0; i<10; i++)
    fail_unless(arc_hash_lookup(c, tbl, INT2FIX(i)) == CUNBOUND);

  /* garbage collection should remove the dangling buckets */
  __arc_markprop(c, tbl);
  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 2);
  fail_unless(TYPE(tbl) == T_TABLE);
  for (i=0; i<10; i++)
    fail_unless(arc_hash_lookup(c, tbl, INT2FIX(i)) == CUNBOUND);

  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 0);
}
END_TEST

START_TEST(test_gc_tagged)
{
  value tagged;
  struct mm_ctx *mmctx = (struct mm_ctx *)c->alloc_ctx;
  Bhdr *ptr;
  int count;

  /* This is a bit tricky */
  c->lastsym = 0;
  c->symtable = arc_mkwtable(c, ARC_HASHBITS);
  c->rsymtable = arc_mkwtable(c, ARC_HASHBITS);
  c->builtins = arc_mkvector(c, BI_last+1);
  c->typedesc = arc_mkhash(c, ARC_HASHBITS);
  VINDEX(c->builtins, BI_syms) = arc_mkvector(c, S_THE_END);
  ARC_BUILTIN(c, S_NIL) = arc_intern_cstr(c, "nil");
  ARC_BUILTIN(c, S_T) = arc_intern_cstr(c, "t");

  /* The above should have produced four items:
     1. The symbol table
     2. The reverse symbol table
     3. The vector for the symbol table data
     4. The vector for the reverse symbol table
     5. The builtins vector
     6. The builtin symbols vector
     7. The string representation of nil
     8. The string representation of t
     9. The hash bucket in symtable for nil
     10. The hash bucket in rsymtable for nil
     11. The hash bucket in symtable for t
     12. The hash bucket in rsymtable for t
     13. The typedesc hash
     14. The typedesc hash vector
   */
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 14);

  /* Annotate created the following objects:

     1. A cons cell for the tagged object
     2. The string representation for the tag
     3. The bucket in the symtable for the tag
     4. The bucket in the rsymtable for the tag
  */
  tagged = arc_annotate(c, arc_intern_cstr(c, "tagged"), INT2FIX(3));
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 18);
  fail_unless(TYPE(tagged) == T_TAGGED);
  fail_unless(arc_type(c, tagged) == arc_intern_cstr(c, "tagged"));
  fail_unless(arc_rep(c, tagged) == INT2FIX(3));

  __arc_markprop(c, tagged);
  __arc_markprop(c, c->symtable);
  __arc_markprop(c, c->rsymtable);
  __arc_markprop(c, c->builtins);
  __arc_markprop(c, c->typedesc);
  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 18);
  fail_unless(TYPE(tagged) == T_TAGGED);
  fail_unless(arc_type(c, tagged) == arc_intern_cstr(c, "tagged"));
  fail_unless(arc_rep(c, tagged) == INT2FIX(3));

  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 0);
}
END_TEST

START_TEST(test_gc_fn_ff)
{
  value fn;
  struct mm_ctx *mmctx = (struct mm_ctx *)c->alloc_ctx;
  Bhdr *ptr;
  int count;

  fn = arc_mkaff(c, arc_iso, arc_mkstringc(c, "iso"));
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 2);
  fail_unless(TYPE(fn) == T_CCODE);


  __arc_markprop(c, fn);
  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 2);
  fail_unless(TYPE(fn) == T_CCODE);

  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 0);
}
END_TEST

START_TEST(test_gc_fn)
{
  value cctx, code;
  struct mm_ctx *mmctx = (struct mm_ctx *)c->alloc_ctx;
  Bhdr *ptr;
  int count, lptr;

  cctx = arc_mkcctx(c);
  lptr = arc_literal(c, cctx, arc_mkflonum(c, 3.1415926535));
  arc_emit1(c, cctx, ildl, INT2FIX(lptr));
  arc_emit(c, cctx, ihlt);
  code = arc_cctx2code(c, cctx);

  /* The code object created should consist of the following:
     1. The code vector itself
     2. The vector for the code
     3. The literal flonum

     Everything else involved in making it should have become garbage.
  */
  __arc_markprop(c, code);
  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 3);

  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 0);
}
END_TEST

#define NUM_CONSES 256

START_TEST(test_gc_lots_of_conses)
{
  value list;
  struct mm_ctx *mmctx = (struct mm_ctx *)c->alloc_ctx;
  Bhdr *ptr;
  int count, i;

  list = CNIL;
  for (i=0; i<NUM_CONSES; i++)
    list = cons(c, INT2FIX(i), list);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == NUM_CONSES);

  __arc_markprop(c, list);
  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == NUM_CONSES);

  c->gc(c);
  count = 0;
  for (ptr = mmctx->alloc_head; ptr; ptr = B2NB(ptr))
    count++;
  fail_unless(count == 0);
  /* The BiBOP page lists should also be emptied by this operation */
  for (i=0; i<=MAX_BIBOP; i++)
    fail_unless(mmctx->bibop_pages[i] == NULL);
}
END_TEST


int main(void)
{
  int number_failed;
  Suite *s = suite_create("Garbage Collection");
  TCase *tc_gc = tcase_create("Garbage Collection");
  SRunner *sr;

  c = &cc;
  arc_init_memmgr(c);
  arc_init_datatypes(c);
  c->markroots = markroots;

  tcase_add_test(tc_gc, test_gc_cons);
#ifdef HAVE_GMP_H
  tcase_add_test(tc_gc, test_gc_bignum);
  tcase_add_test(tc_gc, test_gc_rational);
#endif
  tcase_add_test(tc_gc, test_gc_flonum);
  tcase_add_test(tc_gc, test_gc_complex);

  tcase_add_test(tc_gc, test_gc_char);
  tcase_add_test(tc_gc, test_gc_string);
  tcase_add_test(tc_gc, test_gc_symbol);
  tcase_add_test(tc_gc, test_gc_vector);
  tcase_add_test(tc_gc, test_gc_table);
  tcase_add_test(tc_gc, test_gc_tagged);
  tcase_add_test(tc_gc, test_gc_fn_ff);
  tcase_add_test(tc_gc, test_gc_fn);

  tcase_add_test(tc_gc, test_gc_lots_of_conses);

  suite_add_tcase(s, tc_gc);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
