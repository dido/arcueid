/* 
  Copyright (C) 2009 Rafael R. Sevilla

  This file is part of CArc

  CArc is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; either version 3 of the
  License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA
  02110-1301 USA.
*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "carc.h"
#include "alloc.h"
#include "utf.h"
#include "arith.h"
#include "../config.h"

static Rune scan(carc *c, value str, int *index);
static value read_list(carc *c, value str, int *index);
static value read_anonf(carc *c, value str, int *index);
static value read_quote(carc *c, value str, int *index, const char *sym);
static value read_comma(carc *c, value str, int *index);
static value read_string(carc *c, value str, int *index);
static value read_special(carc *c, value str, int *index);
static void read_comment(carc *c, value str, int *index);
static value read_symbol(carc *c, value str, int *index);
static Rune strgetc(carc *c, value str, int *index);
static void strungetc(carc *c, int *index);

value carc_read(carc *c, value str, int *index, value *pval)
{
  Rune ch;

  while ((ch = scan(c, str, index)) != Runeerror) {
    switch (ch) {
    case '(':
      *pval = read_list(c, str, index);
      return(CTRUE);
    case ')':
      c->signal_error(c, "misplaced right paren");
      return(CNIL);
    case '[':
      *pval = read_anonf(c, str, index);
      return(CTRUE);
    case ']':
      c->signal_error(c, "misplaced right bracket");
      return(CNIL);
    case '\'':
      *pval = read_quote(c, str, index, "quote");
      return(CTRUE);
    case '`':
      *pval = read_quote(c, str, index, "quasiquote");
      return(CTRUE);
    case ',':
      *pval = read_comma(c, str, index);
      return(CTRUE);
    case '"':
      *pval = read_string(c, str, index);
      return(CTRUE);
    case '#':
      *pval = read_special(c, str, index);
      return(CTRUE);
    case ';':
      read_comment(c, str, index);
      return(CTRUE);
    default:
      *pval = read_symbol(c, str, index);
      return(CTRUE);
    }
  }
  return(CNIL);
}

static value read_list(carc *c, value str, int *index)
{
  value top, val, last;
  Rune ch;

  top = val = last = CNIL;
  while ((ch = scan(c, str, index)) != Runeerror) {
    switch (ch) {
    case ';':
      read_comment(c, str, index);
      break;
    case ')':
      return(top);
    default:
      if (!carc_read(c, str, index, &val))
	c->signal_error(c, "unexpected end of string");
      val = cons(c, val, CNIL);
      if (last)
	scdr(last, val);
      else
	top = val;
      last = val;
      break;
    }
  }
  c->signal_error(c, "unexpected end of string");
  return(CNIL);
}

static void read_comment(carc *c, value str, int *index)
{
  Rune ch;

  while ((ch = strgetc(c, str, index)) != Runeerror && !ucisnl(ch))
    ;
  if (ch != Runeerror)
    strungetc(c, index);
}

static int issym(Rune ch)
{
  char *p;

  if (ucisspace(ch)) 
    return(0);
  for (p = "()';[]"; *p != '\0';) {
    if ((Rune)*p++ == ch)
      return(0);
  }
  return(1);
}

#define STRMAX 256

static value getsymbol(carc *c, value str, int *index)
{
  Rune buf[STRMAX];
  Rune ch;
  int i;
  value sym, nstr;

  sym = CNIL;
  i=0;
  while ((ch = strgetc(c, str, index)) != Runeerror && issym(ch)) {
    if (i < STRMAX) {
      buf[i++] = ch;
    } else {
      nstr = carc_mkstring(c, buf, i);
      sym = (sym == CNIL) ? nstr : carc_strcat(c, sym, nstr);
      i = 0;
    }
  }
  if (i==0 && sym == CNIL)
    return(CNIL);
  nstr = carc_mkstring(c, buf, i);
  sym = (sym == CNIL) ? nstr : carc_strcat(c, sym, nstr);

  strungetc(c, index);
  return(sym);
}

/* parse a symbol name or number */
static value read_symbol(carc *c, value str, int *index)
{
  value sym, num;

  if ((sym = getsymbol(c, str, index)) == CNIL)
    c->signal_error(c, "expecting symbol name");
  num = carc_string2num(c, sym);
  return((num == CNIL) ? sym : num);
}

/* scan for first non-blank character */
static Rune scan(carc *c, value str, int *index)
{
  Rune ch;

  while ((ch = strgetc(c, str, index)) != Runeerror && ucisspace(ch))
    ;
  return(ch);
}

static Rune strgetc(carc *c, value str, int *index)
{
  if (*index < carc_strlen(c, str))
    return(carc_strindex(c, str, (*index)++));
  return(Runeerror);
}

static void strungetc(carc *c, int *index)
{
  if (*index <= 0)
    return;
  (*index)--;
}

static value rune2dig(Rune r, int radix)
{
  Rune rl;
  value v;

  if (!ucisalnum(r))
    return(CNIL);
  rl = tolower(r);
  if (rl >= 0x30 && rl <= 0x39)
    v = rl - 0x30;
  else if (rl >= 0x61 && rl <= 0x7a)
    v = (rl - 0x61) + 10;
  if (v > radix)
    return(CNIL);
  return(INT2FIX(v));
}

static double str2flonum(carc *c, value str, int index, int imagflag)
{
  int state = 1, expn = 0, expnsign = 1;
  double sign = 1.0, mantissa=0.0, mult=0.1, fnum;
  value digitval, imag;
  Rune ch;

  while ((ch = strgetc(c, str, &index)) != Runeerror) {
    switch (state) {
    case 1:
      /* sign */
      switch (ch) {
      case '-':
	sign = -1;
	state = 2;
	break;
      case '+':
	sign = 1;
	state = 2;
	break;
      default:
	if (!isdigit(ch))
	  return(CNIL);
	strungetc(c, &index);
	state = 2;
	break;
      }
      break;
    case 2:
      /* non-fractional part of mantissa */
      switch (ch) {
      case '.':
	state = 3;
	break;
      case 'e':
      case 'E':
	state = 4;
	break;
      case '+':
      case '-':
	/* Complex */
	if (imagflag) {
	  return(CNIL);
	} else {
	  /* read the imaginary part */
	  imag = str2flonum(c, str, index-1, 1);
	  if (TYPE(imag) != T_COMPLEX)
	    return(CNIL);
	  REP(imag)._complex.re = sign * (mantissa * pow(10, expnsign * expn));
	  return(imag);
	}
	break;
      case 'i':
      case 'I':
      case 'j':
      case 'J':
	/* imaginary */
	return(carc_mkcomplex(c, 0.0,
			      sign * (mantissa
				      * pow(10, expnsign * expn))));
      default:
	if (!isdigit(ch))
	  return(CNIL);
	mantissa = (mantissa * 10) + FIX2INT(rune2dig(ch, 10));
	break;
      }
      break;
    case 3:
      /* fractional part of mantissa */
      switch (ch) {
      case 'e':
      case 'E':
	state = 4;
	break;
      case '+':
      case '-':
	/* Complex */
	if (imagflag) {
	  return(CNIL);
	} else {
	  /* read the imaginary part */
	  imag = str2flonum(c, str, index-1, 1);
	  if (TYPE(imag) != T_COMPLEX)
	    return(CNIL);
	  REP(imag)._complex.re = sign * (mantissa * pow(10, expnsign * expn));
	  return(imag);
	}
	break;
      case 'i':
      case 'I':
      case 'j':
      case 'J':
	/* imaginary */
	return(carc_mkcomplex(c, 0.0,
			      sign * (mantissa
				      * pow(10, expnsign * expn))));
      default:
	if (!isdigit(ch))
	  return(CNIL);
	mantissa += FIX2INT(rune2dig(ch, 10)) * mult;
	mult *= 0.1;
	break;
      }
      break;
    case 4:
      /* exponent sign */
      switch (ch) {
      case '-':
	expnsign = -1;
	state = 5;
	break;
      case '+':
	expnsign = 1;
	state = 5;
	break;
      default:
	if (!isdigit(ch))
	  return(CNIL);
	strungetc(c, &index);
	state = 5;
	break;
      }
      break;
    case 5:
      switch (ch) {
      case '+':
      case '-':
	/* Complex */
	if (imagflag) {
	  return(CNIL);
	} else {
	  /* read the imaginary part */
	  imag = str2flonum(c, str, index-1, 1);
	  if (TYPE(imag) != T_COMPLEX)
	    return(CNIL);
	  REP(imag)._complex.re = sign * (mantissa * pow(10, expnsign * expn));
	  return(imag);
	}
	break;
      case 'i':
      case 'I':
      case 'j':
      case 'J':
	/* imaginary */
	return(carc_mkcomplex(c, 0.0,
			      sign * (mantissa
				      * pow(10, expnsign * expn))));
      default:
	/* exponent magnitude */
	digitval = rune2dig(ch, 10);
	if (digitval == CNIL)
	  return(CNIL);
	expn = expn * 10 + FIX2INT(digitval);
	break;
      }
      break;
    }
  }
  /* Combine */
  fnum = sign * (mantissa * pow(10, expnsign * expn));
  return(carc_mkflonum(c, fnum));
}

static value string2numindex(carc *c, value str, int index, int rational)
{
  int state = 1, sign = 1, radsel = 0;
  Rune ch;
  value nval = INT2FIX(0), digitval, radix = INT2FIX(10), denom;

  while ((ch = strgetc(c, str, &index)) != Runeerror) {
    switch (state) {
    case 1:
      /* sign */
      switch (ch) {
      case '-':
	sign = -1;
	state = 2;
	break;
      case '+':
	sign = 1;
	state = 2;
	break;
      case '.':
	return(str2flonum(c, str, 0, 0));
	break;
      default:
	if (!isdigit(ch))
	  return(CNIL);
	strungetc(c, &index);
	state = 2;
	break;
      }
      break;
    case 2:
      /* digits, or possible radix */
      switch (ch) {
      case '0':
	radix = INT2FIX(8);
	state = 3;
	break;
      case '.':
	return(str2flonum(c, str, 0, 0));
	break;
      default:
	if (!isdigit(ch))
	  return(CNIL);
	/* more digits */
	strungetc(c, &index);
	state = 4;
	break;
      }
      break;
    case 3:
      /* digits, or possible radix */
      switch (ch) {
      case 'x':
	radix = INT2FIX(16);
	state = 4;
	break;
      case 'b':
	radix = INT2FIX(2);
	state = 4;
	break;
      case '.':
	return(str2flonum(c, str, 0, 0));
	break;
      default:
	if (!isdigit(ch))
	  return(CNIL);
	/* more digits */
	strungetc(c, &index);
	state = 4;
	break;
      }
      break;
    case 4:
      /* digits */
      switch (ch) {
      case '.':
      case 'e':
      case 'E':
	return(str2flonum(c, str, 0, 0));
	break;
      case 'r':
	/* Limbo-style radix selector: the base radix should
	   still be 10, and the value should be between 2 and 36 */
	if (radix == INT2FIX(10) && TYPE(nval) == T_FIXNUM
	    && (FIX2INT(nval) >= 2 && FIX2INT(nval) <= 36)) {
	  radix = nval;
	  nval = INT2FIX(0);
	  state = 5;
	} else if (!radsel) {
	  return(CNIL);		/* invalid radix selector */
	}
	break;
      case '/':
	if (rational)
	  return(CNIL);
	denom = string2numindex(c, str, index, 1);
	if (TYPE(denom) == T_FIXNUM || TYPE(denom) == T_FLONUM)
	  return(__carc_div2(c, nval, denom));
	else
	  return(CNIL);
	break;
      case 'i':
      case 'I':
      case 'j':
      case 'J':
	return(str2flonum(c, str, 0, 0));
      case '+':
      case '-':
	return(str2flonum(c, str, 0, 0));
      default:
	/* Digits */
	digitval = rune2dig(ch, radix);
	if (digitval == CNIL)
	  return(CNIL);
	nval = __carc_add2(c, __carc_mul2(c, nval, radix), digitval);
	break;
      }
      break;
    case 5:
      /* Digits */
      digitval = rune2dig(ch, radix);
      if (digitval == CNIL)
	return(CNIL);
      nval = __carc_add2(c, __carc_mul2(c, nval, radix), digitval);
      break;
    }
  }
  return(nval);
}

value carc_string2num(carc *c, value str)
{
  return(string2numindex(c, str, 0, 0));
}

/* XXX: stub! */
static value read_anonf(carc *c, value str, int *index)
{
  return(CNIL);
}

/* XXX: stub! */
static value read_quote(carc *c, value str, int *index, const char *sym)
{
  return(CNIL);
}

/* XXX: stub! */
static value read_comma(carc *c, value str, int *index)
{
  return(CNIL);
}

/* XXX: stub! */
static value read_string(carc *c, value str, int *index)
{
  return(CNIL);
}

/* XXX: stub! */
static value read_special(carc *c, value str, int *index)
{
  return(CNIL);
}
