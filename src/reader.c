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
static value read_special(carc *c, value src, int *index);
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
      *pval = read_special(c, src, index);
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

/* XXX: stub! */
static value read_comma(carc *c, value src, int *index)
{
  return(CNIL);
}

/* XXX: stub! */
static value read_string(carc *c, value src, int *index)
{
  return(CNIL);
}

/* XXX: stub! */
static value read_special(carc *c, value src, int *index)
{
  return(CNIL);
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
}
