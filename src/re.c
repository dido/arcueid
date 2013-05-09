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
#include <string.h>
#include "arcueid.h"
#include "regexp.h"

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

struct regexp_t {
  void *rp;
  value rxstr;
  unsigned int flags;
};

static void regex_marker(arc *c, value regexp, int depth,
			 void (*mark)(struct arc *, value, int))
{
  struct regexp_t *rxdata;

  rxdata = (struct regexp_t *)REP(regexp);
  mark(c, rxdata->rxstr, depth);
}

static void regex_sweeper(arc *c, value regexp)
{
  struct regexp_t *rxdata;

  rxdata = (struct regexp_t *)REP(regexp);
  if (rxdata->rp != NULL) {
    free(rxdata->rp);
    rxdata->rp = NULL;
  }
  rxdata->rxstr = CNIL;
}

static unsigned long regex_hash(arc *c, value regexp, arc_hs *s)
{
  int i;
  unsigned long len;
  struct regexp_t *rxdata;

  rxdata = (struct regexp_t *)REP(regexp);
  len = arc_strlen(c, rxdata->rxstr);
  for (i=0; i<len; i++)
    arc_hash_update(s, (unsigned long)arc_strindex(c, rxdata->rxstr, i));
  return(len);
}

static value regex_iscmp(arc *c, value v1, value v2)
{
  struct regexp_t *r1, *r2;
  r1 = (struct regexp_t *)REP(v1);
  r2 = (struct regexp_t *)REP(v2);
  return((arc_strcmp(c, r1->rxstr, r2->rxstr) == 0) ? CTRUE : CNIL);
}

static AFFDEF(regex_pprint)
{
  AARG(sexpr, disp, fp);
  AOARG(visithash);
  AVAR(dw, wc);
  struct regexp_t *rxdata;
  AFBEGIN;

  (void)visithash;
  (void)disp;
  WV(dw, arc_mkaff(c, __arc_disp_write, CNIL));
  WV(wc, arc_mkaff(c, arc_writec, CNIL));
  AFCALL(AV(wc), arc_mkchar(c, 'r'), AV(fp));
  AFCALL(AV(wc), arc_mkchar(c, '/'), AV(fp));

  rxdata = (struct regexp_t *)REP(AV(sexpr));
  AFCALL(AV(dw), rxdata->rxstr, CTRUE, AV(fp), AV(visithash));
  AFCALL(AV(wc),  arc_mkchar(c, '/'), AV(fp));
  rxdata = (struct regexp_t *)REP(AV(sexpr));
  if (rxdata->flags & REGEXP_MULTILINE)
    AFCALL(AV(wc),  arc_mkchar(c, 'm'), AV(fp));
  rxdata = (struct regexp_t *)REP(AV(sexpr));
  if (rxdata->flags & REGEXP_CASEFOLD)
    AFCALL(AV(wc),  arc_mkchar(c, 'i'), AV(fp));
  ARETURN(CNIL);
  AFEND;
}
AFFEND

extern char __arc_regex_error[];

value arc_mkregexp(arc *c, value s, unsigned int flags)
{
  value regexp;
  struct regexp_t *rxdata;

  regexp = arc_mkobject(c, sizeof(struct regexp_t), T_REGEXP);
  rxdata = (struct regexp_t *)REP(regexp);
  rxdata->rxstr = s;
  rxdata->rp = regcomp(c, s);
  rxdata->flags = flags;
  if (rxdata->rp == NULL) {
    arc_err_cstrfmt(c, __arc_regex_error);
    return(CNIL);
  }
  return(regexp);
}

AFFDEF(arc_regcomp)
{
  AARG(s);
  AOARG(flags);
  AFBEGIN;
  if (!BOUND_P(AV(flags)))
    WV(flags, INT2FIX(0));
  TYPECHECK(AV(s), T_STRING);
  TYPECHECK(AV(flags), T_FIXNUM);
  ARETURN(arc_mkregexp(c, AV(s), FIX2INT(AV(flags))));
  AFEND;
}
AFFEND

#define MAX_RS 32

value arc_regexp_match(arc *c, value regexp, value str)
{
  Reprog *rp;
  Resub rs[MAX_RS];
  int i, rv;
  struct regexp_t *rxdata;
  value subexprs = CNIL;

  rxdata = (struct regexp_t *)REP(regexp);
  rp = rxdata->rp;
  for (i=0; i<MAX_RS; i++)
    rs[i].csp = rs[i].cep = -1;
  if ((rv = rregexec(c, rp, str, rs, 10)) >= 0) {
    /* handle substring matches */
    for (i=MAX_RS-1; i>=0; i--) {
      value sstr;

      if (rs[i].csp < 0)
	continue;
      sstr = arc_substr(c, str, rs[i].csp, rs[i].cep);
      subexprs = cons(c, sstr, subexprs);
    }
    return(cons(c, INT2FIX(rv), cons(c, subexprs, CNIL)));
  }
  if (rv == -1)
    return(CNIL);
  arc_err_cstrfmt(c, "out of relist space");
  return(CNIL);
}

/* A regex can be applied to a string value */
static int regex_apply(arc *c, value thr, value rx)
{
  value str;

  if (arc_thr_argc(c, thr) != 1) {
    arc_err_cstrfmt(c, "application of a regex expects 1 argument, given %d",
		    arc_thr_argc(c, thr));
    return(TR_RC);
  }
  str = arc_thr_pop(c, thr);
  if (TYPE(str) != T_STRING) {
    arc_err_cstrfmt(c, "application of a regex expects type <string> as argument");
    return(TR_RC);
  }
  arc_thr_set_valr(c, thr, arc_regexp_match(c, rx, str));
  return(TR_RC);
}

typefn_t __arc_regexp_typefn__ = {
  regex_marker,
  regex_sweeper,
  regex_pprint,
  regex_hash,
  regex_iscmp,
  NULL,
  regex_apply,
  NULL
};
