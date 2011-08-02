/* 
  Copyright (C) 2010 Rafael R. Sevilla

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
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA
  02110-1301 USA.
*/
/* XXX -- We should probably rewrite this in Arc just as we wrote
   the compiler in Arc. */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "arcueid.h"
#include "alloc.h"
#include "utf.h"
#include "arith.h"
#include "symbols.h"
#include "../config.h"

#define ID2SYM(x) ((value)(((long)(x))<<8|SYMBOL_FLAG))

value arc_intern(arc *c, value name)
{
  value symval;

  if ((symval = arc_hash_lookup(c, c->symtable, name)) != CUNBOUND)
    return(symval);

  symval = ID2SYM(++c->lastsym);
  arc_hash_insert(c, c->symtable, name, symval);
  arc_hash_insert(c, c->rsymtable, symval, name);
  return(symval);
}

value arc_intern_cstr(arc *c, const char *name)
{
  value symval, symstr;

  if ((symval = arc_hash_lookup_cstr(c, c->symtable, name)) != CUNBOUND)
    return(symval);
  symstr = arc_mkstringc(c, name);
  return(arc_intern(c, symstr));
}

value arc_sym2name(arc *c, value sym)
{
  return(arc_hash_lookup(c, c->rsymtable, sym));
}

#if 0

static Rune scan(arc *c, value src);
static value read_list(arc *c, value src);
static value read_anonf(arc *c, value src);
static value read_quote(arc *c, value src, value sym);
static value read_comma(arc *c, value src);
static value read_string(arc *c, value src);
static value read_char(arc *c, value src);
static void read_comment(arc *c, value src);
static value read_symbol(arc *c, value src);
static value expand_ssyntax(arc *c, value sym);
static value expand_compose(arc *c, value sym);
static value expand_sexpr(arc *c, value sym);
static value expand_and(arc *c, value sym);

value arc_read(arc *c, value src)
{
  Rune ch;

  while ((ch = scan(c, src)) >= 0) {
    switch (ch) {
    case '(':
      return(read_list(c, src));
    case ')':
      c->signal_error(c, "misplaced right paren");
      return(CNIL);
    case '[':
      return(read_anonf(c, src));
    case ']':
      c->signal_error(c, "misplaced right bracket");
      return(CNIL);
    case '\'':
      return(read_quote(c, src, ARC_BUILTIN(c, S_QUOTE)));
    case '`':
      return(read_quote(c, src, ARC_BUILTIN(c, S_QQUOTE)));
    case ',':
      return(read_comma(c, src));
    case '"':
      return(read_string(c, src));
    case '#':
      return(read_char(c, src));
    case ';':
      read_comment(c, src);
    default:
      arc_unreadchar_rune(c, src, ch);
      return(read_symbol(c, src));
      return(CTRUE);
    }
  }
  return(CNIL);
}

static value read_list(arc *c, value src)
{
  value top, val, last;
  Rune ch;

  top = val = last = CNIL;
  while ((ch = scan(c, src)) >= 0) {
    switch (ch) {
    case ';':
      read_comment(c, src);
      break;
    case ')':
      return(top);
    default:
      arc_unreadchar_rune(c, src, ch);
      if (!arc_read(c, src, index, &val))
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

static void read_comment(arc *c, value src, int *index)
{
  Rune ch;

  while ((ch = readchar(c, src, index)) != Runeerror && !ucisnl(ch))
    ;
  if (ch != Runeerror)
    arc_unreadchar_rune(c, src, ch);
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

static value getsymbol(arc *c, value src, int *index)
{
  Rune buf[STRMAX];
  Rune ch;
  int i;
  value sym, nstr;

  sym = CNIL;
  i=0;
  while ((ch = readchar(c, src, index)) != Runeerror && issym(ch)) {
    if (i >= STRMAX) {
      nstr = arc_mkstring(c, buf, i);
      sym = (sym == CNIL) ? nstr : arc_strcat(c, sym, nstr);
      i = 0;
    }
    buf[i++] = ch;
  }
  if (i==0 && sym == CNIL)
    return(CNIL);
  nstr = arc_mkstring(c, buf, i);
  sym = (sym == CNIL) ? nstr : arc_strcat(c, sym, nstr);

  unreadchar(c, src, ch, index);
  return(sym);
}

/* parse a symbol name or number */
static value read_symbol(arc *c, value str, int *index)
{
  value sym, num, ssx;

  if ((sym = getsymbol(c, str, index)) == CNIL)
    c->signal_error(c, "expecting symbol name");
  num = arc_string2num(c, sym);
  if (num == CNIL) {
    ssx = expand_ssyntax(c, sym);
    return((ssx == CNIL) ? arc_intern(c, sym) : ssx);    
  } else
    return(num);
}

/* scan for first non-blank character */
static Rune scan(arc *c, value src)
{
  Rune ch;

  while ((ch = arc_readc_rune(c, src)) >= 0 && ucisspace(ch))
    ;
  return(ch);
}

/* Read an Arc square bracketed anonymous function.  This expands to
   (fn (_) ...) */
static value read_anonf(arc *c, value src, int *index)
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
      ret = cons(c, ARC_BUILTIN(c, S_FN), cons(c, cons(c, ARC_BUILTIN(c, S_US), CNIL), cons(c, top, CNIL)));

      return(ret);
    default:
      unreadchar(c, src, ch, index);
      if (!arc_read(c, src, index, &val))
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

static value read_quote(arc *c, value src, int *index, value sym)
{
  value val;

  if (arc_read(c, src, index, &val) == CNIL)
    c->signal_error(c, "unexpected end of source");
  return(cons(c, sym, cons(c, val, CNIL)));
}

static value read_comma(arc *c, value src, int *index)
{
  Rune ch;

  if ((ch = readchar(c, src, index)) == '@')
    return(read_quote(c, src, index, ARC_BUILTIN(c, S_UNQUOTESP)));
  unreadchar(c, src, ch, index);
  return(read_quote(c, src, index, ARC_BUILTIN(c, S_UNQUOTE)));
}

/* XXX - we need to add support for octal and hexadecimal escapes as well */
static value read_string(arc *c, value src, int *index)
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
	nstr = arc_mkstring(c, buf, i);
	str = (str == CNIL) ? nstr : arc_strcat(c, str, nstr);
	return(str);		/* proper termination */
	break;
      case '\\':
	/* escape character */
	state = 2;
	break;
      default:
	if (i >= STRMAX) {
	  nstr = arc_mkstring(c, buf, i);
	  str = (str == CNIL) ? nstr : arc_strcat(c, str, nstr);
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
	nstr = arc_mkstring(c, buf, i);
	str = (str == CNIL) ? nstr : arc_strcat(c, str, nstr);
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
	  nstr = arc_mkstring(c, buf, i);
	  str = (str == CNIL) ? nstr : arc_strcat(c, str, nstr);
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
static value read_char(arc *c, value src, int *index)
{
  value tok, symch;
  int alldigits, i;
  Rune val, ch, digit;

  if ((ch = readchar(c, src, index)) != '\\') {
    c->signal_error(c, "invalid character constant");
    return(CNIL);
  }

  tok = getsymbol(c, src, index);
  if (arc_strlen(c, tok) == 1)	/* single character */
    return(arc_mkchar(c, arc_strindex(c, tok, 0)));
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
      return(arc_mkchar(c, val));

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
	digit = (digit >= '0' && digit <= '9') ? (digit - '0') : (digit - 'a' + 10);
	val = val * 16 + digit;
      }
      if (alldigits)
	return(arc_mkchar(c, val));
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
      return(arc_mkchar(c, val));
    c->signal_error(c, "invalid Unicode escape");
  }

  /* Symbolic character escape */
  symch = arc_hash_lookup(c, c->charesctbl, tok);
  if (symch == CUNBOUND)
    c->signal_error(c, "invalid character constant");
  return(symch);
}

#endif


value arc_ssyntax(arc *c, value x)
{
  value name;
  int i;
  Rune ch;

  if (TYPE(x) != T_SYMBOL)
    return(CNIL);

  name = arc_sym2name(c, x);
  for (i=0; i<arc_strlen(c, name); i++) {
    ch = arc_strindex(c, name, i);
    if (ch == ':' || ch == '~' || ch == '&' || ch == '.' || ch == '!')
      return(CTRUE);
  }
  return(CNIL);
}

static Rune readxchar(arc *c, value str, int *idx)
{
  if (*idx >= arc_strlen(c, str))
    return(Runeerror);
  return(arc_strindex(c, str, (*idx)++));
}

#define STRMAX 256

/* I imagine this can be done a lot more cleanly! */
static value expand_compose(arc *c, value sym)
{
  value top, last, nelt, elt;
  Rune ch, buf[STRMAX];
  int index = 0, negate = 0, i=0, run = 1;

  top = elt = last = CNIL;
  while (run) {
    ch = readxchar(c, sym, &index);
    switch (ch) {
    case ':':
    case Runeerror:
      nelt = (i > 0) ? arc_mkstring(c, buf, i) : CNIL;
      elt = (elt == CNIL) ? nelt : arc_strcat(c, elt, nelt);
      if (elt != CNIL)
	elt = arc_intern(c, elt);
      i=0;
      if (negate) {
	elt = (elt == CNIL) ? ARC_BUILTIN(c, S_NO) 
	  : cons(c, ARC_BUILTIN(c, S_COMPLEMENT), cons(c, elt, CNIL));
	if (ch == Runeerror && top == CNIL)
	  return(elt);
	negate = 0;
      }
      if (elt == CNIL) {
	if (ch == Runeerror)
	  run = 0;
	continue;
      }
      elt = cons(c, elt, CNIL);
      if (last)
	scdr(last, elt);
      else
	top = elt;
      last = elt;
      elt = CNIL;
      if (ch == Runeerror)
	run = 0;
      break;
    case '~':
      negate = 1;
      break;
    default:
      if (i >= STRMAX) {
	nelt = arc_mkstring(c, buf, i);
	elt = (elt == CNIL) ? nelt : arc_strcat(c, elt, nelt);
	i=0;
      }
      buf[i++] = ch;
      break;
    }
  }
  if (cdr(top) == CNIL)
    return(car(top));
  return(cons(c, ARC_BUILTIN(c, S_COMPOSE), top));
}

static value expand_sexpr(arc *c, value sym)
{
  Rune ch, buf[STRMAX], prevchar;
  value last, cur, nelt, elt;
  int i=0, index=0;

  last = cur = elt = nelt = CNIL;
  while ((ch = readxchar(c, sym, &index)) != Runeerror) {
    switch (ch) {
    case '.':
    case '!':
      prevchar = ch;
      nelt = (i > 0) ? arc_mkstring(c, buf, i) : CNIL;
      elt = (elt == CNIL) ? nelt : arc_strcat(c, elt, nelt);
      i=0;
      if (elt == CNIL)
	continue;
      elt = arc_intern(c, elt);
      if (last == CNIL)
	last = elt;
      else if (prevchar == '!')
	last = cons(c, last, cons(c, ARC_BUILTIN(c, S_QUOTE), cons(c, elt, CNIL)));
      else
	last = cons(c, last, cons(c, elt, CNIL));
      elt = CNIL;
      break;
    default:
      if (i >= STRMAX) {
	nelt = arc_mkstring(c, buf, i);
	elt = (elt == CNIL) ? nelt : arc_strcat(c, elt, nelt);
	i=0;
      }
      buf[i++] = ch;
      break;
    }
  }

  nelt = (i > 0) ? arc_mkstring(c, buf, i) : CNIL;
  elt = (elt == CNIL) ? nelt : arc_strcat(c, elt, nelt);
  elt = arc_intern(c, elt);
  if (elt == CNIL) {
    c->signal_error(c, "Bad ssyntax %s", sym);
    return(CNIL);
  }
  if (last == CNIL) {
    if (prevchar == '!')
      return(cons(c, ARC_BUILTIN(c, S_GET), cons(c, ARC_BUILTIN(c, S_QUOTE), cons(c, elt, CNIL))));
    return(cons(c, ARC_BUILTIN(c, S_GET), cons(c, elt, CNIL)));
  }
  if (prevchar == '!')
    return(cons(c, last, cons(c, ARC_BUILTIN(c, S_QUOTE), cons(c, elt, CNIL))));
  return(cons(c, last, cons(c, elt, CNIL)));
}

static value expand_and(arc *c, value sym)
{
  value top, last, nelt, elt;
  Rune ch, buf[STRMAX];
  int index = 0, i=0, run = 1;

  top = elt = last = CNIL;
  while (run) {
    ch = readxchar(c, sym, &index);
    switch (ch) {
    case '&':
    case Runeerror:
      nelt = (i > 0) ? arc_mkstring(c, buf, i) : CNIL;
      elt = (elt == CNIL) ? nelt : arc_strcat(c, elt, nelt);
      if (elt != CNIL)
	elt = arc_intern(c, elt);
      i=0;
      if (elt == CNIL) {
	if (ch == Runeerror)
	  run = 0;
	continue;
      }
      elt = cons(c, elt, CNIL);
      if (last)
	scdr(last, elt);
      else
	top = elt;
      last = elt;
      elt = CNIL;
      if (ch == Runeerror)
	run = 0;
      break;
    default:
      if (i >= STRMAX) {
	nelt = arc_mkstring(c, buf, i);
	elt = (elt == CNIL) ? nelt : arc_strcat(c, elt, nelt);
	i=0;
      }
      buf[i++] = ch;
      break;
    }
  }
  if (cdr(top) == CNIL)
    return(car(top));
  return(cons(c, ARC_BUILTIN(c, S_ANDF), top));
}

static value expand_ssyntax(arc *c, value sym)
{
  if (arc_strchr(c, sym, ':') != CNIL || arc_strchr(c, sym, '~') != CNIL)
    return(expand_compose(c, sym));
  if (arc_strchr(c, sym, '.') != CNIL || arc_strchr(c, sym, '!') != CNIL)
    return(expand_sexpr(c, sym));
  if (arc_strchr(c, sym, '&') != CNIL)
    return(expand_and(c, sym));
  return(CNIL);
} 

value arc_ssexpand(arc *c, value sym)
{
  value x;

  if (TYPE(sym) != T_SYMBOL)
    return(sym);
  x = arc_sym2name(c, sym);
  return(expand_ssyntax(c, x));
}

static struct {
  char *str;
  Rune val;
} chartbl[] = {
  { "null", 0 }, { "nul", 0 }, { "backspace", 8 }, { "tab", 9 },
  { "newline", 10 }, { "vtab", 11 }, { "page", 12 }, { "return", 13 },
  { "space", 32 }, { "rubout", 127 }, { NULL, -1 }
};

/* We must synchronize this against symbols.h as necessary! */
static char *syms[] = { "fn", "_", "quote", "quasiquote", "unquote",
			"unquote-splicing", "compose", "complement",
			"t", "nil", "no", "andf", "get", "sym",
			"fixnum", "bignum", "flonum", "rational",
			"complex", "char", "string", "cons", "table",
			"input", "output", "exception", "port",
			"thread", "vector", "continuation", "closure",
			"code", "environment", "vmcode", "ccode",
			"custom", "int", "unknown", "re", "im", "num",
			"sig" };

void arc_init_reader(arc *c)
{
  int i;

  /* So that we don't have to add them to the rootset, we mark the
     symbol table, the builtin table, and the character escape table
     and its entries as immutable and immune from garbage collection. */
  c->symtable = arc_mkhash(c, 10);
  BLOCK_IMM(c->symtable);
  c->rsymtable = arc_mkhash(c, 10);
  BLOCK_IMM(c->rsymtable);
  c->builtin = arc_mkvector(c, S_THE_END);
  for (i=0; i<S_THE_END; i++)
    ARC_BUILTIN(c, i) = arc_intern(c, arc_mkstringc(c, syms[i]));

  c->charesctbl = arc_mkhash(c, 5);
  BLOCK_IMM(c->charesctbl);
  for (i=0; chartbl[i].str; i++) {
    value str = arc_mkstringc(c, chartbl[i].str);
    value chr = arc_mkchar(c, chartbl[i].val);
    value cell;

    BLOCK_IMM(str);
    BLOCK_IMM(chr);
    arc_hash_insert(c, c->charesctbl, str, chr);
    cell = arc_hash_lookup2(c, c->charesctbl, str);
    BLOCK_IMM(cell);
    arc_hash_insert(c, c->charesctbl, chr, str);
    cell = arc_hash_lookup2(c, c->charesctbl, chr);
    BLOCK_IMM(cell);
  }
}
