/* 
  Copyright (C) 2013 Rafael R. Sevilla

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
#include <ctype.h>
#include "arcueid.h"
#include "builtins.h"
#include "utf.h"
#include "io.h"
#include "arith.h"
#include "hash.h"
#include "re.h"

static int scan(arc *c, value thr);
static int read_list(arc *c, value thr);
static int read_anonf(arc *c, value thr);
static int read_quote(arc *c, value thr);
static int read_qquote(arc *c, value thr);
static int read_comma(arc *c, value thr);
static int read_string(arc *c, value thr);
static int read_char(arc *c, value thr);
static int read_comment(arc *c, value thr);
static int read_symbol(arc *c, value thr);

static value get_lineno(arc *c, value lndata)
{
  value linenum;

  if (!BOUND_P(lndata))
    return(CUNBOUND);

  linenum = arc_hash_lookup(c, lndata, INT2FIX(-1));
  if (NIL_P(linenum) || !BOUND_P(linenum))
    linenum = INT2FIX(1);
  return(linenum);
}

static value set_lineno(arc *c, value lndata, value lineno)
{
  if (BOUND_P(lndata))
    arc_hash_insert(c, lndata, INT2FIX(-1), lineno);
  return(CNIL);
}

static value inc_lineno(arc *c, value lndata)
{
  value linenum;

  if (!BOUND_P(lndata))
    return(CUNBOUND);
  linenum = get_lineno(c, lndata);
  linenum = INT2FIX(FIX2INT(linenum) + 1);
  set_lineno(c, lndata, linenum);
  return(linenum);
}

value get_file(arc *c, value lndata)
{
  value file;

  if (!BOUND_P(lndata))
    return(CUNBOUND);
  file = arc_hash_lookup(c, lndata, INT2FIX(-2));
  if (!BOUND_P(file))
    file = CNIL;
  return(file);
}

static value set_file(arc *c, value lndata, value file)
{
  if (!BOUND_P(lndata))
    return(CUNBOUND);
  if (NIL_P(get_file(c, lndata)))
    return(arc_hash_insert(c, lndata, INT2FIX(-2), file));
  return(CNIL);
}

static value add_cons_lineno(arc *c, value lndata, value conscell,
			     value linenum)
{
  if (!BOUND_P(lndata))
    return(CNIL);
  arc_hash_insert(c, lndata, __arc_visitkey(conscell), linenum);
  return(conscell);
}

static value get_lineno2(arc *c, value lndata)
{
  value linenum;

  if (!BOUND_P(lndata))
    return(CUNBOUND);

  linenum = arc_hash_lookup(c, lndata, INT2FIX(-3));
  if (NIL_P(linenum) || !BOUND_P(linenum))
    linenum = INT2FIX(1);
  return(linenum);
}

static value set_lineno2(arc *c, value lndata, value lineno)
{
  if (BOUND_P(lndata))
    arc_hash_insert(c, lndata, INT2FIX(-3), lineno);
  return(CNIL);
}

value __arc_reset_lineno(arc *c, value lndata)
{
  return(set_lineno2(c, lndata, INT2FIX(1)));
}

/* Gets the closest file and line number to obj. If obj is not
   found in the lndata table, returns the last value it returned
   (which is probably close enough) */
value __arc_get_fileline(arc *c, value lndata, value obj)
{
  value lineno;

  if (!BOUND_P(lndata))
    return(CUNBOUND);
  lineno = get_lineno2(c, lndata);
  if (CONS_P(obj)) {
    value newln;

    newln = arc_hash_lookup(c, lndata, __arc_visitkey(obj));
    if (BOUND_P(newln)) {
      lineno = newln;
      set_lineno2(c, lndata, lineno);
    }
  }
  return(cons(c, get_file(c, lndata), lineno));
}

/* Is ch a valid character in a symbol? */
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

#define SCAN(fp, lndata, ch)			\
  AFCALL(arc_mkaff(c, scan, CNIL), fp, lndata);	\
  WV(ch, AFCRV)

#define READ(fp, eof, lndata, val)				\
  AFCALL(arc_mkaff(c, arc_sread, CNIL), fp, eof, lndata);	\
  WV(val, AFCRV)

#define READC(fp, val)					\
  AFCALL(arc_mkaff(c, arc_readc, CNIL), fp);	\
  WV(val, AFCRV);					\
  if (NIL_P(AV(val))) {					\
    arc_err_cstrfmt(c, "unexpected end of source");	\
    ARETURN(CNIL);					\
  }

#define READC2(fp, val, r)					\
  AFCALL(arc_mkaff(c, arc_readc, CNIL), fp);			\
  WV(val, AFCRV);						\
  r = (NIL_P(AV(val))) ? Runeerror : arc_char2rune(c, AV(val))	\

#define READ_COMMENT(fd, lndata) AFCALL(arc_mkaff(c, read_comment, CNIL), fd, CNIL, lndata)

/* Read up to the first non-symbol character from fp */
static AFFDEF(getsymbol)
{
  AARG(fp);
  AVAR(buf, ch);
  Rune r;
  AFBEGIN;
  WV(buf, arc_outstring(c, CNIL));
  for (;;) {
    READC2(AV(fp), ch, r);
    if (r == Runeerror)
      break;
    if (!issym(r)) {
      arc_ungetc_rune(c, r, AV(fp));
      break;
    }
    AFCALL(arc_mkaff(c, arc_writec, CNIL), AV(ch), AV(buf));
  }
  ARETURN(arc_inside(c, AV(buf)));
  AFEND;
}
AFFEND

AFFDEF(arc_sread)
{
  AARG(fp, eof);
  AOARG(lndata);
  AVAR(ch, func, linenum);
  Rune r;
  AFBEGIN;

  set_file(c, AV(lndata), arc_portname(c, AV(fp)));
  /* XXX - should put this in builtins somewhere? */
  for (;;) {
    SCAN(AV(fp), AV(lndata), ch);
    if (AV(ch) == CNIL)
      ARETURN(AV(eof));
    r = arc_char2rune(c, AV(ch));
    WV(linenum, get_lineno(c, AV(lndata)));
    /* cannot use switch here: interferes with the case statement implicitly
       created by AFBEGIN! */
    if (r == '(') {
      WV(func, arc_mkaff(c, read_list, CNIL));
    } else if (r == ')') {
      arc_err_cstrfmt(c, "misplaced right paren");
      ARETURN(CNIL);
    } else if (r == '[') {
      WV(func, arc_mkaff(c, read_anonf, CNIL));
    } else if (r == ']') {
      arc_err_cstrfmt(c, "misplaced right bracket");
      ARETURN(CNIL);
    } else if (r == '\'') {
      WV(func, arc_mkaff(c, read_quote, CNIL));
    } else if (r == '`') {
      WV(func, arc_mkaff(c, read_qquote, CNIL));
    } else if (r == ',') {
      WV(func, arc_mkaff(c, read_comma, CNIL));
    } else if (r == '"') {
      WV(func, arc_mkaff(c, read_string, CNIL));
    } else if (r == '#') {
      WV(func, arc_mkaff(c, read_char, CNIL));
    } else if (r == ';') {
      READ_COMMENT(AV(fp), AV(lndata));
      continue;
    } else {
      arc_ungetc_rune(c, r, AV(fp));
      WV(func, arc_mkaff(c, read_symbol, CNIL));
    }
    if (BOUND_P(AV(lndata))) {
      value result;

      AFCALL(AV(func), AV(fp), AV(eof), AV(lndata));
      result = AFCRV;
      if (TYPE(result) == T_CONS)
	add_cons_lineno(c, AV(lndata), result, AV(linenum));
      ARETURN(result);
    } else {
      AFTCALL(AV(func), AV(fp), AV(eof));
    }
  }
  AFEND;
}
AFFEND

/* Scan for the first non-blank character */
static AFFDEF(scan)
{
  AARG(fp, lndata);
  Rune r;
  AVAR(ch);
  AFBEGIN;
  for (;;) {
    AFCALL(arc_mkaff(c, arc_readc, CNIL), AV(fp));
    WV(ch, AFCRV);
    if (NIL_P(AV(ch)))
      ARETURN(CNIL);
    r = arc_char2rune(c, AV(ch));
    if (BOUND_P(AV(lndata)) && ucisnl(r))
      inc_lineno(c, AV(lndata));
    if (ucisspace(r))
      continue;
    break;
  }
  ARETURN(AV(ch));
  AFEND;
}
AFFEND

static AFFDEF(read_list)
{
  AARG(fp, eof);
  AOARG(lndata);
  AVAR(top, val, last, ch, indot);
  Rune r;
  AFBEGIN;

  WV(top, WV(val, WV(last, WV(indot, CNIL))));
  for (;;) {
    SCAN(AV(fp), AV(lndata), ch);
    if (AV(ch) == CNIL)
      ARETURN(AV(eof));
    r = arc_char2rune(c, AV(ch));
    if (r == ';') {
      READ_COMMENT(AV(fp), AV(lndata));
      continue;
    } else if (r == ')') {
      ARETURN(AV(top));
    }
    if (!NIL_P(AV(indot))) {
      arc_err_cstrfmt(c, "illegal use of .");
      ARETURN(CNIL);
    }
    arc_ungetc_rune(c, r, AV(fp));
    READ(AV(fp), AV(eof), AV(lndata), val);
    if (AV(val) == ARC_BUILTIN(c, S_DOT)) {
      READ(AV(fp), AV(eof), AV(lndata), val);
      if (!NIL_P(AV(last))) {
	scdr(AV(last), AV(val));
      } else {
	arc_err_cstrfmt(c, "illegal use of .");
	ARETURN(CNIL);
      }
      WV(indot, CTRUE);
      continue;
    }

    WV(val, cons(c, AV(val), CNIL));
    if (!NIL_P(AV(last))) {
      scdr(AV(last), AV(val));
    } else {
      WV(top, AV(val));
    }
    WV(last, AV(val));
  }
  AFEND;
}
AFFEND

/* Read an Arc square bracketed anonymous function.  This expands
   [ ... _ ... ] to (fn (_) ... _ ...) */
static AFFDEF(read_anonf)
{
  AARG(fp, eof);
  AOARG(lndata);
  AVAR(top, val, last, ch);
  Rune r;
  AFBEGIN;
  WV(top, WV(val, WV(last, CNIL)));
  for (;;) {
    SCAN(AV(fp), AV(lndata), ch);
    if (AV(ch) == CNIL)
      ARETURN(AV(eof));
    r = arc_char2rune(c, AV(ch));
    if (r == ';') {
      READ_COMMENT(AV(fp), AV(lndata));
      continue;
    } else if (r == ']') {
      /* Complete the fn */
      ARETURN(cons(c, ARC_BUILTIN(c, S_FN),
		   cons(c, cons(c, ARC_BUILTIN(c, S_US), CNIL),
			cons(c, AV(top), CNIL))));
    }
    arc_ungetc_rune(c, r, AV(fp));
    READ(AV(fp), AV(eof), AV(lndata), val);
    WV(val, cons(c, AV(val), CNIL));
    if (!NIL_P(AV(last)))
      scdr(AV(last), AV(val));
    else
      WV(top, AV(val));
    WV(last, AV(val));
  }
  arc_err_cstrfmt(c, "unexpected end of source");
  ARETURN(CNIL);
  AFEND;
}
AFFEND

/* Read a portion that happens to be quoted, and then enclose it in a
   form covered by the provided qsym. */
static AFFDEF(readq)
{
  AARG(fp, qsym, eof, lndata);
  AVAR(val);
  AFBEGIN;
  READ(AV(fp), AV(eof), AV(lndata), val);
  ARETURN(cons(c, AV(qsym), cons(c, AV(val), CNIL)));
  AFEND;
}
AFFEND

static AFFDEF(read_quote)
{
  AARG(fp, eof);
  AOARG(lndata);
  AFBEGIN;
  (void)lndata;
  AFTCALL(arc_mkaff(c, readq, CNIL), AV(fp), ARC_BUILTIN(c, S_QUOTE),
	  AV(eof), AV(lndata));
  AFEND;
}
AFFEND

static AFFDEF(read_qquote)
{
  AARG(fp, eof);
  AOARG(lndata);
  AFBEGIN;
  (void)lndata;
  AFTCALL(arc_mkaff(c, readq, CNIL), AV(fp), ARC_BUILTIN(c, S_QQUOTE),
	  AV(eof), AV(lndata));
  AFEND;
}
AFFEND

static AFFDEF(read_comma)
{
  AARG(fp, eof);
  AOARG(lndata);
  Rune r;
  AVAR(ch);
  AFBEGIN;
  (void)lndata;

  READC(AV(fp), ch)
  r = arc_char2rune(c, AV(ch));
  /* unquote-splicing */
  if (r == '@') {
    AFTCALL(arc_mkaff(c, readq, CNIL), AV(fp),
	    ARC_BUILTIN(c, S_UNQUOTESP), AV(eof), AV(lndata));
  }
  /* normal unquote. */
  arc_ungetc_rune(c, r, AV(fp));
  AFTCALL(arc_mkaff(c, readq, CNIL), AV(fp), ARC_BUILTIN(c, S_UNQUOTE),
	  AV(eof), AV(lndata));
  AFEND;
}
AFFEND

/* First unescaped @ in s, if any.  Escape by doubling. */
static int atpos(arc *c, value str, int i)
{
  int len;

  len = arc_strlen(c, str);
  while (i < len) {
    if (arc_strindex(c, str, i) == '@') {
      if (i+1 < len && arc_strindex(c, str, i+1) != '@')
	return(i);
      i++;
    }
    i++;
  }
  return(-1);
}

static value unescape_ats(arc *c, value s)
{
  value ns;
  int i, len;
  Rune ch;

  ns = arc_mkstringlen(c, 0);
  len = arc_strlen(c, s);
  for (i=0; i<len;) {
    ch = arc_strindex(c, s, i);
    if (i+1 < len && ch == '@' && arc_strindex(c, s, i+1) == '@')
      i++;
    ns = arc_strcatc(c, ns, ch);
    i++;
  }
  return(ns);
}

/* Break up a string s into a list of fragments delimited by the @'s. */
static AFFDEF(codestring)
{
  AARG(s, lndata);
  int i, len, rlen;
  AVAR(ss, rest, in, expr);
  AFBEGIN;
  /* Find the position of the first @ sign in the string s. If there is
     no @ sign, just return a list with just that string. */
  i = atpos(c, AV(s), 0);
  if (i < 0)
    ARETURN(cons(c, AV(s), CNIL));
  /* If we have an @ sign somewhere, break the string up at that point,
     into the first half (ss) and the rest (rest).  Read the rest portion
     as though it were not part of the string. */
  len = arc_strlen(c, AV(s));
  WV(ss, arc_substr(c, AV(s), 0, i));
  WV(rest, arc_substr(c, AV(s), i+1, len));
  WV(in, arc_instring(c, AV(rest), CNIL));
  READ(AV(in), CNIL, AV(lndata), expr);

  /* Get the current position after reading. Break the string up
     again at that point, and pass the remainder of the string back
     to codestring recursively. */
  AFCALL(arc_mkaff(c, arc_tell, CNIL), AV(in));
  i = FIX2INT(AFCRV);
  rlen = arc_strlen(c, AV(rest));
  AFCALL(arc_mkaff(c, codestring, CNIL),
	 arc_substr(c, AV(rest), i, rlen), AV(lndata));
  /* We then combine the pre-@-sign portion of the string (ss),
     the post-@-sign portion (that became sexpr after reading), and the
     results of the recursive call to codestring on the portion of the
     string not consumed by the reader as it was called on the other
     part of the string after the @ sign.  XXX - this ought to be
     converted into tail recursive form. */
  ARETURN(cons(c, AV(ss), cons(c, AV(expr), AFCRV)));
  AFEND;
}
AFFEND

static AFFDEF(read_atstring)
{
  AARG(s, lndata);
  AVAR(cs, p);
  AFBEGIN;
  if (atpos(c, AV(s), 0) >= 0) {
    AFCALL(arc_mkaff(c, codestring, CNIL), AV(s), AV(lndata));
    WV(cs, AFCRV);
    for (WV(p, AV(cs)); AV(p); WV(p, cdr(AV(p)))) {
      if (TYPE(car(AV(p))) == T_STRING)
	scar(AV(p), unescape_ats(c, car(AV(p))));
    }
    ARETURN(cons(c, ARC_BUILTIN(c, S_STRING), AV(cs)));
  }
  ARETURN(unescape_ats(c, AV(s)));
  AFEND;
}
AFFEND

AFFDEF(read_string)
{
  AARG(fp, eof);
  AOARG(lndata);
  Rune r;
  int digval;
  AVAR(ch, state, buf, escrune, digcount);
  AFBEGIN;
  WV(state, INT2FIX(1));
  WV(buf, arc_outstring(c, CNIL));

  ((void)eof);
  for (;;) {
    READC(AV(fp), ch);
    r = arc_char2rune(c, AV(ch));
    if (AV(state) == INT2FIX(1)) {
      /* State 1: normal reading */
      if (r == '\"') {
	/* end of string */
	if (arc_declared(c, ARC_BUILTIN(c, S_ATSTRINGS))) {
	  AFTCALL(arc_mkaff(c, read_atstring, CNIL),
		  arc_inside(c, AV(buf)), AV(lndata));
	}
	ARETURN(arc_inside(c, AV(buf)));
      }

      if (r == '\\') {
	/* escape character */
	WV(state, INT2FIX(2));
	continue;
      }

      if (ucisnl(r)) {
	inc_lineno(c, AV(lndata));
      }

      /* Otherwise, just add the character to our string buffer */
      AFCALL(arc_mkaff(c, arc_writec, CNIL), arc_mkchar(c, r), AV(buf));
      continue;
    }

    if (AV(state) == INT2FIX(2)) {
      /* Escape code */
      if (r ==  '\'' || r ==  '\"' || r == '\\') {
	;			/* character as is */
      } else if (r == '0') {
	r = 0x0000;
      } else if (r == 'a') {
	r = 0x0007;
      } else if (r == 'b') {
	r = 0x0008;
      } else if (r == 't') {
	r = 0x0009;
      } else if (r == 'n') {
	r = 0x000a;
      } else if (r == 'v') {
	r = 0x000b;
      } else if (r == 'f') {
	r = 0x000c;
      } else if (r == 'r') {
	r = 0x000d;
      } else if (r == 'U' || r == 'u') {
	/* unicode escape */
	WV(escrune, WV(digcount, INT2FIX(0)));
	WV(state, INT2FIX(3));
	continue;
      } else {
	arc_err_cstrfmt(c, "unknown escape code");
	ARETURN(CNIL);
      }
      AFCALL(arc_mkaff(c, arc_writec, CNIL), arc_mkchar(c, r), AV(buf));
      WV(state, INT2FIX(1));
      continue;
    }

    if (AV(state) == INT2FIX(3)) {
      /* Unicode escape */
      if (FIX2INT(AV(digcount)) >= 5) {
	arc_ungetc_rune(c, r, AV(fp));
	AFCALL(arc_mkaff(c, arc_writec, CNIL),
	       arc_mkchar(c, FIX2INT(AV(escrune))),
	       AV(buf));
	WV(state, INT2FIX(1));
	continue;
      }

      if (r >= '0' && r <= '9')
	digval = r - '0';
      else if (r >= 'A' && r <= 'F')
	digval = r - 'A' + 10;
      else if (r >= 'a' && r <= 'f')
	digval = r - 'a' + 10;
      else {
	arc_err_cstrfmt(c, "invalid character in Unicode escape");
	ARETURN(CNIL);
      }
      WV(escrune, INT2FIX(FIX2INT(AV(escrune)) * 16 + digval));
      WV(digcount, INT2FIX(FIX2INT(AV(digcount)) + 1));
      continue;
    }

    /* should never get here! */
    arc_err_cstrfmt(c, "internal error sread string invalid state");
    ARETURN(CNIL);
  }
  AFEND;
}
AFFEND

/* These character constants are inherited by Arc from MzScheme/Racket.
   Frankly, I think they're stupid, and one would be better off using
   the same character escape sequences as for strings.  But well,
   we have to live with these types of complications for the sake of
   compatibility--maybe later on we can add a declaration (like atstrings)
   that modifies this reader behavior to something more rational (such as
   sharp followed by the actual character, with slash for escapes).  Arc
   does not otherwise use the #-sign for anything else. */
static AFFDEF(read_char)
{
  AARG(fp, eof);
  AOARG(lndata);
  AVAR(ch);
  int alldigits, val, i, digit;
  Rune r;
  value tok, symch;
  AFBEGIN;
  (void)lndata;
  (void)eof;
  READC(AV(fp), ch);
  r = arc_char2rune(c, AV(ch));
  if (r != '\\') {
    arc_err_cstrfmt(c, "invalid character constant");
    ARETURN(CNIL);
  }

  /* Special case for any special characters that are not valid symbols. */
  READC(AV(fp), ch);
  r = arc_char2rune(c, AV(ch));
  if (!issym(r)) {
    ARETURN(arc_mkchar(c, r));
  }
  arc_ungetc_rune(c, r, AV(fp));
  AFCALL(arc_mkaff(c, getsymbol, CNIL), AV(fp));
  tok = AFCRV;
  /* no AFCALLs after this point? */
  if (arc_strlen(c, tok) == 1)	/* single character */
    ARETURN(arc_mkchar(c, arc_strindex(c, tok, 0)));
  if (arc_strlen(c, tok) == 3) {
    /* Possible octal escape */
    alldigits = 1;
    val = 0;
    for (i=0; i<3; i++) {
      digit = arc_strindex(c, tok, i);
      if (!isdigit(digit)) {
	alldigits = 0;
	break;
      }
      val = val * 8 + (digit - '0');
    }
    if (alldigits)
      ARETURN(arc_mkchar(c, val));
    /* Possible hexadecimal escape */
    if (arc_strindex(c, tok, 0) == 'x') {
      alldigits = 1;
      val = 0;
      for (i=1; i<3; i++) {
	digit = arc_strindex(c, tok, i);
	if (!isxdigit(digit)) {
	  alldigits = 0;
	  break;
	}
	digit = tolower(digit);
	digit = (digit >= '0' && digit <= '9') ?
	  (digit - '0') : (digit - 'a' + 10);
	val = val * 16 + digit;
      }
      if (alldigits)
	ARETURN(arc_mkchar(c, val));
    }
    /* Not an octal or hexadecimal escape */
  }

  /* Possible Unicode escape? */
  if (tolower(arc_strindex(c, tok, 0)) == 'u') {
    alldigits = 1;
    val = 0;
    for (i=1; i<arc_strlen(c, tok); i++) {
      digit = arc_strindex(c, tok, i);
      if (!isxdigit(digit)) {
	alldigits = 0;
	break;
      }
      digit = tolower(digit);
      digit = (digit >= '0' && digit <= '9') ? (digit - '0') : (digit - 'a' + 10);
      val = val * 16 + digit;
    }
    if (alldigits)
      ARETURN(arc_mkchar(c, val));
  }

  symch = arc_hash_lookup(c, VINDEX(c->builtins, BI_charesc), tok);
  if (symch == CUNBOUND) {
    arc_err_cstrfmt(c, "invalid character constant");
    ARETURN(CNIL);
  }
  ARETURN(symch);
  AFEND;
}
AFFEND

/* Just basically keep reading until we reach end of line or end of file */
static AFFDEF(read_comment)
{
  AARG(fp, eof);
  AOARG(lndata);
  AVAR(ch);
  Rune r;
  AFBEGIN;
  ((void)eof);
  for (;;) {
    READC2(AV(fp), ch, r);
    if (r == Runeerror)
      break;
    if (ucisnl(r)) {
      inc_lineno(c, AV(lndata));
      break;
    }
  }
  ARETURN(CNIL);
  AFEND;
}
AFFEND

/* See if the "symbol" can be parsed as a regex. A regex
   must begin with the / character, and must end either with
   a /, /m, /i, /im, or /mi. Returns the regex if this is
   true, or CNIL if the string could not be parsed as a regular
   expression. */
static value arc_string2regex(arc *c, value sym)
{
  Rune ch;
  int len, multiline, casefold, endch, i;
  unsigned int flags;
  value rxstr;

  len = arc_strlen(c, sym);
  if (len < 2)
    return(CNIL);	/* a regex must be at least 2 chars */

  if (arc_strindex(c, sym, 0) != '/')
    return(CNIL);	/* does not begin with a slash */

  endch = len-1;
  multiline = casefold = 0;
  for (i=0; i<2; i++) {
    ch = arc_strindex(c, sym, endch);
    if (ch == '/') {
      break;
    } else if (ch == 'm' && !multiline) {
      multiline = 1;
      endch--;
    } else if (ch == 'i' && !casefold) {
      casefold = 1;
      endch--;
    } else {
      return(CNIL);
    }
  }
  /* If we get here, endch must be a slash. The regular expression
     will be the portion of sym from 1 up to endch-1, so the length
     of the regular expression will be endch-1 */
  rxstr = arc_mkstringlen(c, endch-1);
  for (i=0; i<endch-1; i++)
    arc_strsetindex(c, rxstr, i, arc_strindex(c, sym, i+1));
  flags = 0;
  if (multiline)
    flags |= REGEXP_MULTILINE;
  if (casefold)
    flags |= REGEXP_CASEFOLD;
  return(arc_mkregexp(c, rxstr, flags));
}

/* parse a symbol name or number */
static AFFDEF(read_symbol)
{
  AARG(fp, eof);
  AOARG(lndata);
  AVAR(sym);
  value num, rx;
  AFBEGIN;
  (void)lndata;
  (void)eof;
  AFCALL(arc_mkaff(c, getsymbol, CNIL), AV(fp));
  WV(sym, AFCRV);
  if (arc_strcmp(c, AV(sym), arc_mkstringc(c, ".")) == 0)
    ARETURN(ARC_BUILTIN(c, S_DOT));
  /* Try to convert the symbol to a number */
  num = arc_string2num(c, AV(sym), 0, 0);
  if (!NIL_P(num))
    ARETURN(num);
  rx = arc_string2regex(c, AV(sym));
  if (!NIL_P(rx))
    ARETURN(rx);
  /* If that doesn't work, well, just intern the symbol and toss it
     out there. */
  ARETURN(arc_intern(c, AV(sym)));  
  AFEND;
}
AFFEND
