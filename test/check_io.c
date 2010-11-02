/* 
  Copyright (C) 2010 Rafael R. Sevilla

  This file is part of Arcueid

  Arcueid is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; either version 3 of the
  License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include "../src/utf.h"

arc c, *cc;

START_TEST(test_str_read)
{
  value str, strio;
  int i;

  str = arc_mkstringc(&c, "0123456789");
  strio = arc_instring(&c, str);
  for (i=0; i<10; i++)
    fail_unless(arc_readb(&c, strio) == INT2FIX(0x30 + i));
  fail_unless(FIX2INT(arc_readb(&c, strio)) < 0);
}
END_TEST

static Rune codes[] = {
  /* Hello, world */
  0x0048, 0x0065, 0x006c, 0x006c, 0x006f, 0x002c, 0x0020,
  0x0077, 0x006f, 0x0072, 0x006c, 0x0064, 0x000a,
  /* Καλημέρα κόσμε */
  0x039a, 0x03b1, 0x03bb, 0x03b7, 0x03bc, 0x03ad, 0x3c1, 0x03b1, 0x0020,
  0x03ba, 0x03cc, 0x03c3, 0x03bc, 0x03b5, 0x000a,
  /* こんにちは 世界 */
  0x3053, 0x3093, 0x306b, 0x3061, 0x306f, 0x20,
  0x4e16, 0x754c, 0x0a
};

static unsigned char bytevals[] = {
  0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x2c, 0x20, 0x77, 0x6f, 0x72, 0x6c, 0x64,
  0x0a, 0xce, 0x9a, 0xce, 0xb1, 0xce, 0xbb, 0xce, 0xb7, 0xce, 0xbc, 0xce,
  0xad, 0xcf, 0x81, 0xce, 0xb1, 0x20, 0xce, 0xba, 0xcf, 0x8c, 0xcf, 0x83,
  0xce, 0xbc, 0xce, 0xb5, 0x0a, 0xe3, 0x81, 0x93, 0xe3, 0x82, 0x93, 0xe3,
  0x81, 0xab, 0xe3, 0x81, 0xa1, 0xe3, 0x81, 0xaf, 0x20, 0xe4, 0xb8, 0x96,
  0xe7, 0x95, 0x8c, 0x0a
};

START_TEST(test_file_read)
{
  value fname, fp;
  Rune rune;
  int i, byte;

  fname = arc_mkstringc(&c, "./rfile.txt");
  fp = arc_infile(&c, fname);
  fail_if(fp == CNIL);
  i=0;
  while ((rune = arc_readc_rune(&c, fp)) >= 0) {
    fail_unless(rune == codes[i]);
    i++;
  }

  arc_seek(&c, fp, INT2FIX(0), INT2FIX(SEEK_SET));
  fail_unless(arc_tell(&c, fp) == INT2FIX(0));
  i=0;
  while (FIX2INT(byte = arc_readb(&c, fp)) >= 0) {
    fail_unless(FIX2INT(byte) == bytevals[i]);
    i++;
  }
  fail_unless(arc_close(&c, fp) == 0);
}
END_TEST

START_TEST(test_file_ungetc)
{
  value fname, fp;
  Rune rune;

  fname = arc_mkstringc(&c, "./rfile.txt");
  fp = arc_infile(&c, fname);
  fail_if(fp == CNIL);
  rune = arc_readc_rune(&c, fp);
  fail_unless(rune == codes[0]);
  arc_ungetc_rune(&c, rune, fp);
  rune = arc_readc_rune(&c, fp);
  fail_unless(rune == codes[0]);
  rune = arc_readc_rune(&c, fp);
  fail_unless(rune == codes[1]);
  rune = arc_peekc_rune(&c, fp);
  fail_unless(rune == codes[2]);
  rune = arc_readc_rune(&c, fp);
  fail_unless(rune == codes[2]);
  fail_unless(arc_close(&c, fp) == 0);
}
END_TEST

extern unsigned long long gcepochs;

int main(void)
{

  int number_failed;
  Suite *s = suite_create("I/O");
  TCase *tc_strio = tcase_create("String I/O");
  TCase *tc_fileio = tcase_create("File I/O");
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

  tcase_add_test(tc_strio, test_str_read);
  tcase_add_test(tc_fileio, test_file_read);
  tcase_add_test(tc_fileio, test_file_ungetc);
  suite_add_tcase(s, tc_strio);
  suite_add_tcase(s, tc_fileio);

  sr = srunner_create(s);

  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
