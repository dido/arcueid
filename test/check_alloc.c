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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
*/
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include "../src/alloc.h"

struct Shdr *_carc_new_segment(size_t size);

START_TEST(test_new_segment)
{
  struct Shdr *s;
  struct Bhdr *b, *b2;
  char *data;

  s = _carc_new_segment(1024);
  fail_unless(s->size == 1024 + sizeof(struct Shdr) + sizeof(struct Bhdr));
  fail_unless(s->next == NULL);
  b = s->fblk;
  fail_unless(b->magic  == MAGIC_F);
  fail_unless(b->u.s.left == NULL);
  fail_unless(b->u.s.right == NULL);
  fail_unless(b->size == 1024);
  data = B2D(b);
  b->magic = MAGIC_A;
  D2B(b2, data);
  fail_unless(b2 == b);
  memset(data, 0, 1024);
  b = B2NB(b);
  fail_unless(b->magic  == MAGIC_E);
  fail_unless(b->size == 0);
}
END_TEST

Suite *alloc_suite(void)
{
  Suite *s = suite_create("Alloc");
  TCase *tc_alloc = tcase_create("Alloc");

  tcase_add_test(tc_alloc, test_new_segment);
  suite_add_tcase(s, tc_alloc);
  return(s);
}

int main(void)
{
  int number_failed;
  Suite *s = alloc_suite();
  SRunner *sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
