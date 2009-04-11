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

START_TEST(test_ftree_demote)
{
  struct Bhdr blocks[13];
  struct Bhdr *root = &blocks[4];

  /* This is the sample Cartesian tree in Johnson 1991. */
  blocks[0].size = 5;
  blocks[0].u.s.left = NULL;
  blocks[0].u.s.right = NULL;

  blocks[1].size = 10;
  blocks[1].u.s.left = &blocks[0];
  blocks[1].u.s.right = &blocks[2];

  blocks[2].size = 7;
  blocks[2].u.s.left = NULL;
  blocks[2].u.s.right = NULL;

  blocks[3].size = 20;
  blocks[3].u.s.left = &blocks[1];
  blocks[3].u.s.right = NULL;

  blocks[4].size = 60;
  blocks[4].u.s.left = &blocks[3];
  blocks[4].u.s.right = &blocks[9];

  blocks[5].size = 2;
  blocks[5].u.s.left = NULL;
  blocks[5].u.s.right = NULL;

  blocks[6].size = 9;
  blocks[6].u.s.left = &blocks[5];
  blocks[6].u.s.right = &blocks[7];

  blocks[7].size = 4;
  blocks[7].u.s.left = NULL;
  blocks[7].u.s.right = NULL;

  blocks[8].size = 25;
  blocks[8].u.s.left = &blocks[6];
  blocks[8].u.s.right = NULL;

  blocks[9].size = 54;
  blocks[9].u.s.left = &blocks[8];
  blocks[9].u.s.right = &blocks[11];

  blocks[10].size = 18;
  blocks[10].u.s.left = NULL;
  blocks[10].u.s.right = NULL;

  blocks[11].size = 40;
  blocks[11].u.s.left = &blocks[10];
  blocks[11].u.s.right = &blocks[12];

  blocks[12].size = 7;
  blocks[12].u.s.left = NULL;
  blocks[12].u.s.right = NULL;

  blocks[9].size = 20;		/* This node gets rebalanced. */
  _carc_ftree_demote(&root->u.s.right, &blocks[9]);
  fail_unless(blocks[9].u.s.left == NULL);
  fail_unless(blocks[9].u.s.right == &blocks[10]);
  fail_unless(blocks[8].u.s.left == &blocks[6]);
  fail_unless(blocks[8].u.s.right == &blocks[9]);
  fail_unless(blocks[11].u.s.left == &blocks[8]);
  fail_unless(blocks[11].u.s.right == &blocks[12]);
  fail_unless(blocks[4].u.s.left == &blocks[3]);
  fail_unless(blocks[4].u.s.right == &blocks[11]);
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("MemManager");
  TCase *tc_alloc = tcase_create("Alloc");
  TCase *tc_treeops = tcase_create("TreeOps");
  TCase *tc_free = tcase_create("Free");
  SRunner *sr;

  tcase_add_test(tc_alloc, test_new_segment);
  tcase_add_test(tc_treeops, test_ftree_demote);

  suite_add_tcase(s, tc_alloc);
  suite_add_tcase(s, tc_treeops);
  suite_add_tcase(s, tc_free);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
