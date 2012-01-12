/* 
  Copyright (C) 2009 Rafael R. Sevilla

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
/* This is not a very pretty pretty printer! ;)  Someday, we'll implement
   Philip Wadler's algebraic pretty printer, but that day is not today... */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arcueid.h"
#include "alloc.h"
#include "utf.h"
#include "symbols.h"
#include "../config.h"

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
#ifndef alloca
# define alloca __builtin_alloca
#endif
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# include <stddef.h>
void *alloca (size_t);
#endif

#define STRMAX 256

static void append_buffer_close(arc *c, Rune *buf, int *idx, value *ppstr)
{
  value nstr;

  nstr = arc_mkstring(c, buf, *idx);
  *ppstr = (*ppstr == CNIL) ? nstr : arc_strcat(c, *ppstr, nstr);
  *idx = 0;
}

static void append_buffer(arc *c, Rune *buf, int *idx, Rune ch, value *ppstr)
{

  if (*idx >= STRMAX)
    append_buffer_close(c, buf, idx, ppstr);
  buf[(*idx)++] = ch;
}

static void append_cstring(arc *c, char *buf, value *ppstr)
{
  value nstr = arc_mkstringc(c, buf);

  *ppstr = (*ppstr == CNIL) ? nstr : arc_strcat(c, *ppstr, nstr);
}

static value prettyprint(arc *c, value sexpr, value *ppstr)
{
  switch (TYPE(sexpr)) {
  case T_NIL:
    append_cstring(c, "nil", ppstr);
    break;
  case T_TRUE:
    append_cstring(c, "t", ppstr);
    break;
  case T_FIXNUM:
    {
      long val = FIX2INT(sexpr);
      int len;
      char *outstr;

      len = snprintf(NULL, 0, "%ld", val) + 1;
      outstr = (char *)alloca(sizeof(char)*len);
      snprintf(outstr, len+1, "%ld", val);
      append_cstring(c, outstr, ppstr);
    }
    break;
  case T_FLONUM:
    {
      double val = REP(sexpr)._flonum;
      int len;
      char *outstr;

      len = snprintf(NULL, 0, "%lf", val) + 1;
      outstr = (char *)alloca(sizeof(char)*len);
      snprintf(outstr, len, "%lf", val);
      append_cstring(c, outstr, ppstr);
    }
    break;
  case T_COMPLEX:
    {
      double re, im;
      int len;
      char *outstr;

      re = REP(sexpr)._complex.re;
      im = REP(sexpr)._complex.im;
      len = snprintf(NULL, 0, "%lf%+lfi", re, im) + 1;
      outstr = (char *)alloca(sizeof(char)*len);
      snprintf(outstr, len, "%lf%+lfi", re, im);
      append_cstring(c, outstr, ppstr);
    }
    break;
#ifdef HAVE_GMP_H
  case T_BIGNUM:
    {
      char *outstr;
      int len;

      len = mpz_sizeinbase(REP(sexpr)._bignum, 10) + 2;
      outstr = (char *)alloca(sizeof(char)*len);
      mpz_get_str(outstr, 10, REP(sexpr)._bignum);
      append_cstring(c, outstr, ppstr);
    }
    break;
  case T_RATIONAL:
    {
      char *outstr;
      int len;

      len = mpz_sizeinbase(mpq_numref(REP(sexpr)._rational), 10)
	+ mpz_sizeinbase(mpq_denref(REP(sexpr)._rational), 10) + 3;
      outstr = (char *)alloca(sizeof(char)*len);
      mpq_get_str(outstr, 10, REP(sexpr)._rational);
      append_cstring(c, outstr, ppstr);
    }
    break;
#endif
  case T_CHAR:
    {
      Rune outstr[3];
      value escape;
 
      if ((escape = arc_hash_lookup(c, c->charesctbl, sexpr)) != CUNBOUND) {
	/* Escape character */
	*ppstr = (*ppstr == CNIL)
	  ? arc_strcat(c, arc_mkstringc(c, "#\\"), escape)
	  : arc_strcat(c, *ppstr, arc_strcat(c, arc_mkstringc(c, "#\\"),
						escape));
      } else {
	outstr[0] = '#';
	outstr[1] = '\\';
	outstr[2] = REP(sexpr)._char;
	*ppstr = (*ppstr == CNIL) ? arc_mkstring(c, outstr, 3)
	  : arc_strcat(c, *ppstr, arc_mkstring(c, outstr, 3));
      }
    }
    break;
  case T_STRING:
    {
      Rune buf[STRMAX], ch;
      int idx=0, i;
      char outstr[4];

      append_buffer(c, buf, &idx, '\"', ppstr);
      for (i=0; i<arc_strlen(c, sexpr); i++) {
	ch = arc_strindex(c, sexpr, i);
	if (ch < 32) {
	  snprintf(outstr, 4, "%.3o", ch);
	  append_buffer(c, buf, &idx, '\\', ppstr);
	  append_buffer(c, buf, &idx, outstr[0], ppstr);
	  append_buffer(c, buf, &idx, outstr[1], ppstr);
	  append_buffer(c, buf, &idx, outstr[2], ppstr);
	} else {
	  append_buffer(c, buf, &idx, ch, ppstr);
	}
      }
      append_buffer(c, buf, &idx, '\"', ppstr);
      append_buffer_close(c, buf, &idx, ppstr);
    }
    break;
  case T_SYMBOL:
    {
      Rune buf[STRMAX], ch;
      int idx=0, i;
      value sym;

      sym = arc_sym2name(c, sexpr);
      for (i=0; i<arc_strlen(c, sym); i++) {
	ch = arc_strindex(c, sym, i);
	append_buffer(c, buf, &idx, ch, ppstr);
      }
      append_buffer_close(c, buf, &idx, ppstr);
    }
    break;
    /* XXX: These cases that deal with composite data structures
       are naive and do not deal with self-references properly.  Well,
       Paul Graham's reference arc3 is no better, so I guess I shouldn't
       feel too bad, but this is something we *really* need to deal with
       in the near future. */
  case T_CONS:
    {
      append_cstring(c, "(", ppstr);
      while (TYPE(sexpr) == T_CONS) {
	prettyprint(c, car(sexpr), ppstr);
	sexpr = cdr(sexpr);
	if (TYPE(sexpr) != CNIL)
	  append_cstring(c, " ", ppstr);
      }

      if (sexpr != CNIL) {
	append_cstring(c, ". ", ppstr);
	prettyprint(c, sexpr, ppstr);
      }

      append_cstring(c, ")", ppstr);
    }
    break;
  case T_VECTOR:
    {
      int i;

      append_cstring(c, "[", ppstr);
      for (i=0; i<VECLEN(sexpr); i++) {
	prettyprint(c, VINDEX(sexpr, i), ppstr);
	append_cstring(c, " ", ppstr);
      }
      append_cstring(c, "]", ppstr);
    }
    break;
  case T_TABLE:
    {
      value val;
      int ctx = 0;

      append_cstring(c, "#hash(", ppstr);
      while ((val = arc_hash_iter(c, sexpr, &ctx)) != CUNBOUND) {
	prettyprint(c, val, ppstr);
	append_cstring(c, " ", ppstr);
      }
      append_cstring(c, ")", ppstr);
    }
    break;
  case T_TAGGED:
    {
      append_cstring(c, "#(tagged ", ppstr);
      prettyprint(c, arc_type(c, sexpr), ppstr);
      append_cstring(c, " ", ppstr);
      prettyprint(c, arc_rep(c, sexpr), ppstr);
      append_cstring(c, ")", ppstr);
    }
    break;
  case T_TBUCKET:
    {
      append_cstring(c, "(", ppstr);
      prettyprint(c, REP(sexpr)._hashbucket.key, ppstr);
      append_cstring(c, " . ", ppstr);
      prettyprint(c, REP(sexpr)._hashbucket.val, ppstr);
      append_cstring(c, ")", ppstr);
    }
    break;
  case T_CODE:
    append_cstring(c, "#<procedure: ", ppstr);
    if (NIL_P(CODE_NAME(sexpr)))
      append_cstring(c, "(anonymous)", ppstr);
    else
      prettyprint(c, CODE_NAME(sexpr), ppstr);
    append_cstring(c, ">", ppstr);    
    break;
  case T_CONT:
    {
      int i;
      append_cstring(c, "#<continuation: ", ppstr);
      for (i=0; i<VECLEN(sexpr); i++) {
	prettyprint(c, VINDEX(sexpr, i), ppstr);
	append_cstring(c, " ", ppstr);
      }
      append_cstring(c, ">", ppstr);
    }
    break;
  case T_XCONT:
    append_cstring(c, "#<xcont>", ppstr);
    break;
  case T_CLOS:
    prettyprint(c, car(sexpr), ppstr);
    break;
  case T_CCODE:
    append_cstring(c, "#<cprocedure: ", ppstr);
    prettyprint(c, REP(sexpr)._cfunc.name, ppstr);
    append_cstring(c, ">", ppstr);
    break;
  case T_EXCEPTION:
    append_cstring(c, "#<exception:", ppstr);
    prettyprint(c, VINDEX(sexpr, 0), ppstr);
    append_cstring(c, ":from:", ppstr);
    prettyprint(c, VINDEX(sexpr, 1), ppstr);
    append_cstring(c, ">", ppstr);
    break;
  case T_THREAD:
    append_cstring(c, "#<thread:", ppstr);
    prettyprint(c, INT2FIX(TTID(sexpr)), ppstr);
    append_cstring(c, ">", ppstr);
    break;
  case T_PORT:
  case T_CUSTOM:
    {
      value nstr = REP(sexpr)._custom.pprint(c, sexpr);

      *ppstr = (*ppstr == CNIL) ? nstr : arc_strcat(c, *ppstr, nstr);
    }
    break;
  }
  return(*ppstr);
}

value arc_prettyprint(arc *c, value v)
{
  value ret=CNIL;

  prettyprint(c, v, &ret);
  return(ret);
}

void arc_print_string(arc *c, value str)
{
  int i, j, nc;
  char buf[UTFmax];
  Rune r;

  for (i=0; i<arc_strlen(c, str); i++) {
    r = arc_strindex(c, str, i);
    nc = runetochar(buf, &r);
    for (j=0; j<nc; j++)
      putchar(buf[j]);
  }
}

value arc_writestr(arc *c, value str, value port)
{
  int i;
  Rune r;

  for (i=0; i<arc_strlen(c, str); i++) {
    r = arc_strindex(c, str, i);
    arc_writec_rune(c, r, port);
  }
  return(str);
}

void arc_writecstr(arc *c, const char *str, value port)
{
  int i;

  for (i=0; i<strlen(str); i++)
    arc_writec_rune(c, (Rune)str[i], port);
}

extern value coerce_string(arc *c, value obj, value argv);

value arc_sdisp(arc *c, value sexpr, value port)
{
  value str;

  switch (TYPE(sexpr)) {
  case T_NIL:
    arc_writecstr(c, "nil", port);
    break;
  case T_TRUE:
  case T_FIXNUM:
  case T_BIGNUM:
  case T_RATIONAL:
  case T_COMPLEX:
  case T_STRING:
  case T_SYMBOL:
    str = coerce_string(c, sexpr, CNIL);
    arc_writestr(c, str, port);
    break;
  case T_CHAR:
    arc_writec(c, sexpr, port);
    break;
  case T_CONS:
    {
      arc_writec_rune(c, '(', port);
      while (CONS_P(sexpr)) {
	arc_sdisp(c, car(sexpr), port);
	sexpr = cdr(sexpr);
	if (TYPE(sexpr) != CNIL)
	  arc_writec_rune(c, ' ', port);
      }

      if (sexpr != CNIL) {
	arc_writec_rune(c, '.', port);
	arc_writec_rune(c, ' ', port);
	arc_sdisp(c, sexpr, port);
      }
      arc_writec_rune(c, ')', port);
    }
    break;
  case T_TABLE:
    {
      value val;
      int ctx = 0;

      arc_writecstr(c, "#hash(", port);
      while ((val = arc_hash_iter(c, sexpr, &ctx)) != CUNBOUND) {
	arc_sdisp(c, val, port);
	arc_writec_rune(c, ' ', port);
      }
      arc_writec_rune(c, ')', port);
    }
  case T_TAGGED:
    {
      arc_writecstr(c, "#(tagged ", port);
      arc_sdisp(c, arc_type(c, sexpr), port);
      arc_writec_rune(c, ' ', port);
      arc_sdisp(c, arc_rep(c, sexpr), port);
      arc_writec_rune(c, ')', port);
    }
    break;
  case T_TBUCKET:
    {
      arc_writec_rune(c, '(', port);
      arc_sdisp(c, REP(sexpr)._hashbucket.key, port);
      arc_writecstr(c, " . ", port);
      arc_sdisp(c, REP(sexpr)._hashbucket.val, port);
      arc_writec_rune(c, ')', port);
    }
    break;
  case T_CODE:
    arc_writecstr(c, "#<procedure: ", port);
    if (NIL_P(CODE_NAME(sexpr)))
      arc_writecstr(c, "(anonymous)", port);
    else
      arc_sdisp(c, CODE_NAME(sexpr), port);
    arc_writecstr(c, ">", port);    
    break;
  case T_CONT:
    {
      int i;
      arc_writecstr(c, "#<continuation: ", port);
      for (i=0; i<VECLEN(sexpr); i++) {
	arc_sdisp(c, VINDEX(sexpr, i), port);
	arc_writecstr(c, " ", port);
      }
      arc_writec_rune(c, '>', port);
    }
    break;
  case T_XCONT:
    arc_writecstr(c, "#<xcont>", port);
    break;
  case T_CLOS:
    arc_sdisp(c, car(sexpr), port);
    break;
  case T_CCODE:
    arc_writecstr(c, "#<cprocedure: ", port);
    arc_sdisp(c, REP(sexpr)._cfunc.name, port);
    arc_writec_rune(c, '>', port);
    break;
  case T_EXCEPTION:
    arc_writecstr(c, "#<exception:", port);
    arc_sdisp(c, VINDEX(sexpr, 0), port);
    arc_writecstr(c, ":from:", port);
    arc_sdisp(c, VINDEX(sexpr, 1), port);
    arc_writec_rune(c, '>', port);
    break;
  case T_THREAD:
    arc_writecstr(c, "#<thread:", port);
    arc_sdisp(c, TTID(sexpr), port);
    arc_writec_rune(c, '>', port);
    break;
  case T_PORT:
  case T_CUSTOM:
    {
      value nstr = REP(sexpr)._custom.pprint(c, sexpr);
      arc_writestr(c, nstr, port);
    }
    break;
  }
  return(sexpr);
}

value arc_disp(arc *c, int argc, value *argv)
{
  value port, val;

  val = (argc == 0) ? CNIL : argv[0];
  port = (argc > 2) ? argv[1] : arc_stdout(c);

  return(arc_sdisp(c, val, port));
}

value arc_swrite(arc *c, value sexpr, value port)
{
  value ret=CNIL;

  prettyprint(c, sexpr, &ret);
  arc_writestr(c, ret, port);
  return(sexpr);
}

value arc_write(arc *c, int argc, value *argv)
{
  value port, val;

  val = (argc == 0) ? CNIL : argv[0];
  port = (argc > 2) ? argv[1] : arc_stdout(c);
  return(arc_swrite(c, val, port));
}
