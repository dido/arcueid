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

#define READ(fp, eof, val)				\
  AFCALL(arc_mkaff(c, arc_sread, CNIL), fp, eof);	\
  val = AFCRV

#define READC(fp, val)					\
  AFCALL(arc_mkaff(c, arc_readc, CNIL), fp);		\
  val = AFCRV

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

/* I imagine this can be done a lot more cleanly! */
static AFFDEF(expand_compose)
{
  AARG(sym);
  AVAR(top, last, elt, sh, ch);
  AVAR(negate, run);
  Rune rch;
  AFBEGIN;

  AV(sh) = arc_instring(c, AV(sym), CNIL);
  AV(top) = AV(elt) = AV(last) = AV(negate) = CNIL;
  AV(run) = CTRUE;
  while (AV(run) == CTRUE) {
    READC(AV(sh), AV(ch));
    rch = (NIL_P(AV(ch))) ? -1 : arc_char2rune(c, AV(ch));
    if (rch == ':' || rch == -1) {
      if (!NIL_P(AV(elt)) && arc_strlen(c, AV(elt)) > 0) {
	READ(arc_instring(c, AV(elt), CNIL), CNIL, AV(elt));
      } else {
	AV(elt) = CNIL;
      }
      if (AV(negate) == CTRUE) {
	AV(elt) = (NIL_P(AV(elt))) ? ARC_BUILTIN(c, S_NO)
	  : cons(c, ARC_BUILTIN(c, S_COMPLEMENT), cons(c, AV(elt), CNIL));
	if (NIL_P(AV(ch)) && AV(top) == CNIL)
	  ARETURN(AV(elt));
	AV(negate) = CNIL;
      }
      if (NIL_P(AV(elt))) {
	if (NIL_P(AV(ch)))
	  AV(run) = CNIL;
	continue;
      }
      AV(elt) = cons(c, AV(elt), CNIL);
      if (AV(last))
	scdr(AV(last), AV(elt));
      else
	AV(top) = AV(elt);
      AV(last) = AV(elt);
      AV(elt) = CNIL;
      if (NIL_P(AV(ch)))
	AV(run) = CNIL;
    } else if (rch == '~') {
      AV(negate) = CTRUE;
    } else {
      AV(elt) = (NIL_P(AV(elt))) ? arc_mkstring(c, &rch, 1)
	: arc_strcatc(c, AV(elt), rch);
    }
  }
  if (cdr(AV(top)) == CNIL)
    ARETURN(car(AV(top)));
  ARETURN(cons(c, ARC_BUILTIN(c, S_COMPOSE), AV(top)));
  AFEND;
}
AFFEND

static AFFDEF(expand_sexpr)
{
  AARG(sym);
  AVAR(last, cur, elt, sh, ch, run, prevchar);
  Rune rch;
  AFBEGIN;

  AV(sh) = arc_instring(c, AV(sym), CNIL);
  AV(last) = AV(cur) = AV(elt) = AV(last) = CNIL;
  AV(run) = CTRUE;
  while (AV(run) == CTRUE) {
    READC(AV(sh), AV(ch));
    if (NIL_P(AV(ch))) {
      AV(run) = CNIL;
      continue;
    }
    rch = arc_char2rune(c, AV(ch));
    if (rch == '.' || rch == '!') {
      AV(prevchar) = INT2FIX(rch);
      if (NIL_P(AV(elt)) || arc_strlen(c, AV(elt)) <= 0)
	continue;
      READ(arc_instring(c, AV(elt), CNIL), CNIL, AV(elt));
      if (NIL_P(AV(last)))
	AV(last) = AV(elt);
      else if (FIX2INT(AV(prevchar)) == '!')
	AV(last) = cons(c, AV(last), cons(c, cons(c, ARC_BUILTIN(c, S_QUOTE), cons(c, AV(elt), CNIL)), CNIL));
      else
	AV(last) = cons(c, AV(last), cons(c, AV(elt), CNIL));
      AV(elt) = CNIL;
      continue;
    }
    AV(elt) = (NIL_P(AV(elt))) ? arc_mkstring(c, &rch, 1)
      : arc_strcatc(c, AV(elt), rch);
  }
  READ(arc_instring(c, AV(elt), CNIL), CNIL, AV(elt));
  if (AV(elt) == CNIL) {
    arc_err_cstrfmt(c, "Bad ssyntax %s", sym);
    return(CNIL);
  }
  if (AV(last) == CNIL) {
    if (FIX2INT(AV(prevchar)) == '!')
      ARETURN(cons(c, ARC_BUILTIN(c, S_GET), cons(c, cons(c, ARC_BUILTIN(c, S_QUOTE), cons(c, AV(elt), CNIL)), CNIL)));
    ARETURN(cons(c, ARC_BUILTIN(c, S_GET), cons(c, AV(elt), CNIL)));
  }
  if (FIX2INT(AV(prevchar)) == '!')
    ARETURN(cons(c, AV(last), cons(c, cons(c, ARC_BUILTIN(c, S_QUOTE), cons(c, AV(elt), CNIL)), CNIL)));
  ARETURN(cons(c, AV(last), cons(c, AV(elt), CNIL)));
  AFEND;
}
AFFEND

static AFFDEF(expand_and)
{
  AARG(sym);
  AFBEGIN;
  (void)sym;
  ARETURN(CNIL);
  AFEND;
}
AFFEND

static AFFDEF(expand_ssyntax)
{
  AARG(sym);
  AFBEGIN;
  if (arc_strchr(c, AV(sym), ':') != CNIL
      || arc_strchr(c, AV(sym), '~') != CNIL)
    AFTCALL(arc_mkaff(c, expand_compose, CNIL), AV(sym));
  if (arc_strchr(c, AV(sym), '.') != CNIL
      || arc_strchr(c, AV(sym), '!') != CNIL)
    AFTCALL(arc_mkaff(c, expand_sexpr, CNIL), AV(sym));
  if (arc_strchr(c, AV(sym), '&') != CNIL)
    AFTCALL(arc_mkaff(c, expand_and, CNIL), AV(sym));
  ARETURN(CNIL);
  AFEND;
} 
AFFEND

AFFDEF(arc_ssexpand)
{
  AARG(sym);
  value x;
  AFBEGIN;

  if (TYPE(AV(sym)) != T_SYMBOL)
    ARETURN(AV(sym));
  x = arc_sym2name(c, AV(sym));
  AFTCALL(arc_mkaff(c, expand_ssyntax, CNIL), x);
  AFEND;
}
AFFEND
