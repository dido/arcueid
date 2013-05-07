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
#include "../src/io.h"

#define CPUSH_(val) CPUSH(thr, val)

#define XCALL(fname, ...) do {			\
    c->curthread = thr;				\
    SVALR(thr, arc_mkaff(c, fname, CNIL));	\
    TARGC(thr) = NARGS(__VA_ARGS__);		\
    FOR_EACH(CPUSH_, __VA_ARGS__);		\
    __arc_thr_trampoline(c, thr, TR_FNAPP);	\
    ret = TVALR(thr);				\
  } while (0)

arc cc;
arc *c;

START_TEST(test_sio_readb)
{
  value thr, sio;

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "abc"), CNIL);
  SVALR(thr, arc_mkaff(c, arc_readb, CNIL));
  TARGC(thr) = 1;
  CPUSH(thr, sio);
  __arc_thr_trampoline(c, thr, TR_FNAPP);
  fail_unless(TVALR(thr) == INT2FIX(97));

  SVALR(thr, arc_mkaff(c, arc_readb, CNIL));
  TARGC(thr) = 1;
  CPUSH(thr, sio);
  __arc_thr_trampoline(c, thr, TR_FNAPP);
  fail_unless(TVALR(thr) == INT2FIX(98));

  SVALR(thr, arc_mkaff(c, arc_readb, CNIL));
  TARGC(thr) = 1;
  CPUSH(thr, sio);
  __arc_thr_trampoline(c, thr, TR_FNAPP);
  fail_unless(TVALR(thr) == INT2FIX(99));

  SVALR(thr, arc_mkaff(c, arc_readb, CNIL));
  TARGC(thr) = 1;
  CPUSH(thr, sio);
  __arc_thr_trampoline(c, thr, TR_FNAPP);
  fail_unless(NIL_P(TVALR(thr)));
}
END_TEST

START_TEST(test_sio_readc)
{
  value thr, sio;

  thr = arc_mkthread(c);
  sio = arc_instring(c, arc_mkstringc(c, "以呂波"), CNIL);
  SVALR(thr, arc_mkaff(c, arc_readc, CNIL));
  TARGC(thr) = 1;
  CPUSH(thr, sio);
  __arc_thr_trampoline(c, thr, TR_FNAPP);
  fail_unless(TYPE(TVALR(thr)) == T_CHAR);
  fail_unless(arc_char2rune(c, TVALR(thr)) == 0x4ee5);

  SVALR(thr, arc_mkaff(c, arc_readc, CNIL));
  TARGC(thr) = 1;
  CPUSH(thr, sio);
  __arc_thr_trampoline(c, thr, TR_FNAPP);
  fail_unless(TYPE(TVALR(thr)) == T_CHAR);
  fail_unless(arc_char2rune(c, TVALR(thr)) == 0x5442);

  SVALR(thr, arc_mkaff(c, arc_readc, CNIL));
  TARGC(thr) = 1;
  CPUSH(thr, sio);
  __arc_thr_trampoline(c, thr, TR_FNAPP);
  fail_unless(TYPE(TVALR(thr)) == T_CHAR);
  fail_unless(arc_char2rune(c, TVALR(thr)) == 0x6ce2);
  SVALR(thr, arc_mkaff(c, arc_readc, CNIL));
  TARGC(thr) = 1;
  CPUSH(thr, sio);
  __arc_thr_trampoline(c, thr, TR_FNAPP);
  fail_unless(NIL_P(TVALR(thr)));

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

START_TEST(test_fio_readb)
{
  value thr, fio, ret;
  int i;

  thr = arc_mkthread(c);
  XCALL(arc_infile, arc_mkstringc(c, "./rfile.txt"));
  fio = ret;
  fail_if(fio == CNIL);
  fail_unless(TYPE(fio) == T_INPORT);
  for (i=0;;i++) {
    XCALL(arc_readb, fio);
    if (NIL_P(ret))
      break;
    fail_unless(FIX2INT(ret) == bytevals[i]);
  }
  fail_unless(i == sizeof(bytevals)/sizeof(char));
  XCALL(arc_close, fio);
}
END_TEST

START_TEST(test_fio_readc)
{
  value thr, fio, ret;
  int i;

  thr = arc_mkthread(c);
  XCALL(arc_infile, arc_mkstringc(c, "./rfile.txt"));
  fio = ret;
  fail_if(fio == CNIL);
  fail_unless(TYPE(fio) == T_INPORT);
  for (i=0;;i++) {
    XCALL(arc_readc, fio);
    if (NIL_P(ret))
      break;
    fail_unless(arc_char2rune(c, ret) == codes[i]);
  }
  fail_unless(i == sizeof(codes)/sizeof(Rune));
  XCALL(arc_close, fio);
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Arcueid I/O");
  TCase *tc_io = tcase_create("Arcueid I/O");
  SRunner *sr;

  c = &cc;
  arc_init(c);

  tcase_add_test(tc_io, test_sio_readb);
  tcase_add_test(tc_io, test_sio_readc);
  tcase_add_test(tc_io, test_fio_readb);
  tcase_add_test(tc_io, test_fio_readc);

  suite_add_tcase(s, tc_io);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

