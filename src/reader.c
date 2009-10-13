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

static Rune scan(carc *c, value src, int *index);
static value read_list(carc *c, value src, int *index);
static value read_anonf(carc *c, value src, int *index);
static value read_quote(carc *c, value src, int *index, value sym);
static value read_comma(carc *c, value src, int *index);
static value read_string(carc *c, value src, int *index);
static value read_char(carc *c, value src, int *index);
static void read_comment(carc *c, value src, int *index);
static value read_symbol(carc *c, value src, int *index);

#define ID2SYM(x) ((value)(((long)(x))<<8|SYMBOL_FLAG))

value carc_intern(carc *c, value name)
{
  value symval;

  if ((symval = carc_hash_lookup(c, c->symtable, name)) != CNIL)
    return(symval);

  symval = ID2SYM(++c->lastsym);
  carc_hash_insert(c, c->symtable, name, symval);
  carc_hash_insert(c, c->rsymtable, symval, name);
  return(symval);
}

value carc_sym2name(carc *c, value sym)
{
  return(carc_hash_lookup(c, c->rsymtable, sym));
}

/* Reader */
static Rune readchar(struct carc *c, value src, int *index)
{
  switch (TYPE(src)) {
  case T_STRING:
    return(carc_strgetc(c, src, index));
    break;
  default:
    c->signal_error(c, "Attempt to read from an invalid source");
    break;
  }
  return(CNIL);
}

static void unreadchar(struct carc *c, value src, Rune ch, int *index)
{
  switch (TYPE(src)) {
  case T_STRING:
    return(carc_strungetc(c, index));
    break;
  default:
    c->signal_error(c, "Attempt to unread from an invalid source");
    break;
  }
}

value carc_read(carc *c, value src, int *index, value *pval)
{
  Rune ch;

  while ((ch = scan(c, src, index)) != Runeerror) {
    switch (ch) {
    case '(':
      *pval = read_list(c, src, index);
      return(CTRUE);
    case ')':
      c->signal_error(c, "misplaced right paren");
      return(CNIL);
    case '[':
      *pval = read_anonf(c, src, index);
      return(CTRUE);
    case ']':
      c->signal_error(c, "misplaced right bracket");
      return(CNIL);
    case '\'':
      *pval = read_quote(c, src, index, c->quote);
      return(CTRUE);
    case '`':
      *pval = read_quote(c, src, index, c->qquote);
      return(CTRUE);
    case ',':
      *pval = read_comma(c, src, index);
      return(CTRUE);
    case '"':
      *pval = read_string(c, src, index);
      return(CTRUE);
    case '#':
      *pval = read_char(c, src, index);
      return(CTRUE);
    case ';':
      read_comment(c, src, index);
      return(CTRUE);
    default:
      unreadchar(c, src, ch, index);
      *pval = read_symbol(c, src, index);
      return(CTRUE);
    }
  }
  return(CNIL);
}

static value read_list(carc *c, value src, int *index)
{
  value top, val, last;
  Rune ch;

  top = val = last = CNIL;
  while ((ch = scan(c, src, index)) != Runeerror) {
    switch (ch) {
    case ';':
      read_comment(c, src, index);
      break;
    case ')':
      return(top);
    default:
      unreadchar(c, src, ch, index);
      if (!carc_read(c, src, index, &val))
	c->signal_error(c, "unexpected end of source");
      val = cons(c, val, CNIL);
      if (last)
	scdr(last, val);
      else
	top = val;
      last = val;
      break;
    }
  }
  c->signal_error(c, "unexpected end of source");
  return(CNIL);
}

static void read_comment(carc *c, value src, int *index)
{
  Rune ch;

  while ((ch = readchar(c, src, index)) != Runeerror && !ucisnl(ch))
    ;
  if (ch != Runeerror)
    unreadchar(c, src, ch, index);
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

static value getsymbol(carc *c, value src, int *index)
{
  Rune buf[STRMAX];
  Rune ch;
  int i;
  value sym, nstr;

  sym = CNIL;
  i=0;
  while ((ch = readchar(c, src, index)) != Runeerror && issym(ch)) {
    if (i >= STRMAX) {
      nstr = carc_mkstring(c, buf, i);
      sym = (sym == CNIL) ? nstr : carc_strcat(c, sym, nstr);
      i = 0;
    }
    buf[i++] = ch;
  }
  if (i==0 && sym == CNIL)
    return(CNIL);
  nstr = carc_mkstring(c, buf, i);
  sym = (sym == CNIL) ? nstr : carc_strcat(c, sym, nstr);

  unreadchar(c, src, ch, index);
  return(sym);
}

/* parse a symbol name or number */
static value read_symbol(carc *c, value str, int *index)
{
  value sym, num;

  if ((sym = getsymbol(c, str, index)) == CNIL)
    c->signal_error(c, "expecting symbol name");
  num = carc_string2num(c, sym);
  return((num == CNIL) ? carc_intern(c, sym) : num);
}

/* scan for first non-blank character */
static Rune scan(carc *c, value src, int *index)
{
  Rune ch;

  while ((ch = readchar(c, src, index)) != Runeerror && ucisspace(ch))
    ;
  return(ch);
}

/* Read an Arc square bracketed anonymous function.  This expands to
   (fn (_) ...) */
static value read_anonf(carc *c, value src, int *index)
{
  value top, val, last, ret;
  Rune ch;

  top = val = last = CNIL;
  while ((ch = scan(c, src, index)) != Runeerror) {
    switch (ch) {
    case ';':
      read_comment(c, src, index);
      break;
    case ']':
      ret = cons(c, c->fn, cons(c, cons(c, c->us, CNIL), cons(c, top, CNIL)));

      return(ret);
    default:
      unreadchar(c, src, ch, index);
      if (!carc_read(c, src, index, &val))
	c->signal_error(c, "unexpected end of source");
      val = cons(c, val, CNIL);
      if (last)
	scdr(last, val);
      else
	top = val;
      last = val;
      break;
    }
  }
  c->signal_error(c, "unexpected end of source");
  return(CNIL);

}

static value read_quote(carc *c, value src, int *index, value sym)
{
  value val;

  if (carc_read(c, src, index, &val) == CNIL)
    c->signal_error(c, "unexpected end of source");
  return(cons(c, sym, cons(c, val, CNIL)));
}

static value read_comma(carc *c, value src, int *index)
{
  Rune ch;

  if ((ch = readchar(c, src, index)) == '@')
    return(read_quote(c, src, index, c->unquotesp));
  unreadchar(c, src, ch, index);
  return(read_quote(c, src, index, c->unquote));
}

/* XXX - we need to add support for octal and hexadecimal escapes as well */
static value read_string(carc *c, value src, int *index)
{
  Rune buf[STRMAX], ch, escrune;
  int i=0, state=1, digval, digcount;
  value nstr, str = CNIL;

  while ((ch = readchar(c, src, index)) != Runeerror) {
    switch (state) {
    case 1:
      switch (ch) {
      case '\"':
	/* end of string */
	nstr = carc_mkstring(c, buf, i);
	str = (str == CNIL) ? nstr : carc_strcat(c, str, nstr);
	return(str);		/* proper termination */
	break;
      case '\\':
	/* escape character */
	state = 2;
	break;
      default:
	if (i >= STRMAX) {
	  nstr = carc_mkstring(c, buf, i);
	  str = (str == CNIL) ? nstr : carc_strcat(c, str, nstr);
	  i = 0;
	}
	buf[i++] = ch;
	break;
      }
      break;
    case 2:
      /* escape code */
      switch (ch) {
      case '\'':
      case '\"':
      case '\\':
	/* ch is as is */
	break;
      case '0':
	ch = 0x0000;
	break;
      case 'a':
	ch = 0x0007;
	break;
      case 'b':
	ch = 0x0008;
	break;
      case 't':
	ch = 0x0009;
	break;
      case 'n':
	ch = 0x000a;
	break;
      case 'v':
	ch = 0x000b;
	break;
      case 'f':
	ch = 0x000c;
	break;
      case 'r':
	ch = 0x000d;
	break;
      case 'U':
      case 'u':
	escrune = 0;
        digcount = 0;
	state = 3;
        continue;
      default:
	c->signal_error(c, "unknown escape code");
	break;
      }
      if (i >= STRMAX) {
	nstr = carc_mkstring(c, buf, i);
	str = (str == CNIL) ? nstr : carc_strcat(c, str, nstr);
	i = 0;
      }
      buf[i++] = ch;
      state = 1;
      break;
    case 3:
      /* Unicode escape */
      if (digcount >= 5) {
	unreadchar(c, src, ch, index);
	if (i >= STRMAX) {
	  nstr = carc_mkstring(c, buf, i);
	  str = (str == CNIL) ? nstr : carc_strcat(c, str, nstr);
	  i = 0;
	}
	buf[i++] = escrune;
	state = 1;
      } else {
	if (ch >= '0' && ch <= '9')
	  digval = ch - '0';
	else if (ch >= 'A' && ch <= 'F')
	  digval = ch - 'A' + 10;
	else if (ch >= 'a' && ch <= 'f')
	  digval = ch - 'a' + 10;
	else
	  c->signal_error(c, "invalid character in Unicode escape");
	escrune = escrune * 16 + digval;
	digcount++;
      }
      break;
    }
  }
  c->signal_error(c, "unterminated string reaches end of input");
  return(CNIL);			/* to pacify -Wall */
}

/* These character constants are inherited by Arc from MzScheme.
   Frankly, I think they're stupid, and one would be better off using
   the same character escape sequences as for strings.  But well,
   we have to live with these types of complications for the sake of
   compatibility--maybe later on we can add a variable that modifies
   this reader behavior to something more rational (such as sharp
   followed by the actual character, with slash for escapes).  Arc
   does not otherwise use the #-sign for anything else. */
static value read_char(carc *c, value src, int *index)
{
  value tok, symch;
  int alldigits, i;
  Rune val, ch, digit;

  if ((ch = readchar(c, src, index)) != '\\') {
    c->signal_error(c, "invalid character constant");
    return(CNIL);
  }

  tok = getsymbol(c, src, index);
  if (carc_strlen(c, tok) == 1)	/* single character */
    return(carc_mkchar(c, carc_strindex(c, tok, 0)));
  if (carc_strlen(c, tok) == 3) {
    /* Possible octal escape */
    alldigits = 1;
    val = 0;
    for (i=0; i<3; i++) {
      digit = carc_strindex(c, tok, i);
      if (!isdigit(digit)) {
	alldigits = 0;
	break;
      }
      val = val * 8 + (digit - '0');
    }
    if (alldigits)
      return(carc_mkchar(c, val));

    /* Possible hexadecimal escape */
    if (carc_strindex(c, tok, 0) == 'x') {
      alldigits = 1;
      val = 0;
      for (i=1; i<3; i++) {
	digit = carc_strindex(c, tok, i);
	if (!isxdigit(digit)) {
	  alldigits = 0;
	  break;
	}
	digit = tolower(digit);
	digit = (digit >= '0' && digit <= '9') ? (digit - '0') : (digit - 'a' + 10);
	val = val * 16 + digit;
      }
      if (alldigits)
	return(carc_mkchar(c, val));
    }
    /* Not an octal or hexadecimal escape */
  }

  /* Possible Unicode escape? */
  if (tolower(carc_strindex(c, tok, 0)) == 'u') {
    alldigits = 1;
    val = 0;
    for (i=1; i<carc_strlen(c, tok); i++) {
      digit = carc_strindex(c, tok, i);
      if (!isxdigit(digit)) {
	alldigits = 0;
	break;
      }
      digit = tolower(digit);
      digit = (digit >= '0' && digit <= '9') ? (digit - '0') : (digit - 'a' + 10);
      val = val * 16 + digit;
    }
    if (alldigits)
      return(carc_mkchar(c, val));
    c->signal_error(c, "invalid Unicode escape");
  }

  /* Symbolic character escape */
  symch = carc_hash_lookup(c, c->charesctbl, tok);
  if (symch == CNIL)
    c->signal_error(c, "invalid character constant");
  return(symch);
}

void carc_init_reader(carc *c)
{
  c->symtable = carc_mkhash(c, 10);
  c->rsymtable = carc_mkhash(c, 10);
  c->fn = carc_intern(c, carc_mkstringc(c, "fn"));
  c->us = carc_intern(c, carc_mkstringc(c, "_"));
  c->quote = carc_intern(c, carc_mkstringc(c, "quote"));
  c->qquote = carc_intern(c, carc_mkstringc(c, "quasiquote"));
  c->unquote = carc_intern(c, carc_mkstringc(c, "unquote"));
  c->unquotesp = carc_intern(c, carc_mkstringc(c, "unquote-splicing"));
  c->compose = carc_intern(c, carc_mkstringc(c, "compose"));
  c->complement = carc_intern(c, carc_mkstringc(c, "complement"));
  c->t = carc_intern(c, carc_mkstringc(c, "t"));
  c->nil = carc_intern(c, carc_mkstringc(c, "nil"));

  c->charesctbl = carc_mkhash(c, 4);
  carc_hash_insert(c, c->charesctbl, carc_mkstringc(c, "nul"),
		   carc_mkchar(c, 0));
  carc_hash_insert(c, c->charesctbl, carc_mkstringc(c, "null"),
		   carc_mkchar(c, 0));
  carc_hash_insert(c, c->charesctbl, carc_mkstringc(c, "backspace"),
		   carc_mkchar(c, 8));
  carc_hash_insert(c, c->charesctbl, carc_mkstringc(c, "tab"),
		   carc_mkchar(c, 9));
  carc_hash_insert(c, c->charesctbl, carc_mkstringc(c, "newline"),
		   carc_mkchar(c, 10));
  carc_hash_insert(c, c->charesctbl, carc_mkstringc(c, "linefeed"),
		   carc_mkchar(c, 10));
  carc_hash_insert(c, c->charesctbl, carc_mkstringc(c, "vtab"),
		   carc_mkchar(c, 11));
  carc_hash_insert(c, c->charesctbl, carc_mkstringc(c, "page"),
		   carc_mkchar(c, 12));
  carc_hash_insert(c, c->charesctbl, carc_mkstringc(c, "return"),
		   carc_mkchar(c, 13));
  carc_hash_insert(c, c->charesctbl, carc_mkstringc(c, "space"),
		   carc_mkchar(c, 32));
  carc_hash_insert(c, c->charesctbl, carc_mkstringc(c, "rubout"),
		   carc_mkchar(c, 127));
}
