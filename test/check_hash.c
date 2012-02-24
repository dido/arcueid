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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include "../src/arcueid.h"
#include "../config.h"

arc c;

START_TEST(test_hash)
{
  arc_hs hs;
  int i;
  unsigned long val;

  /* These test values are based on Bob Jenkins' own reference code
     from his website.

     The 64-bit reference values are correct and match lookup8.c.  The
     32-bit hash values do not match lookup3.c, because lookup3.c
     actually includes the length of the string as part of the
     initial state, rather than adding it in near the end the way
     lookup8.c does.  We thus have a variant of lookup3.c which does
     the same thing and adds the length near the end.  I don't think
     it should be much worse performing than the original Jenkins
     32-bit hash, and our requirement is that it should be possible to
     incrementally compute the value of the hash: including the length
     of the string as part of the initial state of the hash makes this
     all but impossible.  I'll drop Mr. Jenkins a line again and ask
     what he thinks.
  */

  arc_hash_init(&hs, 0);
  for (i=0; i<6; i++)
    arc_hash_update(&hs, i);
  val = arc_hash_final(&hs, 6);

#if SIZEOF_LONG >= 8
  fail_unless(val == 0x88d7a582ec392ac7LL);
#else
  fail_unless(val == 0x5d482818L);
#endif

  arc_hash_init(&hs, 0);
  for (i=0; i<7; i++)
    arc_hash_update(&hs, i);
  val = arc_hash_final(&hs, 7);

#if SIZEOF_LONG >= 8
  fail_unless(val == 0x2e58f2b17158ae78LL);
#else
  fail_unless(val == 0xe7433410L);
#endif

  arc_hash_init(&hs, 0);
  for (i=0; i<8; i++)
    arc_hash_update(&hs, i);
  val = arc_hash_final(&hs, 8);
#if SIZEOF_LONG >= 8
  fail_unless(val == 0x6d0b4a891c9c3e8aLL);
#else
  fail_unless(val == 0x2a6f52dbL);
#endif

}
END_TEST

START_TEST(test_hash_atomic_value)
{
  unsigned long hash;
  value v;
  char *ucstr;

#if SIZEOF_LONG >= 8
  hash = arc_hash(&c, CNIL);
  fail_unless(hash == 0xc2503378028494d6);
  hash = arc_hash(&c, CTRUE);
  fail_unless(hash == 0xa682854132f7af8e);
  hash = arc_hash(&c, INT2FIX(1234));
  fail_unless(hash == 0x8679bae556349cee);

  v = arc_mkflonum(&c, 3.14159);
  hash = arc_hash(&c, v);
  fail_unless(hash == 0x64cdc28d92b9307f);

  v = arc_mkcomplex(&c, 3.14159, 2.71828);
  hash = arc_hash(&c, v);
  fail_unless(hash == 0x8474b3c3090517c6);

#ifdef HAVE_GMP_H
  v = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v)._bignum, "100000000000000000000000000000", 10);
  hash = arc_hash(&c, v);
  fail_unless(hash == 0x9ead756c83497094);

  v = arc_mkrationall(&c, 1, 2);
  hash = arc_hash(&c, v);
  fail_unless(hash == 0x93e07bdce517fbf6);
#endif

  ucstr = "unicode string \343\201\235\343\201\256\347\233\256\343\200\201\350\252\260\343\201\256\347\233\256\343\200\202\343\200\202\343\200\202";
  v = arc_mkstringc(&c, ucstr);
  hash = arc_hash(&c, v);
  fail_unless(hash == 0x57f9c9fa351f092e);

  hash = arc_hash_cstr(&c, ucstr);
  fail_unless(hash == 0x57f9c9fa351f092e);

#else

  hash = arc_hash(&c, CNIL);
  fail_unless(hash == 0);
  hash = arc_hash(&c, CTRUE);
  fail_unless(hash == 0);
  hash = arc_hash(&c, INT2FIX(1234));
  fail_unless(hash == 0);

  v = arc_mkflonum(&c, 3.14159);
  hash = arc_hash(&c, v);
  fail_unless(hash == 0);

  v = arc_mkcomplex(&c, 3.14159, 2.71828);
  hash = arc_hash(&c, v);
  fail_unless(hash == 0);

#ifdef HAVE_GMP_H
  v = arc_mkbignuml(&c, 0);
  mpz_set_str(REP(v)._bignum, "100000000000000000000000000000", 10);
  hash = arc_hash(&c, v);
  fail_unless(hash == 0);

  v = arc_mkrationall(&c, 1, 2);
  hash = arc_hash(&c, v);
  fail_unless(hash == 0);
#endif

  ucstr = "unicode string \343\201\235\343\201\256\347\233\256\343\200\201\350\252\260\343\201\256\347\233\256\343\200\202\343\200\202\343\200\202";
  v = arc_mkstringc(&c, ucstr);
  hash = arc_hash(&c, v);
  fail_unless(hash == 0);

  hash = arc_hash_cstr(&c, ucstr);
  fail_unless(hash == 0);

#endif
}
END_TEST

START_TEST(test_hash_nonatomic_value)
{
  value list = CNIL;
  int i;
  unsigned long hash;

  for (i=0; i<10; i++)
    list = cons(&c, INT2FIX(i), list);
  hash = arc_hash(&c, list);
  fail_unless(hash == 0x12b1009768733515);
  /*  printf("%lx\n", hash); */
}
END_TEST

static char *names[] = {
  "Saber",
  "Archer",
  "Berserker",
  "Rider",
  "Lancer",
  "Caster",
  "Assassin",
  "Gilgamesh",
  "Emiya Shir\305\215",
  "T\305\215saka Rin",
  "Ilyasviel von Einzbern",
  "Sakura Mat\305\215",
  "Shinji Mat\305\215",
  "Kirei Kotomine",
  "Emiya Kiritsugu",
  "Fujimura Taiga",
  "Kuzuki S\305\215ichir\305\215"
};

static value vnames[17];

START_TEST(test_hash_table)
{
  value table, notfoundstr, str;
  int i;

  /* Start with 3 bits.  This should expand to at least five bits after
     we add the 17 names above. */
  table = arc_mkhash(&c, 3);

  for (i=0; i<17; i++) {
    vnames[i] = arc_mkstringc(&c, names[i]);
    arc_hash_insert(&c, table, vnames[i], INT2FIX(i));
  }

  /* verify */
  for (i=0; i<17; i++)
    fail_unless(arc_hash_lookup(&c, table, vnames[i]) == INT2FIX(i));

  /* verify with cstrings */
  for (i=0; i<17; i++) {
    value v;

    v = arc_hash_lookup_cstr(&c, table, names[i]);
    fail_unless(v == INT2FIX(i));
  }

  /* Test not present key */
  notfoundstr = arc_mkstringc(&c, "Caren Ortensia");
  fail_unless(arc_hash_lookup(&c, table, notfoundstr) == CUNBOUND);

  /* Test rebinding key */
  str = arc_mkstringc(&c, "Saber");
  fail_unless(arc_hash_lookup(&c, table, str) == INT2FIX(0));
  arc_hash_insert(&c, table, str, INT2FIX(100));
  fail_unless(arc_hash_lookup(&c, table, str) == INT2FIX(100));

  /* Test length */
  i=arc_hash_length(&c, table);
  fail_unless(i==17);

  /* Test deletion */
  for (i=16; i>=0; i--) {
    arc_hash_delete(&c, table, vnames[i]);
    fail_unless(arc_hash_lookup(&c, table, vnames[i]) == CUNBOUND);
  }
  i=arc_hash_length(&c, table);
  fail_unless(i==0);
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Hashing");
  TCase *tc_hash = tcase_create("Hash functions");
  SRunner *sr;

  arc_set_memmgr(&c);

  tcase_add_test(tc_hash, test_hash);
  tcase_add_test(tc_hash, test_hash_atomic_value);
  tcase_add_test(tc_hash, test_hash_nonatomic_value);
  tcase_add_test(tc_hash, test_hash_table);

  suite_add_tcase(s, tc_hash);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

