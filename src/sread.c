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

#define SCAN(fn, ch, fp)			\
  fn = arc_mkaff(c, scan, CNIL);		\
  AFCALL(fn, fp);				\
  ch = AFCRV;

#define READ(fn, fp, eof, val)			\
  fn = arc_mkaff(c, arc_sread, CNIL);		\
  AFCALL(fn, fp, eof);				\
  val = AFCRV;

#define READ_COMMENT(func, fd)			\
  func = arc_mkaff(c, read_comment, CNIL);	\
  AFCALL(func, fd);


AFFDEF(arc_sread, fp, eof)
{
  Rune r;
  AVAR(ch, func);
  AFBEGIN;
  /* XXX - should put this in builtins somewhere? */
  for (;;) {
    SCAN(AV(func), AV(ch), AV(fp));
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
      READ_COMMENT(AV(func), AV(fd));
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
AFFEND;

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
    SCAN(AV(func), AV(ch), AV(fp));
    if (AV(ch) == CNIL)
      ARETURN(AV(eof));
    r = arc_char2rune(c, AV(ch));
    if (r == ';') {
      READ_COMMENT(AV(fn), AV(fd));
      continue;
    } else if (r == ')') {
      ARETURN(AV(top));
    }
    if (!NIL_P(AV(indot))) {
      arc_err_cstrfmt(c, "illegal use of .");
      return(CNIL);
    }
    arc_ungetc_rune(c, r, AV(fd));
    READ(AV(fn), AV(fp), AV(eof), AV(val));
    if (AV(val) == ARC_BUILTIN(c, S_DOT)) {
      READ(AV(fn), AV(fp), AV(eof), AV(val));
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
AFFEND;

/* Read an Arc square bracketed anonymous function.  This expands
   [ ... _ ... ] to (fn (_) ... _ ...) */
static AFFDEF(read_anonf, fp, eof)
{
  AVAL(top, val ,last);
  Rune ch;
  AFBEGIN;
  AV(top) = AV(val) = AV(last) = CNIL;
  for (;;) {
    SCAN(AV(func), AV(ch), AV(fp));
    if (AV(ch) == CNIL)
      ARETURN(AV(eof));
    r = arc_char2rune(c, AV(ch));
    if (r == ';') {
      READ_COMMENT(AV(fn), AV(fd));
      continue;
    } else if (r == ']') {
      /* Complete the fn */
      ARETURN(cons(c, ARC_BUILTIN(c, S_FN),
		   cons(c, cons(c, ARC_BUILTIN(c, S_US), CNIL),
			cons(c, AV(top), CNIL))));
    }
    arc_ungetc_rune(c, r, AV(fd));
    READ(AV(fn), AV(fp), AV(eof), AV(val));
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
AFFEND;
