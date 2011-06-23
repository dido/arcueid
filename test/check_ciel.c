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
#include <math.h>
#include <stdio.h>
#include "../src/arcueid.h"
#include "../config.h"

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

START_TEST(test_ciel_true)
{
  Rune data[] =
    { 0xc1, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* header */
      0x01 };					      /* GTRUE */
  value cieldata, cielfd, result;

  cieldata = arc_mkstring(cc, data, 9);
  cielfd = arc_instring(cc, cieldata);
  result = arc_ciel_unmarshal(cc, cielfd);
  fail_unless(result == CTRUE);
}
END_TEST

START_TEST(test_ciel_int)
{
  Rune data1[] =
    { 0xc1, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* header */
      0x02, 43, 210, 4, 0, 0, 0, 0, 0, 128 };	      /* GINT */
  Rune data2[] =
    { 0xc1, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* header */
      0x02, 45, 210, 4, 0, 0, 0, 0, 0, 128 };	      /* GINT */
#ifdef HAVE_GMP_H
  Rune data3[] =
    { 0xc1, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* header */
      0x02, 43, 192, 136, 86, 99, 197, 86, 65, 88,
      183, 198, 233, 58, 5, 34, 242, 147 }; /* GINT */
  mpz_t expected;
#endif
  value cieldata, cielfd, result;

  cieldata = arc_mkstring(cc, data1, sizeof(data1) / sizeof(Rune));
  cielfd = arc_instring(cc, cieldata);
  result = arc_ciel_unmarshal(cc, cielfd);
  fail_unless(FIXNUM_P(result));
  fail_unless(FIX2INT(result) == 1234);

  cieldata = arc_mkstring(cc, data2, sizeof(data2) / sizeof(Rune));
  cielfd = arc_instring(cc, cieldata);
  result = arc_ciel_unmarshal(cc, cielfd);
  fail_unless(FIXNUM_P(result));
  fail_unless(FIX2INT(result) == -1234);

#ifdef HAVE_GMP_H
  cieldata = arc_mkstring(cc, data3, sizeof(data3) / sizeof(Rune));
  cielfd = arc_instring(cc, cieldata);
  result = arc_ciel_unmarshal(cc, cielfd);
  fail_unless(TYPE(result) == T_BIGNUM);
  mpz_init(expected);
  mpz_set_str(expected, "13256278887989457651018865901401704640", 10);
  fail_unless(mpz_cmp(expected, REP(result)._bignum) == 0);
  mpz_clear(expected);
#endif
}
END_TEST

START_TEST(test_ciel_flonum)
{
  Rune data1[] =
    { 0xc1, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* header */
      0x03, 0, 0, 0, 0, 0, 0, 240, 63 };	      /* GFLO */
  Rune data2[] =
    { 0xc1, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* header */
      0x03, 23, 45, 68, 84, 251, 33, 9, 64 };	      /* GFLO */
  Rune data3[] =
    { 0xc1, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* header */
      0x03, 150, 193, 78, 139, 11, 134, 11, 57 };     /* GFLO */
  Rune data4[] =
    { 0xc1, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* header */
      0x03, 118, 120, 23, 38, 134, 225, 223, 68 };     /* GFLO */
  value cieldata, cielfd, result;
  double expected;

  cieldata = arc_mkstring(cc, data1, sizeof(data1) / sizeof(Rune));
  cielfd = arc_instring(cc, cieldata);
  result = arc_ciel_unmarshal(cc, cielfd);
  fail_unless(TYPE(result) == T_FLONUM);
  expected = 1.0;
  fail_unless(fabs(REP(result)._flonum - expected)/expected < 1e-6);

  cieldata = arc_mkstring(cc, data2, sizeof(data2) / sizeof(Rune));
  cielfd = arc_instring(cc, cieldata);
  result = arc_ciel_unmarshal(cc, cielfd);
  fail_unless(TYPE(result) == T_FLONUM);
  expected = 3.14159265358979323846;
  fail_unless(fabs(REP(result)._flonum - expected)/expected < 1e-6);

  cieldata = arc_mkstring(cc, data3, sizeof(data3) / sizeof(Rune));
  cielfd = arc_instring(cc, cieldata);
  result = arc_ciel_unmarshal(cc, cielfd);
  fail_unless(TYPE(result) == T_FLONUM);
  expected = 6.6260689633e-34;
  fail_unless(fabs(REP(result)._flonum - expected)/expected < 1e-6);

  cieldata = arc_mkstring(cc, data4, sizeof(data4) / sizeof(Rune));
  cielfd = arc_instring(cc, cieldata);
  result = arc_ciel_unmarshal(cc, cielfd);
  fail_unless(TYPE(result) == T_FLONUM);
  expected = 6.0221417930e23;
  fail_unless(fabs(REP(result)._flonum - expected)/expected < 1e-6);
}
END_TEST

START_TEST(test_ciel_char)
{
  Rune data[] =
    { 0xc1, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* header */
      0x04, 0xdf, 0x86, 0x00, 0x00 };     /* GCHAR */
  value cieldata, cielfd, result;

  cieldata = arc_mkstring(cc, data, sizeof(data) / sizeof(Rune));
  cielfd = arc_instring(cc, cieldata);
  result = arc_ciel_unmarshal(cc, cielfd);
  fail_unless(TYPE(result) == T_CHAR);
  fail_unless(REP(result)._char == 0x86df);
}
END_TEST

START_TEST(test_ciel_string)
{
  Rune codes[] = {
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
  value cielfd, result, fname;
  int i;

  fname = arc_mkstringc(cc, "./string.ciel");
  cielfd = arc_infile(cc, fname);
  result = arc_ciel_unmarshal(cc, cielfd);
  arc_close(cc, cielfd);
  fail_unless(TYPE(result) == T_STRING);
  fail_unless(arc_strlen(cc, result) == sizeof(codes)/sizeof(Rune));
  for (i=0; i<sizeof(codes)/sizeof(Rune); i++)
    fail_unless(arc_strindex(cc, result, i) == codes[i]);
}
END_TEST

START_TEST(test_ciel_sym)
{
  value cielfd, result, fname;

  fname = arc_mkstringc(cc, "./sym.ciel");
  cielfd = arc_infile(cc, fname);
  result = arc_ciel_unmarshal(cc, cielfd);
  arc_close(cc, cielfd);
  fail_unless(SYMBOL_P(result));
  fail_unless(arc_intern(cc, arc_mkstringc(cc, "hello")) == result);
}
END_TEST

START_TEST(test_ciel_rat)
{
#ifdef HAVE_GMP_H
  Rune data1[] =
    { 0xc1, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* header */
      0x02, 43, 2, 0, 0, 0, 0, 0, 0, 128,	      /* GINT */
      0x02, 43, 1, 0, 0, 0, 0, 0, 0, 128,	      /* GINT */
      0x09					      /* CRAT */
    };
  value cieldata, cielfd, result;
  double d, expected;

  cieldata = arc_mkstring(cc, data1, sizeof(data1) / sizeof(Rune));
  cielfd = arc_instring(cc, cieldata);
  result = arc_ciel_unmarshal(cc, cielfd);
  fail_unless(TYPE(result) == T_RATIONAL);
  d = mpq_get_d(REP(result)._rational);
  expected = 0.5;
  fail_unless(fabs((d - expected)/expected) < 1e-6);
#endif
}
END_TEST

START_TEST(test_ciel_complex)
{
  Rune data1[] =
    { 0xc1, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* header */
      0x03, 23, 45, 68, 84, 251, 33, 9, 64,	      /* GFLO */
      0x03, 0, 0, 0, 0, 0, 0, 240, 63,		      /* GFLO */
      0x0a };					      /* CCOMPLEX */
  value cieldata, cielfd, result;
  double d, expected;

  cieldata = arc_mkstring(cc, data1, sizeof(data1) / sizeof(Rune));
  cielfd = arc_instring(cc, cieldata);
  result = arc_ciel_unmarshal(cc, cielfd);

  fail_unless(TYPE(result) == T_COMPLEX);

  d = REP(result)._complex.re;
  expected = 1.0;
  fail_unless(fabs((d - expected)/expected) < 1e-6);

  d = REP(result)._complex.im;
  expected = 3.14159265358979323846;
  fail_unless(fabs((d - expected)/expected) < 1e-6);

}
END_TEST

START_TEST(test_ciel_cons)
{
  Rune data1[] =
    { 0xc1, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* header */
      0x03, 23, 45, 68, 84, 251, 33, 9, 64,	      /* GFLO */
      0x03, 0, 0, 0, 0, 0, 0, 240, 63,		      /* GFLO */
      0x0c };					      /* CCONS */
  value cieldata, cielfd, result, v;
  double d, expected;

  cieldata = arc_mkstring(cc, data1, sizeof(data1) / sizeof(Rune));
  cielfd = arc_instring(cc, cieldata);
  result = arc_ciel_unmarshal(cc, cielfd);

  fail_unless(CONS_P(result));

  v = car(result);
  fail_unless(TYPE(v) == T_FLONUM);
  d = REP(v)._flonum;
  expected = 1.0;
  fail_unless(fabs((d - expected)/expected) < 1e-6);

  v = cdr(result);
  fail_unless(TYPE(v) == T_FLONUM);
  d = REP(v)._flonum;
  expected = 3.14159265358979323846;
  fail_unless(fabs((d - expected)/expected) < 1e-6);
}
END_TEST

START_TEST(test_ciel_code)
{
  value cielfd, result, fname, thr;

  fname = arc_mkstringc(cc, "./code.ciel");
  cielfd = arc_infile(cc, fname);
  result = arc_ciel_unmarshal(cc, cielfd);
  arc_close(cc, cielfd);
  fail_unless(TYPE(result) == T_CODE);
  thr = arc_mkthread(&c, result, 2048, 0);
  arc_vmengine(&c, thr, 1000);
  fail_unless(TVALR(thr) == INT2FIX(2));
}
END_TEST

START_TEST(test_ciel_xdup)
{
  Rune data1[] =
    { 0xc1, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* header */
      0x03, 23, 45, 68, 84, 251, 33, 9, 64,	      /* GFLO */
      0x0e,					      /* XDUP */
      0x0c };					      /* CCONS */
  value cieldata, cielfd, result, v;
  double d, expected;

  cieldata = arc_mkstring(cc, data1, sizeof(data1) / sizeof(Rune));
  cielfd = arc_instring(cc, cieldata);
  result = arc_ciel_unmarshal(cc, cielfd);

  fail_unless(CONS_P(result));

  v = car(result);
  fail_unless(TYPE(v) == T_FLONUM);
  d = REP(v)._flonum;
  expected = 3.14159265358979323846;
  fail_unless(fabs((d - expected)/expected) < 1e-6);

  v = cdr(result);
  fail_unless(TYPE(v) == T_FLONUM);
  d = REP(v)._flonum;
  expected = 3.14159265358979323846;
  fail_unless(fabs((d - expected)/expected) < 1e-6);

}
END_TEST

START_TEST(test_ciel_memo)
{
  Rune data1[] =
    { 0xc1, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* header */
      0x03, 23, 45, 68, 84, 251, 33, 9, 64,	      /* GFLO */
      0x0f, 0, 0, 0, 0, 0, 0, 0, 128,		      /* XMST 0 */
      0x03, 0, 0, 0, 0, 0, 0, 240, 63,		      /* GFLO */
      0x0f, 1, 0, 0, 0, 0, 0, 0, 128,		      /* XMST 1 */
      0x10, 0, 0, 0, 0, 0, 0, 0, 128 };		      /* XMLD 0 */
  value cieldata, cielfd, v;
  double d, expected;

  cieldata = arc_mkstring(cc, data1, sizeof(data1) / sizeof(Rune));
  cielfd = arc_instring(cc, cieldata);
  v = arc_ciel_unmarshal(cc, cielfd);

  fail_unless(TYPE(v) == T_FLONUM);
  d = REP(v)._flonum;
  expected = 3.14159265358979323846;
  fail_unless(fabs((d - expected)/expected) < 1e-6);
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("CIEL");
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
  tcase_add_test(tc_ciel, test_ciel_true);
  tcase_add_test(tc_ciel, test_ciel_int);
  tcase_add_test(tc_ciel, test_ciel_flonum);
  tcase_add_test(tc_ciel, test_ciel_char);
  tcase_add_test(tc_ciel, test_ciel_string);
  tcase_add_test(tc_ciel, test_ciel_sym);
  tcase_add_test(tc_ciel, test_ciel_rat);
  tcase_add_test(tc_ciel, test_ciel_complex);
  tcase_add_test(tc_ciel, test_ciel_cons);
  tcase_add_test(tc_ciel, test_ciel_code);
  tcase_add_test(tc_ciel, test_ciel_xdup);
  tcase_add_test(tc_ciel, test_ciel_memo);
  suite_add_tcase(s, tc_ciel);

  sr = srunner_create(s);

  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
