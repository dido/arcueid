/* 
  Copyright (C) 2011 Rafael R. Sevilla

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
#include "vmengine.h"
#include "symbols.h"

/* Macro expansion.  This will look for any macro applications in e
   and attempt to expand them.

   This current implementation can only expand macros which are defined
   by global symbols.  I don't know if you should be able to define a
   macro in a local environment or even create an anonymous macro and
   apply it directly, e.g. (+ 10 ((annotate 'mac (fn () (list '+ 1
   2))).

   Note that neither reference Arc nor Anarki can handle this case, as
   it seems that macro definitions should be known at read (i.e. compile)
   time, when macros are expanded.  I don't know that anonymous or local
   macros are particularly useful either.

   Also, I don't see any other atomic types that can evaluate to
   macros, so the eval inside the reference Arc implementation becomes
   nothing but a lookup in the global symbol table here.
*/
value arc_macex(arc *c, value e)
{
  value op;

  if (!CONS_P(e))
    return(e);
  op = car(e);
  /* I don't know if it's possible to make any other type of atom evaluate
     to a macro. */
  if (!SYMBOL_P(op))
    return(e);

  /* Look up the symbol's binding in the global symbol table */
  op = arc_hash_lookup(c, c->genv, op);
  if (TYPE(op) != T_MACRO)
    return(e);			/* not a macro */
  return(arc_macapply(c, op, cdr(e)));
}

static value compile_literal(arc *c, value lit, value ctx, value cont);
static value compile_ident(arc *c, value ident, value ctx, value cont);
static value compile_list(arc *c, value list, value ctx, value cont);
static value compile_continuation(arc *c, value ctx, value cont);
static int find_literal(arc *c, value ctx, value lit);

/* Given an expression nexpr, a compilation context ctx, and a continuation
   flag, return the compilation context after the expression is compiled.
   NOTE: all macros must be fully expanded before compiling! */
value arc_compile(arc *c, value nexpr, value ctx, value cont)
{
  value expr;

  expr = arc_macex(c, nexpr);
  if (expr == ARC_BUILTIN(c, S_T) || expr == ARC_BUILTIN(c, S_NIL)
      || expr == CNIL || expr == CTRUE
      || TYPE(expr) == T_CHAR || TYPE(expr) == T_STRING
      || TYPE(expr) == T_FIXNUM || TYPE(expr) == T_BIGNUM
      || TYPE(expr) == T_FLONUM || TYPE(expr) == T_RATIONAL
      || TYPE(expr) == T_RATIONAL || TYPE(expr) == T_COMPLEX)
    return(compile_literal(c, expr, ctx, cont));
  if (SYMBOL_P(expr))
    return(compile_ident(c, expr, ctx, cont));
  if (CONS_P(expr))
    return(compile_list(c, expr, ctx, cont));
  c->signal_error(c, "invalid_expression %p", expr);
  return(ctx);
}

static value compile_continuation(arc *c, value ctx, value cont)
{
  if (cont != CNIL)
    arc_gcode(c, ctx, iret);
  return(ctx);
}

/* Find a literal lit in ctx.  If not found, create it and add to the
   literals in the ctx. */
static int find_literal(arc *c, value ctx, value lit)
{
  value lits;
  int i;

  lits = CCTX_LITS(ctx);
  for (i=0; i<VECLEN(lits); i++) {
    if (arc_iso(c, VINDEX(lits, i), lit))
      return(i);
  }
  /* create the literal since it doesn't exist */
  return(arc_literal(c, ctx, lit));
}

static value compile_literal(arc *c, value lit, value ctx, value cont)
{
  if (lit == ARC_BUILTIN(c, S_NIL) || lit == CNIL) {
    arc_gcode(c, ctx, inil);
  } else if (lit == ARC_BUILTIN(c, S_T) || lit == CTRUE) {
    arc_gcode(c, ctx, itrue);
  } else if (FIXNUM_P(lit)) {
    arc_gcode1(c, ctx, ildi, lit);
  } else {
    arc_gcode1(c, ctx, ildl, find_literal(c, ctx, lit));
  }
  return(compile_continuation(c, ctx, cont));
}

static value compile_ident(arc *c, value ident, value ctx, value cont)
{
  return(CNIL);
}

static value compile_list(arc *c, value list, value ctx, value cont)
{
  return(CNIL);
}
