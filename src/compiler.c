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
static value find_literal(arc *c, value ctx, value lit)
{
  value lits;

  int i;
  lits = CCTX_LITS(ctx);
  if (!NIL_P(lits)) {
    for (i=0; i<VECLEN(lits); i++) {
      if (arc_is2(c, VINDEX(lits, i), lit))
	return(INT2FIX(i));
    }
  }

  /* create the literal since it doesn't exist */
  return(INT2FIX(arc_literal(c, ctx, lit)));
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
    arc_emit2(c, ctx, ilde, INT2FIX(level), INT2FIX(offset));
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

/* Add a new name with index idx to envframe. */
static void add_env_name(arc *c, value envframe, value name, value idx)
{
  arc_hash_insert(c, envframe, name, idx);
  arc_hash_insert(c, envframe, idx, name);
}


/* generate code to set up the new environment given the arguments. 
   After producing the code to generate the new environment, which
   generally consists of an env or envr instruction to create an
   environment of the appropriate size, additional instructions are
   added to fill in optional arguments and perform any destructuring
   binds.  The function returns a new environment, which for the
   compiler is a hash table of argument name to environment index
   mappings, as expected by find_var.

   This might be the most complicated single function in the whole
   compiler!
 */
static AFFDEF(compile_args, args, ctx, env)
{
  AVAR(nframe, optargtbl, dsbtbl);
  int regargs, dsbargs, optargs, idx, optargbegin, restarg;
  value arg;
  AFBEGIN;

  /* just return the current environment if no args */
  if (AV(args) == CNIL)
    ARETURN(AV(env));

  if (SYMBOL_P(AV(args))) {
    /* if args is a single name, make an environment with a single
       name and a list containing the name of the sole argument. */
    AV(nframe) = arc_mkhash(c, ARC_HASHBITS);
    add_env_name(c, AV(nframe), AV(args), INT2FIX(0));
    arc_emit3(c, AV(ctx), ienvr, INT2FIX(0), INT2FIX(0), INT2FIX(0));
    AV(env) = cons(c, AV(nframe), AV(env));
    ARETURN(AV(env));
  }

  if (!CONS_P(AV(args))) {
    arc_err_cstrfmt(c, "invalid fn arg");
    ARETURN(AV(env));
  }

  /* Iterate over all of the args and obtain counts of each type of arg
     specified */
  regargs = dsbargs = optargs = idx = optargbegin = restarg = 0;
  AV(nframe) = arc_mkhash(c, ARC_HASHBITS);
  AV(optargtbl) = arc_mkhash(c, ARC_HASHBITS);
  AV(dsbtbl) = arc_mkhash(c, ARC_HASHBITS);
  for (arg = AV(args);;) {
    if (SYMBOL_P(car(arg))) {
      if (optargbegin) {
	arc_err_cstrfmt(c, "non-optional arg found after optional args");
	ARETURN(AV(env));
      }
      /* Ordinary symbol arg. */
      add_env_name(c, AV(nframe), car(arg), INT2FIX(idx));
      idx++;
      regargs++;
    } else if (CONS_P(car(AV(arg)))
	       && car(car(AV(arg))) == ARC_BUILTIN(c, S_O)) {
      value oarg, oargname, oargdef;

      /* Optional arg */
      optargbegin = 1;
      oarg = car(arg);
      oargname = cadr(oarg);
      if (!SYMBOL_P(oarg)) {
	arc_err_cstrfmt(c, "optional arg is not an identifier");
	ARETURN(AV(env));
      }
      oargdef = (NIL_P(cddr(oarg))) ? CNIL : car(cddr(oarg));
      add_env_name(c, AV(nframe), oargname, INT2FIX(idx));
      idx++;
      optargs++;
      /* Optional arguments are put into a hash, keyed by the name of the
	 argument mapping it to the default value of the argument. */
      arc_hash_insert(c, AV(optargtbl), oargname, oargdef);
      arg = cdr(arg);
      continue;
    } else if (CONS_P(car(arg))) {
      /* Destructuring binds are put into a hash table, indexed
	 by the initial index.  We generate instructions to
	 unbind them later. */
      arc_hash_insert(c, AV(dsbtbl), INT2FIX(idx), car(arg));
      idx++;
    } else if (NIL_P(car(arg))) {
      /* increment the index and the regular args without creating
	 a name for the arg in that position if we see a nil. */
      idx++;
      regargs++;
    } else {
      arc_err_cstrfmt(c, "invalid fn arg");
      ARETURN(AV(env));
    }

    if (SYMBOL_P(cdr(arg))) {
      /* rest arg */
      restarg = 1;
      add_env_name(c, AV(nframe), cdr(arg), INT2FIX(idx));
      idx++;
      break;
    } else if (NIL_P(cdr(arg))) {
      /* done */
      break;
    }
    arg = cdr(arg);
  }

  /* XXX - we need to fix the number of destructuring bind arguments later */
  arc_emit3(c, AV(ctx), (restarg) ? ienvr : ienv,
	    INT2FIX(regargs), INT2FIX(0), INT2FIX(optargs));
  AV(env) = cons(c, AV(nframe), AV(env));
  ARETURN(AV(env));
  AFEND;
}
AFFEND

static AFFDEF(compile_fn, expr, ctx, env, cont)
{
  AVAR(args, body, nctx, nenv, newcode, stmts);
  AFBEGIN;

  AV(stmts) = INT2FIX(0);
  AV(args) = car(AV(expr));
  AV(body) = cdr(AV(expr));
  AV(nctx) = arc_mkcctx(c);
  AFCALL(arc_mkaff(c, compile_args, CNIL),
	 AV(args), AV(nctx), AV(env));
  AV(nenv) = AFCRV;
  /* the body of a fn works as an implicit do/progn */
  for (; AV(body); AV(body) = cdr(AV(body))) {
    /* The last statement in the body gets compiled with the 
       continuation flag set true. */
    AFCALL(arc_mkaff(c, arc_compile, CNIL),
	   car(AV(body)), AV(nctx), AV(nenv),
	   (NIL_P(cdr(AV(body)))) ? CTRUE : CNIL);
    AV(stmts) = INT2FIX(FIX2INT(AV(stmts)) + 1);
  }
  /* if we have an empty list of statements add a nil instruction */
  if (AV(stmts) == INT2FIX(0)) {
    arc_emit(c, AV(nctx), inil);
    arc_emit(c, AV(nctx), iret);
  }
  /* convert the new context into a code object and generate an
     instruction in the present context to load it as a literal,
     then create a closure using the code object and the current
     environment. */
  AV(newcode) = arc_cctx2code(c, AV(nctx));
  arc_emit1(c, AV(ctx), ildl, find_literal(c, AV(ctx), AV(newcode)));
  arc_emit(c, AV(ctx), icls);
  ARETURN(compile_continuation(c, AV(ctx), AV(cont)));
  AFEND;
}
AFFEND

static int (*spform(arc *c, value ident))(arc *, value)
{
  if (ARC_BUILTIN(c, S_IF) == ident)
    return(compile_if);
  if (ARC_BUILTIN(c, S_FN) == ident)
    return(compile_fn);
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

static AFFDEF(compile_apply, expr, ctx, env, cont)
{
  AVAR(fname, args, nahd, contaddr, nargs);
  AFBEGIN;

  AV(fname) = car(AV(expr));
  AV(args) = cdr(AV(expr));

  /* There are two possible cases here.  If this is not a tail call,
     cont will be nil, so we need to make a continuation. */
  if (NIL_P(AV(cont))) {
    AV(contaddr) = CCTX_VCPTR(AV(ctx));
    arc_emit1(c, AV(ctx), icont, FIX2INT(0));
  }
  AV(nahd) = AV(args);
  /* Traverse the arguments, compiling each and pushing them on the stack */
  for (AV(nargs) = INT2FIX(0); AV(nahd); AV(nahd) = cdr(AV(nahd)),
	 AV(nargs) = INT2FIX(FIX2INT(AV(nargs)) + 1)) {
    AFCALL(arc_mkaff(c, arc_compile, CNIL), car(AV(nahd)),
	   AV(ctx), AV(env), CNIL);
    arc_emit(c, AV(ctx), ipush);
  }
  /* compile the function name, which should load it into the value register */
  AFCALL(arc_mkaff(c, arc_compile, CNIL), AV(fname), AV(ctx), AV(env), CNIL);

  /* If this is a tail call, create a menv instruction to overwrite the
     current environment just before performing the application */
  if (!NIL_P(AV(cont))) {
    arc_emit1(c, AV(ctx), imenv, AV(nargs));
  }
  /* create the apply instruction that will perform the application */
  arc_emit1(c, AV(ctx), iapply, AV(nargs));
  /* If we are not a tail call, fix the continuation address */
  if (NIL_P(AV(cont))) {
    arc_jmpoffset(c, AV(ctx), FIX2INT(AV(contaddr)),
		  FIX2INT(CCTX_VCPTR(AV(ctx))));
  }
  /* done */
  /* XXX - this emits a ret instruction that is never reached
     if this compiles a tail call.  If it isn't a tail call, then
     compile_continuation does precisely nothing. */
  /* ARETURN(compile_continuation(c, AV(ctx), AV(cont))); */
  ARETURN(AV(ctx));
  AFEND;
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

