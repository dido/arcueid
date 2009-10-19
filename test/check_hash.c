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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include "../src/carc.h"
#include "../config.h"

carc c;

START_TEST(test_hash)
{
  carc_hs hs;
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

  carc_hash_init(&hs, 0);
  for (i=0; i<6; i++)
    carc_hash_update(&hs, i);
  val = carc_hash_final(&hs, 6);

#if SIZEOF_LONG >= 8
  fail_unless(val == 0x88d7a582ec392ac7LL);
#else
  fail_unless(val == 0x5d482818L);
#endif

  carc_hash_init(&hs, 0);
  for (i=0; i<7; i++)
    carc_hash_update(&hs, i);
  val = carc_hash_final(&hs, 7);

#if SIZEOF_LONG >= 8
  fail_unless(val == 0x2e58f2b17158ae78LL);
#else
  fail_unless(val == 0xe7433410L);
#endif

  carc_hash_init(&hs, 0);
  for (i=0; i<8; i++)
    carc_hash_update(&hs, i);
  val = carc_hash_final(&hs, 8);
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

  hash = carc_hash(&c, CNIL);
  fail_unless(hash == 0xc2503378028494d6);
  hash = carc_hash(&c, CTRUE);
  fail_unless(hash == 0xa682854132f7af8e);
  hash = carc_hash(&c, INT2FIX(1234));
  fail_unless(hash == 0xf2e87bdaf56a2cfb);

  v = carc_mkflonum(&c, 3.14159);
  hash = carc_hash(&c, v);
  fail_unless(hash == 0xa0c45deaa94dc4ae);

  v = carc_mkcomplex(&c, 3.14159, 2.71828);
  hash = carc_hash(&c, v);
  fail_unless(hash == 0x4e587f004b90b15a);

#ifdef HAVE_GMP_H
  v = carc_mkbignuml(&c, 0);
  mpz_set_str(REP(v)._bignum, "100000000000000000000000000000", 10);
  hash = carc_hash(&c, v);
  fail_unless(hash == 0x1fdc09d537e098df);

  v = carc_mkrationall(&c, 1, 2);
  hash = carc_hash(&c, v);
  fail_unless(hash == 0x78b0714df1847441);
#endif

  v = carc_mkstringc(&c, "unicode string \343\201\235\343\201\256\347\233\256\343\200\201\350\252\260\343\201\256\347\233\256\343\200\202\343\200\202\343\200\202");
  hash = carc_hash(&c, v);
  fail_unless(hash == 0x51f424be55282439);
}
END_TEST

START_TEST(test_hash_nonatomic_value)
{
  value list = CNIL;
  int i;
  unsigned long hash;

  for (i=0; i<10; i++)
    list = cons(&c, INT2FIX(i), list);
  hash = carc_hash(&c, list);
  fail_unless(hash == 0xd43dd6e10d38e3fa);
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
  value table, notfoundstr;
  int i;
  void *ctx = NULL;

  /* Start with 3 bits.  This should expand to at least five bits after
     we add the 17 names above. */
  table = carc_mkhash(&c, 3);

  for (i=0; i<17; i++) {
    vnames[i] = carc_mkstringc(&c, names[i]);
    carc_hash_insert(&c, table, vnames[i], INT2FIX(i));
  }

  /* verify */
  for (i=0; i<17; i++)
    fail_unless(carc_hash_lookup(&c, table, vnames[i]) == INT2FIX(i));

  /* Test not present key */
  notfoundstr = carc_mkstringc(&c, "Caren Ortensia");
  fail_unless(carc_hash_lookup(&c, table, notfoundstr) == CUNBOUND);

  /* Test iteration */
  i=0;
  while (carc_hash_iter(&c, table, &ctx) != CUNBOUND)
    i++;
  fail_unless(i==17);

  /* Test deletion */
  for (i=16; i>=0; i--) {
    carc_hash_delete(&c, table, vnames[i]);
    fail_unless(carc_hash_lookup(&c, table, vnames[i]) == CUNBOUND);
  }
  i=0;
  while (carc_hash_iter(&c, table, &ctx) != CUNBOUND)
    i++;
  fail_unless(i==0);
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Hashing");
  TCase *tc_hash = tcase_create("Hash functions");
  SRunner *sr;

  carc_set_memmgr(&c);

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

