/* Copyright (C) 2017, 2018 Rafael R. Sevilla

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
#include <string.h>
#include "arcueid.h"
#include "utf.h"
#include "alloc.h"

typedef struct {
  unsigned int length;
  Rune *strdata;
} arcstr;

static void strfree(arc *c, value s)
{
  arcstr *sd = (arcstr *)s;
  __arc_free((struct mm_ctx *)c->mm_ctx, sd->strdata);
}

static uint64_t strhash(arc *c, value s, uint64_t seed)
{
  struct hash_ctx ctx;
  uint64_t t;
  arcstr *sd = (arcstr *)s;
  /* first 64 bits of sha512 of 'string' */
  static const uint64_t magic = 0x2757cb3cafc39af4ULL;
  unsigned int lendiv, endpos, i;

  __arc_hash_init(&ctx, seed);
  __arc_hash_update(&ctx, &magic, 1);
  t = (uint64_t)sd->length;
  __arc_hash_update(&ctx, &t, 1);
  lendiv = (sizeof(Rune)*sd->length)/sizeof(uint64_t);
  __arc_hash_update(&ctx, (uint64_t *)sd->strdata, lendiv);
  endpos = (lendiv * sizeof(uint64_t)) / sizeof(Rune);
  for (i=endpos; i<sd->length; i++) {
    t = (uint64_t)(*(sd->strdata + i));
    __arc_hash_update(&ctx, &t, 1);
  }
  return(__arc_hash_final(&ctx));
}

static int striso(arc *c, value s1, value s2)
{
  arcstr *str1 = (arcstr *)s1, *str2 = (arcstr *)s2;

  /* Must have same length */
  if (str1->length != str2->length)
    return(0);
  /* Compare */
  return(memcmp(str1->strdata, str2->strdata, str1->length*sizeof(Rune)) == 0);
}

arctype __arc_string_t = { strfree, NULL, strhash, striso, striso, NULL };

static value str_alloc(arc *c, int len)
{
  value val = arc_new(c, &__arc_string_t, sizeof(arcstr));
  arcstr *str = (arcstr *)val;

  str->length = len;
  str->strdata = (Rune *)__arc_alloc((struct mm_ctx *)c->mm_ctx,
				     sizeof(Rune)*len);
  return(val);
}

value arc_str_new(arc *c, int len, Rune ch)
{
  value val = str_alloc(c, len);
  arcstr *str = (arcstr *)val;
  unsigned int i;

  for (i=0; i<str->length; i++)
    *(str->strdata + i) = ch;
  return(val);
}

value arc_string_new_str(arc *c, int len, Rune *s)
{
  value val = str_alloc(c, len);
  arcstr *str = (arcstr *)val;

  memcpy(str->strdata, s, sizeof(Rune)*len);
  return(val);
}

value arc_string_new_cstr(arc *c, char *cstr)
{
  int len;
  value val;
  arcstr *str;
  char *p;
  Rune *r;

  len = utflen(cstr);
  val = str_alloc(c, len);
  str = (arcstr *)val;
  r = str->strdata;
  for (p = cstr; *p;)
    p += chartorune(r++, p);
  return(val);
}
