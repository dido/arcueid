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

/* first 64 bits of sha512 of 'string' */
static const uint64_t magic = 0x2757cb3cafc39af4ULL;

uint64_t __arc_utfstrhash(arc *c, const char *s, uint64_t seed)
{
  struct hash_ctx ctx;
  uint64_t t;
  Rune r[2];
  const char *p;
  int ofs;

  __arc_hash_init(&ctx, seed);
  __arc_hash_update(&ctx, &magic, 1);
  t = (uint64_t)utflen(s);
  __arc_hash_update(&ctx, &t, 1);
  ofs = 0;
  for (p = s; *p;) {
    p += chartorune(r + ofs, p);
    ofs = (ofs + 1) % 2;
    if (ofs == 0)
      __arc_hash_update(&ctx, (uint64_t *)r, 1);
  }
  if (ofs > 0) {
    t = (uint64_t)r[0];
    __arc_hash_update(&ctx, &t, 1);
  }
  return(__arc_hash_final(&ctx));
}

static uint64_t strhash(arc *c, value s, uint64_t seed)
{
  struct hash_ctx ctx;
  uint64_t t;
  arcstr *sd = (arcstr *)s;
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

static enum arc_trstate apply(arc *c, value t)
{
  /* XXX fill this in */
  return(TR_RC);
}

arctype __arc_string_t = { strfree, NULL, strhash, striso, striso, NULL, apply };

static value str_alloc(arc *c, int len)
{
  value val = arc_new(c, &__arc_string_t, sizeof(arcstr));
  arcstr *str = (arcstr *)val;

  str->length = len;
  str->strdata = (Rune *)__arc_alloc((struct mm_ctx *)c->mm_ctx,
				     sizeof(Rune)*len);
  return(val);
}

value arc_string_new(arc *c, int len, Rune ch)
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

value arc_string_new_cstr(arc *c, const char *cstr)
{
  int len;
  value val;
  arcstr *str;
  const char *p;
  Rune *r;

  len = utflen(cstr);
  val = str_alloc(c, len);
  str = (arcstr *)val;
  r = str->strdata;
  for (p = cstr; *p;)
    p += chartorune(r++, p);
  return(val);
}

unsigned int arc_strlen(arc *c, value v)
{
  return(((arcstr *)v)->length);
}

Rune arc_strindex(arc *c, value s, int index)
{
  return(((arcstr *)s)->strdata[index]);
}

Rune arc_strsetindex(arc *c, value s, int index, Rune r)
{
  return(((arcstr *)s)->strdata[index] = r);
}

/* XXX - this is extremely inefficient! It will allocate a new memory
   block and copy the old data, and add r to the end, and then free
   the block containing the old data. */
value arc_strcatrune(arc *c, value s, Rune r)
{
  arcstr *sp = (arcstr *)s;
  Rune *old;

  old = sp->strdata;
  sp->strdata = (Rune *)__arc_alloc((struct mm_ctx *)c->mm_ctx,
				    sizeof(Rune)*(sp->length+1));
  
  memcpy(sp->strdata, old, sizeof(Rune)*sp->length);
  *(sp->strdata + sp->length) = r;
  sp->length++;
  __arc_free((struct mm_ctx *)c->mm_ctx, old);
  return(s);
}

value arc_substr(arc *c, value s, unsigned int sidx, unsigned int eidx)
{
  unsigned int len, nlen;
  value ns;

  len = arc_strlen(c, s);;
  if (eidx > len)
    eidx = len;
  nlen = eidx - sidx;
  ns = str_alloc(c, nlen);
  memcpy(((arcstr *)ns)->strdata, ((arcstr *)s)->strdata + sidx,
	 nlen*sizeof(Rune));
  return(ns);
}

value arc_strcat(arc *c, value s1, value s2)
{
  value newstr;
  int len;
  Rune *rptr;

  len = ((arcstr *)s1)->length + ((arcstr *)s2)->length;
  newstr = str_alloc(c, len);
  rptr = ((arcstr *)newstr)->strdata;
  memcpy(rptr, ((arcstr *)s1)->strdata, sizeof(Rune)*((arcstr *)s1)->length);
  rptr += ((arcstr *)s1)->length;
  memcpy(rptr, ((arcstr *)s2)->strdata, sizeof(Rune)*((arcstr *)s2)->length);
  return(newstr);
}

unsigned int arc_strutflen(arc *c, value s)
{
  unsigned int i, count;
  char buf[UTFmax];
  Rune *r = ((arcstr *)s)->strdata;

  for (i=0; i<arc_strlen(c, s); i++) {
    count += runetochar(buf, r);
    r++;
  }
  return(count);
}

int arc_strrune(arc *c, value s, Rune r)
{
  unsigned int i;
  Rune *rp = ((arcstr *)s)->strdata;

  for (i=0; i<arc_strlen(c, s); i++) {
    if (*rp++ == r)
      return(i);
  }
  return(-1);
}

char *arc_str2cstr(arc *c, value s, char *ptr)
{
  unsigned int i, nc;
  char *p;
  Rune *r;

  p = ptr;
  r = ((arcstr *)s)->strdata;
  for (i=0; i<arc_strlen(c, s); i++) {
    nc = runetochar(p, r++);
    p += nc;
  }
  *p = 0;			/* null terminator */
  return(ptr);
}

int arc_is_str_cstr(arc *c, const char *s1, const value s2)
{
  const arcstr *ss = (arcstr *)s2;
  Rune r, *rp;
  const char *p;

  if ((unsigned)utflen(s1) != ss->length)
    return(0);

  for (rp = ss->strdata, p = s1; *p;) {
    p += chartorune(&r, p);
    if (*rp++ != r)
      return(0);
  }
  return(1);
}

value arc_strdup(arc *c, const value s)
{
  const arcstr *ss = (arcstr *)s;

  return(arc_string_new_str(c, ss->length, ss->strdata));
}
