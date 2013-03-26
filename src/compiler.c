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
#include "vmengine.h"

static value compile_continuation(arc *c, value ctx, value cont)
{
  if (!NIL_P(cont))
    arc_emit(c, ctx, iret);
  return(ctx);
}

/* Find a literal lit in ctx.  If not found, create it and add it to the
   literals in the ctx. */
static int find_literal(arc *c, value ctx, value lit)
{
  value lits;

  int i;
  lits = CCTX_LITS(ctx);
  if (!NIL_P(lits)) {
    for (i=0; i<VECLEN(lits); i++) {
      if (arc_is2(c, VINDEX(lits, i), lit))
	return(i);
    }
  }

  /* create the literal since it doesn't exist */
  return(arc_literal(c, ctx, lit));
}

static value compile_literal(arc *c, value lit, value ctx, value cont)
{
  if (lit == ARC_BUILTIN(c, S_NIL) || lit == CNIL) {
    arc_emit(c, ctx, inil);
  } else if (lit == ARC_BUILTIN(c, S_T) || lit == CTRUE) {
    arc_emit(c, ctx, itrue);
  } else if (FIXNUM_P(lit)) {
    arc_emit1(c, ctx, ildi, lit);
  } else {
    arc_emit1(c, ctx, ildl, find_literal(c, ctx, lit));
  }
  return(compile_continuation(c, ctx, cont));
}

/* Find the symbol var in the environment env.  Each environment frame
   as represented by the compiler is simply a list of hash tables, each
   hash table key being a symbol name, and each value being the index
   inside the environment frame.  Returns CNIL if var is a name unbound
   in the current set of environments.  Returns CTRUE otherwise, and sets
   frameno to the frame number of the environment, and idx to the index in
   that environment.  */
static value find_var(arc *c, value var, value env, int *frameno, int *idx)
{
  value vidx;
  int fnum;

  for (fnum=0; env; env = cdr(env), fnum++) {
    if ((vidx = arc_hash_lookup(c, car(env), var)) != CUNBOUND) {
      *frameno = fnum;
      *idx = FIX2INT(vidx);
      return(CTRUE);
    }
  }
  return(CNIL);
}

static value compile_ident(arc *c, value ident, value ctx, value env,
			   value cont)
{
  int level, offset;

  /* look for the variable in the environment first */
  if (find_var(c, ident, env, &level, &offset) == CTRUE) {
    arc_emit2(c, ctx, ilde, level, offset);
  } else {
    /* If the variable is not bound in the current environment, it's
       a global symbol. */
    arc_emit1(c, ctx, ildg, find_literal(c, ctx, ident));
  }
  return(compile_continuation(c, ctx, cont));

}

static AFFDEF(compile_if, args, ctx, env, cont)
{
  AVAR(jumpaddr, jumpaddr2);
  AFBEGIN;
  /* if we run out of arguments, the last value becomes nil */
  if (NIL_P(AV(args))) {
    arc_emit(c, AV(ctx), inil);
    ARETURN(compile_continuation(c, AV(ctx), AV(cont)));
  }

  /* If the next is the end of the line, compile the tail end if no
     additional */
  if (NIL_P(cdr(AV(args)))) {
    AFTCALL(arc_mkaff(c, arc_compile, CNIL), car(AV(args)), AV(ctx),
	    AV(env), AV(cont));
  }

  /* In the final case, we have the conditional (car), the then portion
     (cadr), and the else portion (cddr). */
  /* First, compile the conditional */
  AFCALL(arc_mkaff(c, arc_compile, CNIL), car(AV(args)), AV(ctx),
	 AV(env), CNIL);
  /* this jump address will be the address of the jf instruction
     which we are about to generate.  We have to patch it with the
     address of the start of the else portion once we know it. */
  AV(jumpaddr) = CCTX_VCPTR(AV(ctx));
  arc_emit1(c, AV(ctx), ijf, INT2FIX(0));
  /* compile the then portion */
  AFCALL(arc_mkaff(c, arc_compile, CNIL), cadr(AV(args)), AV(ctx),
	 AV(env), CNIL);
  /* This second jump target should be patched with the address of the
     unconditional jump at the end.  It should be patched after the else
     portion is compiled. */
  AV(jumpaddr2) = CCTX_VCPTR(AV(ctx));
  arc_emit1(c, AV(ctx), ijmp, INT2FIX(0));
  /* patch jumpaddr so that it will jump to the address of the else
     portion which is about to be compiled */
  arc_jmpoffset(c, AV(ctx), FIX2INT(AV(jumpaddr)),
		FIX2INT(CCTX_VCPTR(AV(ctx))));
  /* compile the else portion, which should be treated as though it were
     an if as well */
  AFCALL(arc_mkaff(c, compile_if, CNIL), cddr(AV(args)), AV(ctx),
	 AV(env), CNIL);
  /* Fix the target address of the conditional jump at the end of the
     then portion (jumpaddr2) */
  arc_jmpoffset(c, AV(ctx), FIX2INT(AV(jumpaddr2)),
		FIX2INT(CCTX_VCPTR(AV(ctx))));
  ARETURN(compile_continuation(c, AV(ctx), AV(cont)));
  AFEND;
}
AFFEND

static int (*spform(arc *c, value ident))(arc *, value)
{
  if (ARC_BUILTIN(c, S_IF) == ident)
    return(compile_if);
  return(NULL);
}

static int (*inline_func(arc *c, value ident))(arc *, value)
{
  return(NULL);
}

static AFFDEF(compile_compose, nexpr, ctx, env, cont)
{
  (void)cont;
  (void)env;
  (void)ctx;
  (void)nexpr;
  ARETURN(CNIL);
}
AFFEND

static AFFDEF(compile_complement, nexpr, ctx, env, cont)
{
  (void)cont;
  (void)env;
  (void)ctx;
  (void)nexpr;
  ARETURN(CNIL);
}
AFFEND

static AFFDEF(compile_apply, nexpr, ctx, env, cont)
{
  (void)cont;
  (void)env;
  (void)ctx;
  (void)nexpr;
  ARETURN(CNIL);
}
AFFEND

static AFFDEF(compile_list, nexpr, ctx, env, cont)
{
  int (*fun)(arc *, value) = NULL;
  value expr;
  AFBEGIN;

  expr = AV(nexpr);
  if ((fun = spform(c, car(expr))) != NULL) {
    AFTCALL(arc_mkaff(c, fun, CNIL), cdr(AV(nexpr)), AV(ctx), AV(env),
	    AV(cont));
  }

  if ((fun = inline_func(c, car(expr))) != NULL) {
    AFTCALL(arc_mkaff(c, fun, CNIL), AV(nexpr), AV(ctx), AV(env), AV(cont));
  }
  /* compose in a functional position */
  if (CONS_P(car(expr)) && car(car(expr)) == ARC_BUILTIN(c, S_COMPOSE)) {
    AFTCALL(arc_mkaff(c, compile_compose, CNIL), AV(nexpr), AV(ctx), AV(env),
	    AV(cont));
  }

  /* complement in a functional position */
  if (CONS_P(car(expr)) && car(car(expr)) == ARC_BUILTIN(c, S_COMPLEMENT)) {
    AFTCALL(arc_mkaff(c, compile_complement, CNIL), AV(nexpr), AV(ctx),
	    AV(env), AV(cont));
  }

  AFTCALL(arc_mkaff(c, compile_apply, CNIL), AV(nexpr), AV(ctx), AV(env),
	  AV(cont));
  AFEND;
}
AFFEND

/* Given an expression nexpr, a compilation context ctx, and a continuation
   flag, return the compilation context after the expression is compiled. */
AFFDEF(arc_compile, nexpr, ctx, env, cont)
{
  AVAR(expr, ssx);
  AFBEGIN;

  /* AFCALL(arc_mkaff(c, macex, CNIL), AV(nexpr), CTRUE);
     AV(expr) = AFCRV;
  */
  AV(expr) = AV(nexpr);
  if (AV(expr) == ARC_BUILTIN(c, S_T) || AV(expr) == ARC_BUILTIN(c, S_NIL)
      || NIL_P(AV(expr)) || AV(expr) == CTRUE
      || TYPE(AV(expr)) == T_CHAR || TYPE(AV(expr)) == T_STRING
      || TYPE(AV(expr)) == T_FIXNUM || TYPE(AV(expr)) == T_BIGNUM
      || TYPE(AV(expr)) == T_FLONUM || TYPE(AV(expr)) == T_RATIONAL
      || TYPE(AV(expr)) == T_RATIONAL || TYPE(AV(expr)) == T_COMPLEX) {
    ARETURN(compile_literal(c, AV(expr), AV(ctx), AV(cont)));
  }

  if (SYMBOL_P(AV(expr))) {
    /*    AFCALL(arc_mkaff(c, arc_ssexpand, CNIL), AV(expr));
	  AV(ssx) = AFCRV; */
    AV(ssx) = CNIL;
    if (NIL_P(AV(ssx))) {
      ARETURN(compile_ident(c, AV(expr), AV(ctx), AV(env), AV(cont)));
    }
    AFTCALL(arc_mkaff(c, arc_compile, CNIL), AV(ssx), AV(ctx),
	    AV(env), AV(cont));
  }
  if (CONS_P(AV(expr))) {
    AFTCALL(arc_mkaff(c, compile_list, CNIL), AV(expr), AV(ctx),
	    AV(env), AV(cont));
  }
  arc_err_cstrfmt(c, "invalid_expression");
  ARETURN(AV(ctx));
  AFEND;
}
AFFEND

