/* The authors of this software are Rob Pike and Ken Thompson.

   Copyright (c) 1994-1999 Lucent Technologies Inc.  All rights 
   reserved.

   Revisions Copyright (c) 2000-2007 Vita Nuova Holdings
   Limited (www.vitanuova.com).  All rights reserved.
 
   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without
   restriction, including without limitation the rights to use, copy,
   modify, merge, publish, distribute, sublicense, and/or sell copies
   of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
 */

#ifndef UTF_H

#define UTF_H

#include <ctype.h>
#include "arcueid.h"

#define	nelem(x)	(sizeof(x)/sizeof((x)[0]))

enum
{
  UTFmax = 6,	     /* maximum bytes per rune, as per RFC 2279 */
  Runesync = 0x80,   /* cannot represent part of a UTF sequence (<) */
  Runeself = 0x80,   /* rune and UTF sequences are the same (<) */
  Runeerror = 0x80   /* decoding error in UTF */
};

extern int chartorune(Rune *rune, const char *str);
extern int utflen(const char *s);
extern int runetochar(char *str, Rune *rune);
extern int fullrune(char *str, int n);
extern char *utfrune(char *s, long c);

static inline int ucisspace(Rune cc)
{
  return((cc >= 0x0009 && cc <= 0x000d) || (cc == 0x20) || (cc == 0x0085)
	 || (cc == 0x00a0) || (cc == 0x1680) || (cc == 0x180e)
	 || (cc >= 0x2000 && cc <= 0x200a) || (cc == 0x2028)
	 || (cc == 0x2029) || (cc == 0x202f) || (cc == 0x205f)
	 || (cc == 0x3000));
}

static inline int ucisnl(Rune cc)
{
  return(cc == 0x000d || cc == 0x000a || cc == 0x0085 || cc == 0x000c
	 || cc == 0x2028 || cc == 0x2029);
}

static inline int ucisalnum(Rune cc)
{
  if (cc > 0x007f)
    return(0);
  return(isalnum(cc));
}

#endif
