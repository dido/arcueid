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
/* This is more or less the UTF-8 handling code from the Inferno/Plan
   9 sources.  It needed to be modified to handle Astral plane
   characters and for full RFC 2279 compliance, but its fundamentals
   seem to be sound.  */

#include "utf.h"

/* Definitions from Plan 9/Inferno UTF-8 library */
enum {
  Bit1	= 7,
  Bitx	= 6,
  Bit2	= 5,
  Bit3	= 4,
  Bit4	= 3,
  Bit5  = 2,			/* Added for RFC 2279 compliance */
  Bit6  = 1,			/* Added for RFC 2279 compliance */
  Bit7  = 0,

  T1	= ((1<<(Bit1+1))-1) ^ 0xFF, /* 0000 0000 */
  Tx	= ((1<<(Bitx+1))-1) ^ 0xFF, /* 1000 0000 */
  T2	= ((1<<(Bit2+1))-1) ^ 0xFF, /* 1100 0000 */
  T3	= ((1<<(Bit3+1))-1) ^ 0xFF, /* 1110 0000 */
  T4	= ((1<<(Bit4+1))-1) ^ 0xFF, /* 1111 0000 */
  T5    = ((1<<(Bit5+1))-1) ^ 0xFF, /* 1111 1000 */
  T6    = ((1<<(Bit6+1))-1) ^ 0xFF, /* 1111 1100 */
  T7    = ((1<<(Bit7+1))-1) ^ 0xFF, /* 1111 1110 -- invalid */

  Rune1	= (1<<(Bit1+0*Bitx))-1,	/* 0x0000007F */
  Rune2	= (1<<(Bit2+1*Bitx))-1,	/* 0x000007FF */
  Rune3	= (1<<(Bit3+2*Bitx))-1,	/* 0x0000FFFF */
  Rune4 = (1<<(Bit4+3*Bitx))-1,	/* 0x001FFFFF */
  Rune5 = (1<<(Bit5+4*Bitx))-1, /* 0x03FFFFFF */
  /*  Rune6 = (1<<(Bit6+5*Bitx))-1, */
  Rune6 = 0x7FFFFFFF,

  Maskx	= (1<<Bitx)-1,		/* 0011 1111 */
  Testx	= Maskx ^ 0xFF,		/* 1100 0000 */

  Bad	= Runeerror,

};

int chartorune(Rune *rune, const char *str)
{
  int c, c1, c2, c3, c4, c5;
  long l;

  /*
   * one character sequence
   *	00000-0007F => T1
   */
  c = *(unsigned char *)str;
  if(c < Tx) {
    *rune = c;
    return 1;
  }

  /*
   * two character sequence
   *	0080-07FF => T2 Tx
   */
  c1 = *(unsigned char*)(str+1) ^ Tx;
  if (c1 & Testx)
    goto bad;
  if (c < T3) {
    if (c < T2)
      goto bad;
    l = ((c << Bitx) | c1) & Rune2;
    if (l <= Rune1)
      goto bad;
    *rune = l;
    return 2;
  }

  /*
   * three character sequence
   *	0800-FFFF => T3 Tx Tx
   */
  c2 = *(unsigned char *)(str+2) ^ Tx;
  if (c2 & Testx)
    goto bad;
  if (c < T4) {
    l = ((((c << Bitx) | c1) << Bitx) | c2) & Rune3;
    if(l <= Rune2 || (l >= 0xd800 && l <= 0xdfff))
      goto bad;
    *rune = l;
    return 3;
  }

  /* Four character sequence 0001_0000-001F_FFFF, T4 Tx Tx Tx */
  c3 = *(unsigned char *)(str+3) ^ Tx;
  if (c3 & Testx)
    goto bad;
  if (c < T5) {
    l = ((((((c << Bitx) | c1) << Bitx) | c2) << Bitx) | c3) & Rune4;
    if (l <= Rune3)
      goto bad;
    *rune = l;
    return(4);
  }

  /* Five character sequence 00200000-03FFFFFF, T5 Tx Tx Tx Tx*/
  c4 = *(unsigned char *)(str+4) ^ Tx;
  if (c4 & Testx)
    goto bad;
  if (c < T6) {
    l = ((((((((c << Bitx) | c1) << Bitx) | c2) << Bitx) | c3) << Bitx) | c4) & Rune5;
    if (l <= Rune4)
      goto bad;
    *rune = l;
    return(5);
  }

  /* Six character sequence 04000000-7FFFFFFF, T6 Tx Tx Tx Tx Tx */
  c5 = *(unsigned char *)(str+5) ^ Tx;
  if (c5 & Testx)
    goto bad;
  if (c < T7) {
    l = ((((((((((c << Bitx) | c1) << Bitx) | c2) << Bitx) | c3) << Bitx) | c4) << Bitx) | c5) & Rune6;
    if (l <= Rune5)
      goto bad;
    *rune = l;
    return(6);
  }


  /*
   * bad decoding
   */
 bad:
  *rune = Bad;
  return 1;
}

int utflen(const char *s)
{
  int c;
  long n;
  Rune rune;

  n = 0;
  for (;;) {
    c = *(unsigned char*)s;
    if (c < 0x80) {
      if(c == 0)
	return(n);
      s++;
    } else
      s += chartorune(&rune, s);
    n++;
  }
  return(0);
}

int runetochar(char *str, Rune *rune)
{
  Rune c;

  /*
   * one character sequence
   *	00000-0007F => 00-7F
   */
  c = *rune;
  if(c <= Rune1) {
    str[0] = c;
    return(1);
  }

  /*
   * two character sequence
   *	0080-07FF => T2 Tx
   */
  if(c <= Rune2) {
    str[0] = T2 | (c >> 1*Bitx);
    str[1] = Tx | (c & Maskx);
    return(2);
  }

  /*
   * three character sequence
   *	0800-FFFF => T3 Tx Tx
   */
  if (c < Rune3) {
    str[0] = T3 |  (c >> 2*Bitx);
    str[1] = Tx | ((c >> 1*Bitx) & Maskx);
    str[2] = Tx |  (c & Maskx);
    return(3);
  }

  /* Four character sequence 0001_0000-001F_FFFF, T4 Tx Tx Tx */
  if (c < Rune4) {
    str[0] = T3 |  (c >> 3*Bitx);
    str[1] = Tx | ((c >> 2*Bitx) & Maskx);
    str[2] = Tx | ((c >> 1*Bitx) & Maskx);
    str[3] = Tx |  (c & Maskx);
    return(4);
  }

  /* Five character sequence 00200000-03FFFFFF, T5 Tx Tx Tx Tx*/
  if (c < Rune5) {
    str[0] = T5 |  (c >> 4*Bitx);
    str[1] = Tx | ((c >> 3*Bitx) & Maskx);
    str[2] = Tx | ((c >> 2*Bitx) & Maskx);
    str[3] = Tx | ((c >> 1*Bitx) & Maskx);
    str[4] = Tx |  (c & Maskx);
    return(5);
  }

  /* Six character sequence 04000000-7FFFFFFF, T6 Tx Tx Tx Tx Tx */
  if (c < Rune6) {
    str[0] = T6 |  (c >> 5*Bitx);
    str[1] = Tx | ((c >> 4*Bitx) & Maskx);
    str[2] = Tx | ((c >> 3*Bitx) & Maskx);
    str[3] = Tx | ((c >> 2*Bitx) & Maskx);
    str[4] = Tx | ((c >> 1*Bitx) & Maskx);
    str[5] = Tx |  (c & Maskx);
    return(6);
  }
  /* invalid rune */
  return(0);
}
