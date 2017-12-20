/* 
  Copyright (C) 2017 Rafael R. Sevilla

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
#include <check.h>
#include <stdint.h>
#include <stdio.h>
#include "../src/arcueid.h"
#include "../src/alloc.h"

struct mm_ctx *c;

/* Random number generator based on
   http://burtleburtle.net/bob/rand/smallprng.html */

struct ranctx { uint64_t a; uint64_t b; uint64_t c; uint64_t d; };

#define rot(x,k) (((x)<<(k))|((x)>>(64-(k))))

static uint64_t ranval(struct ranctx *x)
{
  uint64_t e = x->a - rot(x->b, 7);
  x->a = x->b ^ rot(x->c, 13);
  x->b = x->c + rot(x->d, 37);
  x->c = x->d + e;
  x->d = e + x->a;
  return(x->d);
}

static void raninit(struct ranctx *x, uint64_t seed)
{
  uint64_t i;
  x->a = 0xf1ea5eed, x->b = x->c = x->d = seed;
  for (i=0; i<20; ++i) {
    (void)ranval(x);
  }
}

/* We limit the maximum test size. I have tested this privately up to
   MAX_BIBOP */
#define MAX_SIZE 64

START_TEST(test_alloc)
{
  int i, j, k;
  unsigned char *ptrs[BIBOP_PAGE_SIZE*2];
  struct ranctx ctx;
  uint64_t seed;
  struct Bhdr *h;
  unsigned int rval;

  for (i=1; i<=MAX_SIZE; i++) {
    /* Allocate PAGE_SIZE * 2 blocks of size i */
    for (j=0; j<BIBOP_PAGE_SIZE*2; j++)
      ptrs[j] = (unsigned char *)__arc_alloc(c, i);
    /* Fill these blocks with random data */
    for (j=BIBOP_PAGE_SIZE*2-1; j>=0; j--) {
      seed = (uint64_t)((i & 0xffff) | ((j & 0xffff) << 16));
      raninit(&ctx, seed);
      for (k=0; k<i; k++)
	ptrs[j][k] = (unsigned char)(ranval(&ctx) & 0xff);
    }

    /* Verify that the random data is unchanged and that the Bhdrs contain
       the expected values. */
    for (j=0; j<BIBOP_PAGE_SIZE*2; j++) {
      D2B(h, ptrs[j]);
      ck_assert(BALLOCP(h));
      ck_assert(BSIZE(h) == i);
      seed = (uint64_t)((i & 0xffff) | ((j & 0xffff) << 16));
      raninit(&ctx, seed);
      for (k=0; k<i; k++) {
	rval = (unsigned char)(ranval(&ctx) & 0xff);
	ck_assert(ptrs[j][k] == rval);
      }
    }
  }
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("Memory Allocator");
  TCase *tc_alloc = tcase_create("Memory Allocator");
  SRunner *sr;

  c = __arc_new_mm_ctx();

  tcase_add_test(tc_alloc, test_alloc);

  suite_add_tcase(s, tc_alloc);
  sr = srunner_create(s);
  srunner_run_all(sr, CK_VERBOSE);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  __arc_free_mm_ctx(c);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
