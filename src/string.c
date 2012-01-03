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
#include <inttypes.h>
#include <string.h>
#include "arcueid.h"
#include "alloc.h"
#include "utf.h"

value arc_mkstringlen(arc *c, int length)
{
  value str;

  str = c->get_cell(c);
  BTYPE(str) = T_STRING;
  REP(str)._str.length = length;
  REP(str)._str.str = c->get_block(c, length*sizeof(Rune));
  BLOCK_IMM(REP(str)._str.str);
  return(str);

}

value arc_newstring(arc *c, value length)
{
  if (TYPE(length) != T_FIXNUM || FIX2INT(length) < 0) {
    arc_err_cstrfmt(c, "newstring: expects argument of type <non-negative exact integer>; given %P", length);
    return(CNIL);
  }
  return(arc_mkstringlen(c, FIX2INT(length)));
}

/* Make a string based on UCS-4 data */
value arc_mkstring(arc *c, const Rune *data, int length)
{
  value str;

  str = arc_mkstringlen(c, length);
  memcpy(REP(str)._str.str, data, length*sizeof(Rune));
  return(str);
}

/* Make a string based on a C string, considering the string as a
   UTF-8 string */
value arc_mkstringc(arc *c, const char *s)
{
  value str;
  int len, ch;
  Rune *runeptr;

  len = utflen(s);
  str= arc_mkstringlen(c, len);
  runeptr = REP(str)._str.str;
  for (;;) {
    ch = *(unsigned char *)s;
    if (ch == 0)
      break;
    s += chartorune(runeptr++, s);
  }
  return(str);
}

value arc_mkchar(arc *c, Rune r)
{
  value ch;

  ch = c->get_cell(c);
  BTYPE(ch) = T_CHAR;
  REP(ch)._char = r;
  return(ch);
}

/* Most of these trivial and inefficient functions should
   become more complex and efficient later--they'll become
   Boehm-Atkinson-Plass rope structures. */

int arc_strlen(arc *c, value v)
{
  return(REP(v)._str.length);
}

Rune arc_strindex(arc *c, value v, int index)
{
  return(REP(v)._str.str[index]);
}

Rune arc_strsetindex(arc *c, value v, int index, Rune ch)
{
  REP(v)._str.str[index] = ch;
  return(ch);
}

/* XXX - this is extremelly inefficient! */
value arc_strcatc(arc *c, value v1, Rune ch)
{
  Rune *runeptr;
  value newstr;

  newstr = arc_mkstringlen(c, REP(v1)._str.length + 1);
  runeptr = REP(newstr)._str.str;
  memcpy(runeptr, REP(v1)._str.str, REP(v1)._str.length*sizeof(Rune));
  runeptr += REP(v1)._str.length;
  *runeptr = ch;
  return(newstr);
}

value arc_strcat(arc *c, value v1, value v2)
{
  value newstr;
  int len;
  Rune *runeptr;

  len = REP(v1)._str.length + REP(v2)._str.length;
  newstr = arc_mkstringlen(c, len);
  runeptr = REP(newstr)._str.str;
  memcpy(runeptr, REP(v1)._str.str, REP(v1)._str.length*sizeof(Rune));
  runeptr += REP(v1)._str.length;
  memcpy(runeptr, REP(v2)._str.str, REP(v2)._str.length*sizeof(Rune));
  return(newstr);
}

Rune arc_strgetc(arc *c, value str, int *index)
{
  if (*index < arc_strlen(c, str))
    return(arc_strindex(c, str, (*index)++));
  return(Runeerror);
}

void arc_strungetc(arc *c, int *index)
{
  if (*index <= 0)
    return;
  (*index)--;
}

value arc_strchr(arc *c, value str, Rune ch)
{
  int i;

  for (i=0; i<arc_strlen(c, str); i++) {
    if (arc_strindex(c, str, i) == ch)
      return(INT2FIX(i));
  }
  return(CNIL);
}

/* This is a simple binary comparison of strings, giving a
   lexicographic ordering of the input strings based on the Unicode
   code point ordering.

   XXX: We should eventually implement UTS #10 (Unicode Collation
   algorithm) for this function someday and a parser for Unicode
   collation element tables. */
value arc_strcmp(arc *c, value vs1, value vs2)
{
  Rune c1, c2, *s1, *s2;
  int sl1, sl2;

  sl1 = REP(vs1)._str.length;
  sl2 = REP(vs2)._str.length;
  s1 = REP(vs1)._str.str;
  s2 = REP(vs2)._str.str;
  for (;;) {
    if (sl1 == 0 && sl2 != 0)
      return(INT2FIX(-1));
    if (sl2 == 0 && sl1 != 0)
      return(INT2FIX(1));
    if (sl1 == 0 && sl2 == 0)
      return(INT2FIX(0));
    c1 = *s1++;
    c2 = *s2++;
    if (c1 != c2) {
      if (c1 > c2)
	return(INT2FIX(1));
      return(INT2FIX(-1));
    }
    sl1--;
    sl2--;
  }
  return(INT2FIX(0));
}

value arc_strutflen(arc *c, value str)
{
  int i, count;
  char buf[UTFmax];
  Rune r;

  count = 0;
  for (i=0; i<arc_strlen(c, str); i++) {
    r = arc_strindex(c, str, i);
    count += runetochar(buf, &r);
  }
  return(INT2FIX(count));
}

/* Convert a string into a C string.  The pointer ptr must be big
   enough to store all the data. BE SURE OF THIS BEFORE USING THIS
   FUNCTION! */
void arc_str2cstr(arc *c, value str, char *ptr)
{
  int i, nc;
  char *p;
  Rune r;

  p = ptr;
  for (i=0; i<arc_strlen(c, str); i++) {
    r = arc_strindex(c, str, i);
    nc = runetochar(p, &r);
    p += nc;
  }
  *p = 0;			/* null terminator */
}
