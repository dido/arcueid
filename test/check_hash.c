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

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Hashing");
  TCase *tc_hash = tcase_create("Hash functions");
  SRunner *sr;

  tcase_add_test(tc_hash, test_hash);

  suite_add_tcase(s, tc_hash);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

