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
#include "arcueid.h"
#include "builtins.h"

static int scan(arc *c, value thr);
static int read_list(arc *c, value thr);
static int read_anonf(arc *c, value thr);
static int read_quote(arc *c, value thr);
static int read_qquote(arc *c, value thr);
static int read_comma(arc *c, value thr);
static int read_string(arc *c, value thr);
static int read_char(arc *c, value thr);
static int read_comment(arc *c, value thr)
static int read_symbol(arc *c, value thr);

#define SCAN(fp, ch)				\
  AFCALL(arc_mkaff(c, scan, CNIL), fp);		\
  ch = AFCRV

#define READ(fp, eof, val)				\
  AFCALL(arc_mkaff(c, arc_sread, CNIL), fp, eof);	\
  val = AFCRV

#define READ_COMMENT(func, fd) AFCALL(arc_mkaff(c, read_comment, CNIL), fd)


AFFDEF(arc_sread, fp, eof)
{
  Rune r;
  AVAR(ch, func);
  AFBEGIN;
  /* XXX - should put this in builtins somewhere? */
  for (;;) {
    SCAN(AV(fp), AV(ch));
    if (AV(ch) == CNIL)
      ARETURN(AV(eof));
    r = arc_char2rune(c, AV(ch));
    /* cannot use switch here: interferes with the case statement implicitly
       created by AFBEGIN! */
    if (r == '(') {
      AV(func) = arc_mkaff(c, read_list, CNIL);
    } else if (r == ')') {
      arc_err_cstrfmt(c, "misplaced right paren");
      ARETURN(CNIL);
    } else if (r == '[') {
      AV(func) = arc_mkaff(c, read_anonf, CNIL);
    } else if (r == ']') {
      arc_err_cstrfmt(c, "misplaced right bracket");
      ARETURN(CNIL);
    } else if (r == '\'') {
      AV(func) = arc_mkaff(c, read_quote, CNIL);
    } else if (r == '`') {
      AV(func) = arc_mkaff(c, read_qquote, CNIL);
    } else if (r == ',') {
      AV(func) = arc_mkaff(c, read_comma, CNIL);
    } else if (r == '"') {
      AV(func) = arc_mkaff(c, read_string, CNIL);
    } else if (r == '#') {
      AV(func) = arc_mkaff(c, read_char, CNIL);
    } else if (r == ';') {
      READ_COMMENT(AV(fd));
      continue;
    } else {
      arc_ungetc_rune(c, r, AV(fd));
      AV(func) = arc_mkaff(c, read_symbol, CNIL);
    }
    AFCALL(AV(func), AV(fd), AV(eof));
    ARETURN(AFCRV);
  }
  AFEND;
}
AFFEND

/* Scan for the first non-blank character */
static AFFDEF(scan, fp)
{
  Rune r;
  AVAR(ch, readc);  
  AFBEGIN;
  AV(readc) = arc_mkaff(c, arc_readc, CNIL);
  for (;;) {
    AFCALL(AV(readc), AV(fp));
    AV(ch) = AFCRV;
    if (NIL_P(AV(ch)))
      return(CNIL);
    r = arc_char2rune(c, AV(ch));
    if (ucisspace(r))
      continue;
    break;
  }
  AFEND;
}
AFFEND

static AFFDEF(read_list, fp, eof)
{
  AVAR(top, val, last, fn, ch, indot);
  AFBEGIN;

  AV(top) = AV(val) = AV(last) = AV(indot) = CNIL;
  for (;;) {
    SCAN(AV(fp), AV(ch));
    if (AV(ch) == CNIL)
      ARETURN(AV(eof));
    r = arc_char2rune(c, AV(ch));
    if (r == ';') {
      READ_COMMENT(AV(fd));
      continue;
    } else if (r == ')') {
      ARETURN(AV(top));
    }
    if (!NIL_P(AV(indot))) {
      arc_err_cstrfmt(c, "illegal use of .");
      return(CNIL);
    }
    arc_ungetc_rune(c, r, AV(fd));
    READ(AV(fp), AV(eof), AV(val));
    if (AV(val) == ARC_BUILTIN(c, S_DOT)) {
      READ(AV(fp), AV(eof), AV(val));
      if (!NIL_P(AV(last))) {
	scdr(AV(last), AV(val));
      } else {
	arc_err_cstrfmt(c, "illegal use of .");
	return(CNIL);
      }
      AV(indot) = CTRUE;
      continue;
    }

    AV(val) = cons(c, AV(val), CNIL);
    if (!NIL_P(AV(last))) {
      scdr(AV(last), AV(val));
    } else {
      AV(top) = AV(val);
    }
    AV(last) = AV(val);
  }
  AFEND;
}
AFFEND

/* Read an Arc square bracketed anonymous function.  This expands
   [ ... _ ... ] to (fn (_) ... _ ...) */
static AFFDEF(read_anonf, fp, eof)
{
  AVAR(top, val ,last);
  Rune ch;
  AFBEGIN;
  AV(top) = AV(val) = AV(last) = CNIL;
  for (;;) {
    SCAN(AV(fp), AV(ch));
    if (AV(ch) == CNIL)
      ARETURN(AV(eof));
    r = arc_char2rune(c, AV(ch));
    if (r == ';') {
      READ_COMMENT(AV(fd));
      continue;
    } else if (r == ']') {
      /* Complete the fn */
      ARETURN(cons(c, ARC_BUILTIN(c, S_FN),
		   cons(c, cons(c, ARC_BUILTIN(c, S_US), CNIL),
			cons(c, AV(top), CNIL))));
    }
    arc_ungetc_rune(c, r, AV(fd));
    READ(AV(fp), AV(eof), AV(val));
    AV(val) = cons(c, AV(val), CNIL);
    if (!NIL_P(AV(last)))
      scdr(AV(last), AV(val));
    else
      AV(top) = AV(val);
    AV(last) = AV(val);
  }
  arc_err_cstrfmt(c, "unexpected end of source");
  ARETURN(CNIL);
  AFEND;
}
AFFEND

/* Read a portion that happens to be quoted, and then enclose it in a
   form covered by the provided qsym. */
static AFFDEF(readq, fp, qsym, eof)
{
  AVAR(val, fn);
  AFBEGIN;
  READ(AV(fp), AV(eof), AV(val));
  ARETURN(cons(c, AV(qsym), cons(c, AV(val), CNIL)));
  AFEND;
}
AFFEND

static AFFDEF(read_quote, fp, eof)
{
  ABEGIN;
  AFCALL(arc_mkaff(c, readq, CNIL), fp, ARC_BUILTIN(c, S_QUOTE), eof);
  ARETURN(AFCRV);
  AEND;
}
AFFEND

static AFFDEF(read_qquote, fp, eof)
{
  ABEGIN;
  AFCALL(arc_mkaff(c, readq, CNIL), fp, ARC_BUILTIN(c, S_QQUOTE), eof);
  ARETURN(AFCRV);
  AEND;
}
AFFEND

static AFFDEF(read_comma, fp, eof)
{
  Rune r;
  AVAR(ch, readc);  
  AFBEGIN;

  AV(readc) = arc_mkaff(c, arc_readc, CNIL);
  AFCALL(AV(readc), AV(fp));
  AV(ch) = AFCRV;
  if (AV(ch) == CNIL) {
    arc_err_cstrfmt(c, "unexpected end of source");
    ARETURN(CNIL);
  }
  r = arc_char2rune(c, AV(ch));
  /* unquote-splicing */
  if (r == '@') {
    AFCALL(arc_mkaff(c, readq, CNIL), fp, ARC_BUILTIN(c, S_UNQUOTESP), eof);
    ARETURN(AFCRV);
  }
  /* normal unquote. */
  arc_ungetc_rune(c, r, AV(fd));
  AFCALL(arc_mkaff(c, readq, CNIL), fp, ARC_BUILTIN(c, S_UNQUOTE), eof);
  ARETURN(AFCRV);
  AEND;
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
static AFFDEF(codestring, s)
{
  int i, len, rlen;
  AVAR(ss, rest, in, expr);
  AFBEGIN;
  /* Find the position of the first @ sign in the string s. If there is
     no @ sign, just return a list with just that string. */
  i = atpos(c, AV(s), 0);
  if (i < 0)
    return(cons(c, AV(s), CNIL));
  /* If we have an @ sign somewhere, break the string up at that point,
     into the first half (ss) and the rest (rest).  Read the rest portion
     as though it were not part of the string. */
  len = arc_strlen(c, AV(s));
  AV(ss) = arc_substr(c, AV(s), 0, i);
  AV(rest) = arc_substr(c, AV(s), i+1, len);
  AV(in) = arc_instring(c, AV(rest), CNIL);
  READ(AV(in), CNIL, AV(expr));

  /* Get the current position after reading. Break the string up
     again at that point, and pass the remainder of the string back
     to codestring recursively. */
  i = arc_tell(c, AV(in));
  AV(rlen) = arc_strlen(c, AV(rest));
  AFCALL(arc_mkaff(c, codestring, CNIL),
	 arc_substr(c, AV(rest), i, AV(rlen)));
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

static AFFDEF(read_atstring, s)
{
  AVAR(cs, p);
  AFBEGIN;
  if (atpos(c, AV(s), 0) >= 0) {
    AFCALL(arc_mkaff(c, codestring, CNIL), AV(s));
    for (AV(p)=AV(cs); AV(p); AV(p) = cdr(AV(p))) {
      if (TYPE(car(AV(p))) == T_STRING)
	scar(AV(p), unescape_ats(c, car(AV(p))));
    }
    ARETURN(cons(c, ARC_BUILTIN(c, S_STRING), AV(cs)));
  }
  ARETURN(unescape_ats(c, AV(s)));
  AFEND;
}
AFFEND

AFFDEF(read_string, fp, eof)
{
  Rune r, escrune;
  int digcount;
  AVAR(ch, readc, state, buf);
  AFBEGIN;
  AV(readc) = arc_mkaff(c, arc_readc, CNIL);
  AV(state) = INT2FIX(1);
  AV(buf) = arc_outstring(c, CNIL);

  for (;;) {
    AFCALL(AV(readc), AV(fp));
    AV(ch) = AFCRV;
    if (NIL_P(AV(ch))) {
      arc_err_cstrfmt(c, "unterminated string reaches end of input");
      ARETURN(CNIL);
    }
    r = arc_char2rune(c, AV(ch));
    if (AV(state) == INT2FIX(1)) {
      /* State 1: normal reading */
      if (r == '\"') {
	/* end of string */
	if (c->atstrings) {
	  AFCALL(mkaff(c, read_atstring, CNIL), arc_inside(c, AV(buf)));
	  ARETURN(AFCRV);
	}
	ARETURN(arc_inside(c, AV(buf)));
      }

      if (r == '\\') {
	/* escape character */
	AV(state) = INT2FIX(2);
	continue;
      }

      /* Otherwise, just add the character to our string buffer */
      AFCALL(mkaff(c, arc_writec, CNIL), arc_mkchar(c, r), buf);
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
	break;
      } else if (r == 'U' || r == 'u') {
	/* unicode escape */
	escrune = digcount = INT2FIX(0);
	AV(state) = INT2FIX(3);
	continue;
      } else {
	arc_err_cstrfmt(c, "unknown escape code");
	ARETURN(CNIL);
      }
      AFCALL(mkaff(c, arc_writec, CNIL), arc_mkchar(c, r), buf);
      continue;
    }

    if (AV(state) == INT2FIX(3)) {
      /* Unicode escape */
      if (digcount >= 5) {
	arc_ungetc_rune(c, r, AV(fd));
	AFCALL(mkaff(c, arc_writec, CNIL), arc_mkchar(c, escrune), buf);
	AV(state) = INT2FIX(1);
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
      escrune = escrune * 16 + digval;
      digcount++;
      continue;
    }

    /* should never get here! */
    arc_err_cstrfmt(c, "internal error sread string invalid state");
    ARETURN(CNIL);
  }
  AFEND;
}
AFFEND
