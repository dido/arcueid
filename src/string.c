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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA
  02110-1301 USA.
*/
#include <inttypes.h>
#include <string.h>
#include "carc.h"
#include "alloc.h"
#include "utf.h"

/* Make a string based on UCS-4 data */
value carc_mkstring(carc *c, const Rune *data, int length)
{
  Bhdr *b;
  value str;

  str = c->get_cell(c);
  BTYPE(str) = T_STRING;
  REP(str)._str.length = length;
  REP(str)._str.str = c->get_block(c, length*sizeof(Rune));
  memcpy(REP(str)._str.str, data, length*sizeof(Rune));
  D2B(b, REP(str)._str.str);
  b->magic = MAGIC_I;		/* mark the block as immutable */
  return(str);
}

/* Make a string based on a C string, considering the string as a
   UTF-8 string */
value carc_mkstringc(carc *c, const char *s)
{
  Bhdr *b;
  value str;
  int len, ch;
  Rune *runeptr;

  len = utflen(s);
  str = c->get_cell(c);
  BTYPE(str) = T_STRING;
  REP(str)._str.length = len;
  runeptr = REP(str)._str.str = c->get_block(c, len*sizeof(Rune));
  D2B(b, REP(str)._str.str);
  b->magic = MAGIC_I;		/* mark the block as immutable */
  for (;;) {
    ch = *(unsigned char *)s;
    if (ch == 0)
      break;
    s += chartorune(runeptr++, s);
  }
  return(str);
}

