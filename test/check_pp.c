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
#include <stdio.h>
#include "../src/arcueid.h"
#include "../config.h"

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
#ifndef alloca
# define alloca __builtin_alloca
#endif
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# include <stddef.h>
void *alloca (size_t);
#endif

arc cc;
arc *c;

/* Do nothing: root marking is done by individual tests */
static void markroots(arc *c)
{
}

START_TEST(test_cons)
{
  value list1, list2, list3, list4, list5;
  value ppstr=CNIL;
  char *str;

  list1 = cons(c, INT2FIX(1), cons(c, INT2FIX(2), cons(c, INT2FIX(3), CNIL)));

  arc_prettyprint(c, list1, &ppstr, CNIL);
  str = (char *)alloca(FIX2INT(arc_strutflen(c, ppstr))*sizeof(char));
  arc_str2cstr(c, ppstr, str);
  fail_unless(strcmp(str, "(1 2 3)") == 0);

  /* Various types of circular lists */
  list2 = cons(c, INT2FIX(1), cons(c, INT2FIX(2), cons(c, INT2FIX(3), CNIL)));
  car(list2) = list2;
  ppstr = CNIL;
  arc_prettyprint(c, list2, &ppstr, CNIL);
  str = (char *)alloca(FIX2INT(arc_strutflen(c, ppstr))*sizeof(char));
  arc_str2cstr(c, ppstr, str);
  fail_unless(strcmp(str, "((...) 2 3)") == 0);

  list3 = cons(c, INT2FIX(1), cons(c, INT2FIX(2), cons(c, INT2FIX(3), CNIL)));
  car(cdr(list3)) = list3;
  ppstr = CNIL;
  arc_prettyprint(c, list3, &ppstr, CNIL);
  str = (char *)alloca(FIX2INT(arc_strutflen(c, ppstr))*sizeof(char));
  arc_str2cstr(c, ppstr, str);
  fail_unless(strcmp(str, "(1 (...) 3)") == 0);

  list4 = cons(c, INT2FIX(1), cons(c, INT2FIX(2), cons(c, INT2FIX(3), CNIL)));
  car(cdr(cdr(list4))) = list4;
  ppstr = CNIL;
  arc_prettyprint(c, list4, &ppstr, CNIL);
  str = (char *)alloca(FIX2INT(arc_strutflen(c, ppstr))*sizeof(char));
  arc_str2cstr(c, ppstr, str);
  fail_unless(strcmp(str, "(1 2 (...))") == 0);

  list5 = cons(c, INT2FIX(1), cons(c, INT2FIX(2), cons(c, INT2FIX(3), CNIL)));
  cdr(cdr(cdr(list5))) = list5;
  ppstr = CNIL;
  arc_prettyprint(c, list5, &ppstr, CNIL);
  str = (char *)alloca(FIX2INT(arc_strutflen(c, ppstr))*sizeof(char));
  arc_str2cstr(c, ppstr, str);
  fail_unless(strcmp(str, "(1 2 3 . (...))") == 0);
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Pretty Printer");
  TCase *tc_pp = tcase_create("Pretty Printer");
  SRunner *sr;

  c = &cc;
  arc_set_memmgr(c);
  arc_init_datatypes(c);
  c->markroots = markroots;

  tcase_add_test(tc_pp, test_cons);

  suite_add_tcase(s, tc_pp);
  sr = srunner_create(s);
  /*  srunner_set_fork_status(sr, CK_NOFORK); */
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
