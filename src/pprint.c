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
/* This is not a very pretty pretty printer! ;)  Someday, we'll implement
   Philip Wadler's algebraic pretty printer, but that day is not today... */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "carc.h"
#include "alloc.h"
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

static void append_cstring(carc *c, char *buf, value *ppstr)
{
  value nstr = carc_mkstringc(c, buf);

  *ppstr = (*ppstr == CNIL) ? nstr : carc_strcat(c, *ppstr, nstr);
}

static value prettyprint(carc *c, value sexpr, value *ppstr)
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
  case T_CHAR:
    {
      Rune outstr[3];
      value escape;
 
      if ((escape = carc_hash_lookup(c, c->charesctbl, sexpr)) != CUNBOUND) {
	/* Escape character */
	*ppstr = (*ppstr == CNIL)
	  ? carc_strcat(c, carc_mkstringc(c, "#\\"), escape)
	  : carc_strcat(c, *ppstr, carc_strcat(c, carc_mkstringc(c, "#\\"),
						escape));
      } else {
	outstr[0] = '#';
	outstr[1] = '\\';
	outstr[2] = REP(sexpr)._char;
	*ppstr = (*ppstr == CNIL) ? carc_mkstring(c, outstr, 3)
	  : carc_strcat(c, *ppstr, carc_mkstring(c, outstr, 3));
      }
    }
    break;
#endif
  }
  return(*ppstr);
}

value carc_prettyprint(carc *c, value v)
{
  value ret=CNIL;

  prettyprint(c, v, &ret);
  return(ret);
}
