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

arc c, *cc;
extern unsigned long long gcepochs;

START_TEST(test_ciel_nil)
{
  Rune data[] =
    { 0xc1, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* header */
      0x00 };					      /* GNIL */
  value cieldata, cielfd, result;

  cieldata = arc_mkstring(cc, data, 9);
  cielfd = arc_instring(cc, cieldata);
  result = arc_ciel_unmarshal(cc, cielfd);
  fail_unless(result == CNIL);
}
END_TEST

int main(void)
{

  int number_failed;
  Suite *s = suite_create("I/O");
  TCase *tc_ciel = tcase_create("CIEL");
  SRunner *sr;
  unsigned long long oldepoch;

  arc_set_memmgr(&c);
  arc_init_reader(&c);

  oldepoch = gcepochs;
  while (gcepochs == oldepoch) {
    c.rungc(&c);
  }

  oldepoch = gcepochs;
  while (gcepochs == oldepoch) {
    c.rungc(&c);
  }

  oldepoch = gcepochs;
  while (gcepochs == oldepoch) {
    c.rungc(&c);
  }

  cc = &c;

  tcase_add_test(tc_ciel, test_ciel_nil);
  suite_add_tcase(s, tc_ciel);

  sr = srunner_create(s);

  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
