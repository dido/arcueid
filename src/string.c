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
#include "arith.h"
#include "builtins.h"

typedef struct {
  int len;
  Rune str[1];
} string;

#define STRREP(v) ((string *)REP(v))

#define PSTRMAX 1024

#if 0
static value string_pprint(arc *c, value sexpr, value *ppstr, value visithash)
{
  Rune buf[PSTRMAX], ch;
  int idx=0, i;
  char outstr[4];

  __arc_append_buffer(c, buf, &idx, PSTRMAX, '\"', ppstr);
  for (i=0; i<arc_strlen(c, sexpr); i++) {
    ch = arc_strindex(c, sexpr, i);
    if (ch < 32) {
      snprintf(outstr, 4, "%.3o", ch);
      __arc_append_buffer(c, buf, &idx, PSTRMAX, '\\', ppstr);
      __arc_append_buffer(c, buf, &idx, PSTRMAX, outstr[0], ppstr);
      __arc_append_buffer(c, buf, &idx, PSTRMAX, outstr[1], ppstr);
      __arc_append_buffer(c, buf, &idx, PSTRMAX, outstr[2], ppstr);
    } else {
      __arc_append_buffer(c, buf, &idx, PSTRMAX, ch, ppstr);
    }
  }
  __arc_append_buffer(c, buf, &idx, PSTRMAX, '\"', ppstr);
  __arc_append_buffer_close(c, buf, &idx, ppstr);
  return(*ppstr);
}
#endif

static unsigned long string_hash(arc *c, value v, arc_hs *s)
{
  int i;
  unsigned long len;

  len = arc_strlen(c, v);
  for (i=0; i<len; i++)
    arc_hash_update(s, (unsigned long)arc_strindex(c, v, i));
  return(len);
}

static value string_iscmp(arc *c, value v1, value v2)
{
  return((arc_strcmp(c, v1, v2) == 0) ? CTRUE : CNIL);
}

/* A string can be applied to a fixnum value */
static int string_apply(arc *c, value thr, value str)
{
  value fidx;
  Rune r;
  int index;

  if (arc_thr_argc(c, thr) != 1) {
    arc_err_cstrfmt(c, "application of a string expects 1 argument, given %d",
		    arc_thr_argc(c, thr));
    return(TR_RC);
  }
  fidx = arc_thr_pop(c, thr);
  if (TYPE(fidx) != T_FIXNUM) {
    arc_err_cstrfmt(c, "application of a string expects type <non-negative exact integer> as argument");
    return(TR_RC);
  }
  index = FIX2INT(fidx);

  if (index < 0) {
    arc_err_cstrfmt(c, "application of a string expects type <non-negative exact integer> as argument");
    return(TR_RC);
  }

  if (index >= arc_strlen(c, str)) {
    arc_err_cstrfmt(c, "string index %d out of range [0, %d] for string",
		    index, arc_strlen(c, str)-1);
    return(TR_RC);
  }
  r = arc_strindex(c, str, index);
  arc_thr_set_valr(c, thr, arc_mkchar(c, r));
  return(TR_RC);
}

value arc_mkstringlen(arc *c, int length)
{
  value str;
  string *strdata;


  str = arc_mkobject(c, sizeof(string) + (length-1)*sizeof(Rune), T_STRING);
  strdata = (string *)REP(str);
  strdata->len = length;
  return(str);
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

#if 0
static value char_pprint(arc *c, value v, value *ppstr, value visithash)
{
  /* XXX fill this in */
  return(CNIL);
}
#endif

static unsigned long char_hash(arc *c, value v, arc_hs *s)
{
  arc_hash_update(s, *((unsigned long *)REP(v)));
  return(1);
}

static value char_iscmp(arc *c, value v1, value v2)
{
  return((*(Rune *)REP(v1)) == (*(Rune *)REP(v2)) ? CTRUE : CNIL);
}

value arc_mkchar(arc *c, Rune r)
{
  value ch;
  ch = arc_mkobject(c, sizeof(Rune), T_CHAR);
  *((Rune *)REP(ch)) = r;
  return(ch);
}

Rune arc_char2rune(arc *c, value ch)
{
  return(*((Rune *)REP(ch)));;
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
  if (index > STRREP(v)->len)
    return(Runeerror);
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
  runeptr += STRREP(v1)->len;
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

/* This is a simple binary comparison of strings, giving a
   lexicographic ordering of the input strings based on the Unicode
   code point ordering.

   XXX: We should eventually implement UTS #10 (Unicode Collation
   algorithm) for this function someday and a parser for Unicode
   collation element tables. */
int arc_strcmp(arc *c, value v1, value v2)
{
  int len1, len2, len;
  int i;

  len1 = arc_strlen(c, v1);
  len2 = arc_strlen(c, v2);
  len = (len1 > len2) ? len2 : len1;
  for (i=0; i<len; i++) {
    int r1, r2;
    r1 = arc_strindex(c, v1, i);
    r2 = arc_strindex(c, v2, i);
    if (r1 > r2)
      return(1);
    if (r1 < r2)
      return(-1);
  }
  if (len1 > len2)
    return(1);
  if (len1 < len2)
    return(-1);
  return(0);
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

value arc_strchr(arc *c, value str, Rune ch)
{
  int i;

  for (i=0; i<arc_strlen(c, str); i++) {
    if (arc_strindex(c, str, i) == ch)
      return(INT2FIX(i));
  }
  return(CNIL);
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

static AFFDEF(char_xcoerce)
{
  AARG(obj, stype, arg);
  AFBEGIN;
  (void)arg;
  if (FIX2INT(AV(stype)) == T_CHAR)
    ARETURN(AV(obj));

  if (FIX2INT(AV(stype)) == T_FIXNUM || FIX2INT(AV(stype)) == T_BIGNUM
      || FIX2INT(AV(stype)) == T_INT)
    ARETURN(INT2FIX(arc_char2rune(c, AV(obj))));

  if (FIX2INT(AV(stype)) == T_STRING)
    ARETURN(arc_mkstring(c, (Rune *)REP(AV(obj)), 1));
  arc_err_cstrfmt(c, "cannot coerce");
  ARETURN(CNIL);
  AFEND;
}
AFFEND

static value coerce_flonum(arc *c, value obj, value argv)
{
  value val, base = argv;

  val = __arc_str2flo(c, obj, base, 0, arc_strlen(c, obj));
  if (val == CNIL) {
    arc_err_cstrfmt(c, "string->flonum cannot convert string to a flonum");
    return(CNIL);
  }
  return(val);
}

static value coerce_num(arc *c, value obj, value argv)
{
  int len, base;
  value res;

  base = FIX2INT(argv);
  len = arc_strlen(c, obj);

  /* 1. If string ends with 'i' or 'j', convert as complex */
  if (arc_strindex(c, obj, len-1) == 'i'
      || arc_strindex(c, obj, len-1) == 'j') {
    /* To do this, we have to find where the real part ends and
       the complex part begins.  The real part ends after a + or -
       that is NOT preceded by an E, P, or &, which is where the
       imaginary part begins.  The imaginary unit i/j must be at the
       end of the string.   Once we know where the parts are we can
       use the functions for extracting floating point numbers to
       get the real and imaginary parts of the number. */
    value re, im;
    int reend = -1, i, len, b = base;
    Rune r;

    len = arc_strlen(c, obj);
    for (i=0; i<len; i++) {
      r = arc_strindex(c, obj, i);
      if ((r == '-' || r == '+') && i-1 >= 0) {
	Rune prev = arc_strindex(c, obj, i-1);
	if (b < 14 && (prev == 'e' || prev == 'E'))
	  continue;
	if (b < 25 && (prev == 'p' || prev == 'P'))
	  continue;
	if (prev == '&')
	  continue;
	reend = i;
	break;
      }
    }
    if (reend < 0) {
      arc_err_cstrfmt(c, "cannot find end of real part of number", obj);
      return(CNIL);
    }
    r = tolower(arc_strindex(c, obj, len-1));
    if (r != 'i' && r != 'j') {
      arc_err_cstrfmt(c, "cannot find end of imaginary part of number", obj);
      return(CNIL);
    }
    re = __arc_str2flo(c, obj, base, 0, reend);
    im = __arc_str2flo(c, obj, base, reend, len-1);
    if (re == CNIL || im == CNIL) {
      arc_err_cstrfmt(c, "failed to parse complex number", obj);
      return(CNIL);
    }
    return(arc_mkcomplex(c, REPFLO(re) + I*REPFLO(im)));
  }

  /* 2. If string contains '.', convert as floating point. */
  if (!(arc_strchr(c, obj, '.') == CNIL))
    return(coerce_flonum(c, obj, argv));

  /* 3. If base is less than 14 and the string contains 'e/E',
        convert as floating point. */
  if (base < 14 && !(arc_strchr(c, obj, 'e') == CNIL
		     && arc_strchr(c, obj, 'E') == CNIL))
    return(coerce_flonum(c, obj, argv));

  /* 4. If base is less than 25 and the string contains 'p/P',
        convert as floating point. */
  if (base < 25 && !(arc_strchr(c, obj, 'p') == CNIL
		     && arc_strchr(c, obj, 'P') == CNIL))
    return(coerce_flonum(c, obj, argv));

  /* 5. If string contains '&', convert as floating point. */
  if (!(arc_strchr(c, obj, '&') == CNIL))
    return(coerce_flonum(c, obj, argv));

#ifdef HAVE_GMP_H
  /* 6. If string contains '/', convert as rational. */
  if (!(arc_strchr(c, obj, '/') == CNIL)) {
    value numer, denom, base = argv;
    int slashpos=-1;

    slashpos = FIX2INT(arc_strchr(c, obj, '/'));
    numer = __arc_str2int(c, obj, base, 0, slashpos);
    denom = __arc_str2int(c, obj, base, slashpos+1, arc_strlen(c, obj));
    if (numer == CNIL || denom == CNIL) {
      arc_err_cstrfmt(c, "string->flonum cannot convert to a rational");
      return(CNIL);
    }
    return(__arc_div2(c, numer, denom));
  }
#endif

  /* 7. Otherwise, consider string as representing an integer */
  res = __arc_str2int(c, obj, INT2FIX(base), 0, len);
  if (res != CNIL)
    return(res);
  arc_err_cstrfmt(c, "cannot coerce to numeric type");
  return(CNIL);

}

static AFFDEF(string_xcoerce)
{
  AARG(obj, stype, arg);
  AFBEGIN;

  if (FIX2INT(AV(stype)) == T_STRING)
    ARETURN(AV(obj));

  if (FIX2INT(AV(stype)) == T_SYMBOL)
    ARETURN(arc_intern(c, AV(obj)));

  if (FIX2INT(AV(stype)) == T_CONS) {
    value ncons = CNIL;
    int i;
    for (i=arc_strlen(c, AV(obj))-1; i>= 0; i--)
      ncons = cons(c, arc_mkchar(c, arc_strindex(c, AV(obj), i)), ncons);
    ARETURN(ncons);
  }

  if (!BOUND_P(AV(arg)))
    AV(arg) = INT2FIX(10);

  if (FIX2INT(AV(stype)) == T_FIXNUM || FIX2INT(AV(stype)) == T_BIGNUM) {
    char *cstr, *endptr;
    long fix;

    cstr = alloca(sizeof(char) * (arc_strlen(c, AV(obj)) + 1));
    arc_str2cstr(c, AV(obj), cstr);

    fix = strtol(cstr, &endptr, FIX2INT(AV(arg)));

    if (*endptr != '\0') {
      arc_err_cstrfmt(c, "cannot coerce");
      ARETURN(CNIL);
    }

    if (fix >= FIXNUM_MIN && fix <= FIXNUM_MAX)
      ARETURN(INT2FIX(fix));

#ifdef HAVE_GMP_H
    /* we have something that should be a bignum */
    do {
      value bn;

      bn = arc_mkbignuml(c, 0L);
      if (mpz_set_str(REPBNUM(bn), cstr, FIX2INT(AV(arg))) == 0)
	ARETURN(bn);
    } while (0);
#endif
    /* Without GMP, this is where we wind up. Also where we wind up without
       a properly parseable number */
    arc_err_cstrfmt(c, "cannot coerce");
  }

  /* Generic type conversions */
  if (FIX2INT(AV(stype)) == T_NUM)
    ARETURN(coerce_num(c, AV(obj), AV(arg)));

  if (FIX2INT(AV(stype)) == T_INT) {
    value num;

    num = coerce_num(c, AV(obj), AV(arg));
    AFTCALL(arc_mkaff(c, arc_coerce, CNIL), num, ARC_BUILTIN(c, S_INT));
  }

  arc_err_cstrfmt(c, "cannot coerce");
  ARETURN(CNIL);
  AFEND;
}
AFFEND

typefn_t __arc_char_typefn__ = {
  __arc_null_marker,
  __arc_null_sweeper,
  NULL,
  char_hash,
  char_iscmp,
  NULL,
  NULL,
  char_xcoerce
};

typefn_t __arc_string_typefn__ = {
  __arc_null_marker,
  __arc_null_sweeper,
  NULL,
  string_hash,
  string_iscmp,
  NULL,
  string_apply,
  string_xcoerce
};
