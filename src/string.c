/* 
  Copyright (C) 2012 Rafael R. Sevilla

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
#include "utf.h"

typedef struct {
  int len;
  Rune str[1];
} string;

#define STRREP(v) ((string *)REP(v))

static value string_pprint(arc *c, value v)
{
  return(CNIL);
}

value arc_mkstringlen(arc *c, int length)
{
  struct cell *ss;
  string *strdata;

  ss = (struct cell *)c->alloc(c, sizeof(struct cell) - sizeof(value) +
			       sizeof(string) + (length-1)*sizeof(Rune));
  ss->_type = T_STRING;
  ss->pprint = string_pprint;
  ss->marker = __arc_null_marker;
  ss->sweeper = __arc_null_sweeper;
  strdata = (string *)ss->_obj;
  strdata->len = length;
  return((value)ss);
}

/* Make string from UCS-4 Runes */
value arc_mkstring(arc *c, const Rune *data, int length)
{
  value str;

  str = arc_mkstringlen(c, length);
  memcpy(((string *)REP(str))->str, data, length*sizeof(Rune));
  STRREP(str)->len = length;
  return(str);
}

/* Make string from a UTF-8 C string */
value arc_mkstringc(arc *c, const char *s)
{
  value str;
  int len, ch;
  Rune *runeptr;

  len = utflen(s);
  str = arc_mkstringlen(c, len);
  runeptr = ((string *)REP(str))->str;
  for (;;) {
    ch = *(unsigned char *)s;
    if (ch == 0)
      break;
    s += chartorune(runeptr++, s);
  }
  STRREP(str)->len = len;
  return(str);
}

static value char_pprint(arc *c, value v)
{
  /* XXX fill this in */
  return(CNIL);
}

value arc_mkchar(arc *c, Rune r)
{
  struct cell *ss;

  ss = (struct cell *)c->alloc(c, sizeof(struct cell) - sizeof(value) +
			       sizeof(Rune));
  ss->_type = T_STRING;
  ss->pprint = char_pprint;
  ss->marker = __arc_null_marker;
  ss->sweeper = __arc_null_sweeper;
  *((Rune *)ss->_obj) = r;
  return((value)ss);
}

/* Most of these trivial and inefficient functions should
   become more complex and efficient later--they'll become
   Boehm-Atkinson-Plass rope structures. */

int arc_strlen(arc *c, value v)
{
  return(STRREP(v)->len);
}

Rune arc_strindex(arc *c, value v, int index)
{
  return(STRREP(v)->str[index]);
}

Rune arc_strsetindex(arc *c, value v, int index, Rune ch)
{
  STRREP(v)->str[index] = ch;
  return(ch);
}

/* XXX - this is extremely inefficient! */
value arc_strcatc(arc *c, value v1, Rune ch)
{
  Rune *runeptr;
  value newstr;

  newstr = arc_mkstringlen(c, STRREP(v1)->len + 1);
  runeptr = STRREP(newstr)->str;
  memcpy(runeptr, STRREP(v1)->str, STRREP(v1)->len*sizeof(Rune));
  runeptr += STRREP(v1)->len*sizeof(Rune);
  *runeptr = ch;
  return(newstr);
}

value arc_substr(arc *c, value s, int sidx, int eidx)
{
  int len, nlen;
  value ns;

  len = arc_strlen(c, s);
  if (eidx > len)
    eidx = len;
  nlen = eidx - sidx;
  ns = arc_mkstringlen(c, nlen);
  memcpy(STRREP(ns)->str, STRREP(s)->str + sidx, nlen*sizeof(Rune));
  return(ns);
}

value arc_strcat(arc *c, value v1, value v2)
{
  value newstr;
  int len;
  Rune *runeptr;

  len = STRREP(v1)->len + STRREP(v2)->len;
  newstr = arc_mkstringlen(c, len);
  runeptr = STRREP(newstr)->str;
  memcpy(runeptr, STRREP(v1)->str, STRREP(v1)->len*sizeof(Rune));
  runeptr += STRREP(v1)->len;
  memcpy(runeptr, STRREP(v2)->str, STRREP(v2)->len*sizeof(Rune));
  return(newstr);
}
