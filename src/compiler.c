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

/* Get the closest line number for obj */
static value get_lineno(arc *c, value obj)
{
  value lndata;

  lndata = arc_cmark(c, ARC_BUILTIN(c, S_LNDATA));
  if (NIL_P(lndata))
    return(CUNBOUND);
  return(__arc_get_fileline(c, lndata, obj));
}

/* Given a symbol op, return the macro corresponding to it, if any.  If
   it is not a macro, return nil. */
static int ismacro(arc *c, value op)
{
  while (arc_type(c, op = arc_hash_lookup(c, c->genv, op)) == T_SYMBOL)
    ;
  if (arc_type(c, op) == ARC_BUILTIN(c, S_MAC))
    return(op);
  return(CNIL);
}

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
static AFFDEF(macex)
{
  AARG(e, once);
  AVAR(op, expansion);
  AFBEGIN;
  do {
    if (!CONS_P(AV(e)))
      ARETURN(AV(e));
    WV(op, car(AV(e)));
    /* I don't know if it's possible to make any other type of atom evaluate
       to a macro. */
    if (!SYMBOL_P(AV(op)))
      ARETURN(AV(e));

    WV(op, ismacro(c, AV(op)));
    if (NIL_P(AV(op)))
      ARETURN(AV(e));		/* not a macro */

    AFCALL2(arc_rep(c, AV(op)), cdr(AV(e)));
    WV(expansion, AFCRV);
    WV(e, AV(expansion));
  } while (AV(once) == CTRUE);
  AFEND;
}
AFFEND

static value compile_continuation(arc *c, value ctx, value cont)
{
  if (!NIL_P(cont))
    arc_emit(c, ctx, iret, get_lineno(c, CNIL));
  return(ctx);
}

extern void __arc_print_string(arc *c, value ppstr);

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
    arc_emit(c, ctx, inil, get_lineno(c, CNIL));
  } else if (lit == ARC_BUILTIN(c, S_T) || lit == CTRUE) {
    arc_emit(c, ctx, itrue, get_lineno(c, CNIL));
  } else if (FIXNUM_P(lit)) {
    arc_emit1(c, ctx, ildi, lit, get_lineno(c, CNIL));
  } else {
    arc_emit1(c, ctx, ildl, find_literal(c, ctx, lit), get_lineno(c, CNIL));
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
    arc_emit2(c, ctx, ilde, INT2FIX(level), INT2FIX(offset),
	      get_lineno(c, CNIL));
  } else {
    /* If the variable is not bound in the current environment, it's
       a global symbol. */
    arc_emit1(c, ctx, ildg, find_literal(c, ctx, ident), get_lineno(c, CNIL));
  }
  return(compile_continuation(c, ctx, cont));

}

static AFFDEF(compile_if)
{
  AARG(args, ctx, env, cont);
  AVAR(jumpaddr, jumpaddr2);
  AFBEGIN;
  /* if we run out of arguments, the last value becomes nil */
  if (NIL_P(AV(args))) {
    arc_emit(c, AV(ctx), inil, get_lineno(c, AV(args)));
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
  WV(jumpaddr, CCTX_VCPTR(AV(ctx)));
  arc_emit1(c, AV(ctx), ijf, INT2FIX(0), get_lineno(c, AV(args)));
  /* compile the then portion */
  AFCALL(arc_mkaff(c, arc_compile, CNIL), cadr(AV(args)), AV(ctx),
	 AV(env), CNIL);
  /* This second jump target should be patched with the address of the
     unconditional jump at the end.  It should be patched after the else
     portion is compiled. */
  WV(jumpaddr2, CCTX_VCPTR(AV(ctx)));
  arc_emit1(c, AV(ctx), ijmp, INT2FIX(0), get_lineno(c, AV(args)));
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

#define FIXINC(x) (WV(x, INT2FIX(FIX2INT(AV(x)) + 1)))

/* To perform a destructuring bind, we begin by assuming that the
   value to be unbound is in the value argument.  We traverse the
   destructuring bind argument (which can be considered a binary
   tree).  During the traversal, there are several possibilities:

   1. We find a symbol.  In this case, we create an iste instruction
      that stores the value thus unbound to get there.
   2. We find a cons cell.  We decide to visit the car and cdr of the
      cell if one is not null.  In any case, we push the argument onto
      the stack (so we can pop it again later) generate a car or cdr
      instruction, and then call ourselves recursively with it.

*/
static AFFDEF(destructure)
{
  AARG(arg, ctx, env, idx);
  AOARG(optional);
  AVAR(frame, jumpaddr);
  AFBEGIN;

  WV(frame, car(AV(env)));
  if (TYPE(AV(arg)) == T_SYMBOL) {
    WV(jumpaddr, CCTX_VCPTR(AV(ctx)));
    arc_emit1(c, AV(ctx), ijbnd, INT2FIX(0), get_lineno(c, AV(arg)));
    arc_emit(c, AV(ctx), inil, get_lineno(c, AV(arg)));
    arc_jmpoffset(c, AV(ctx), FIX2INT(AV(jumpaddr)), 
		    FIX2INT(CCTX_VCPTR(AV(ctx))));
    arc_emit2(c, AV(ctx), iste, INT2FIX(0), AV(idx), get_lineno(c, AV(arg)));
    add_env_name(c, AV(frame), AV(arg), AV(idx));
    FIXINC(idx);
    ARETURN(AV(idx));
  }

  if (TYPE(AV(arg)) != T_CONS) {
    arc_err_cstrfmt_line(c, get_lineno(c, AV(arg)), "invalid fn arg");
    ARETURN(AV(idx));
  }

  /* Optional arguments are only processed if the optional flag is set */
  if (BOUND_P(AV(optional)) && !NIL_P(AV(optional))) {
    /* If we have an optional argument, we have to do some contortions */
    if (car(AV(arg)) == ARC_BUILTIN(c, S_O) && !NIL_P(cdr(AV(arg))) &&
	!NIL_P(cadr(AV(arg)))) {
      value oargname, oargdef;
      oargname = cadr(AV(arg));
      if (!SYMBOL_P(oargname)) {
	arc_err_cstrfmt_line(c, get_lineno(c, car(AV(arg))),
			     "optional arg is not an identifier");
	ARETURN(AV(idx));
      }
      oargdef = (NIL_P(cddr(AV(arg)))) ? CNIL : car(cddr(AV(arg)));
      WV(jumpaddr, CCTX_VCPTR(AV(ctx)));
      arc_emit1(c, AV(ctx), ijbnd, INT2FIX(0), get_lineno(c, AV(arg)));
      /* compile the optional argument's definition */
      AFCALL(arc_mkaff(c, arc_compile, CNIL), oargdef, AV(ctx), AV(env), CNIL);
      arc_jmpoffset(c, AV(ctx), FIX2INT(AV(jumpaddr)), 
		    FIX2INT(CCTX_VCPTR(AV(ctx))));
      arc_emit2(c, AV(ctx), iste, INT2FIX(0), AV(idx), get_lineno(c, AV(arg)));
      add_env_name(c, AV(frame), cadr(AV(arg)), AV(idx));
      FIXINC(idx);
      ARETURN(AV(idx));
    }
  }

  /* an optional argument can only appear in the car of a destructuring
     bind. */
  if (!NIL_P(car(AV(arg))) && !NIL_P(cdr(AV(arg)))) {
    arc_emit(c, AV(ctx), ipush, get_lineno(c, AV(arg)));
    arc_emit(c, AV(ctx), idcar, get_lineno(c, AV(arg)));
    AFCALL(arc_mkaff(c, destructure, CNIL), car(AV(arg)), AV(ctx),
	   AV(env), AV(idx), CTRUE);
    WV(idx, AFCRV);
    arc_emit(c, AV(ctx), ipop, get_lineno(c, AV(arg)));
    arc_emit(c, AV(ctx), idcdr, get_lineno(c, AV(arg)));
    AFTCALL(arc_mkaff(c, destructure, CNIL), cdr(AV(arg)), AV(ctx),
	    AV(env), AV(idx), CNIL);
  } else if (!NIL_P(car(AV(arg))) && NIL_P(cdr(AV(arg)))) {
    arc_emit(c, AV(ctx), idcar, get_lineno(c, AV(arg)));
    AFTCALL(arc_mkaff(c, destructure, CNIL), car(AV(arg)), AV(ctx),
	    AV(env), AV(idx), CTRUE);
  } else if (NIL_P(car(AV(arg))) && !NIL_P(cdr(AV(arg)))) {
    arc_emit(c, AV(ctx), idcdr, get_lineno(c, AV(arg)));
    AFTCALL(arc_mkaff(c, destructure, CNIL), cdr(AV(arg)), AV(ctx),
	    AV(env), AV(idx));
  }
  ARETURN(AV(idx));
  AFEND;
}
AFFEND

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
static AFFDEF(compile_args)
{
  AARG(args, ctx, env);
  AVAR(nframe, envptr, jumpaddr, dsb, oldidx);
  AVAR(regargs, dsbargs, optargs, idx, optargbegin);
  AFBEGIN;

  /* just return the current environment if no args */
  if (AV(args) == CNIL)
    ARETURN(AV(env));

  if (SYMBOL_P(AV(args))) {
    /* if args is a single name, make an environment with a single
       name and a list containing the name of the sole argument. */
    WV(nframe, arc_mkhash(c, ARC_HASHBITS));
    add_env_name(c, AV(nframe), AV(args), INT2FIX(0));
    arc_emit3(c, AV(ctx), ienvr, INT2FIX(0), INT2FIX(0), INT2FIX(0),
	      get_lineno(c, AV(args)));
    WV(env, cons(c, AV(nframe), AV(env)));
    ARETURN(AV(env));
  }

  if (!CONS_P(AV(args))) {
    arc_err_cstrfmt_line(c, get_lineno(c, AV(args)), "invalid fn arg");
    ARETURN(AV(env));
  }

  /* Iterate over all of the args and obtain counts of each type of arg
     specified */
  WV(regargs, WV(dsbargs, WV(optargs, WV(idx, INT2FIX(0)))));
  WV(optargbegin, WV(dsb, CNIL));
  WV(nframe, arc_mkhash(c, ARC_HASHBITS));
  WV(env, cons(c, AV(nframe), AV(env)));
  /* save address of env instruction -- we will need to patch it later to
     fill in the values, and we may even need to change it to an envr */
  WV(envptr, CCTX_VCPTR(AV(ctx)));
  arc_emit3(c, AV(ctx), ienv, INT2FIX(0), INT2FIX(0), INT2FIX(0),
	    get_lineno(c, AV(args)));
  for (;;) {
    if (SYMBOL_P(car(AV(args)))) {
      if (AV(optargbegin) == CTRUE) {
	arc_err_cstrfmt_line(c, get_lineno(c, AV(args)),
			     "non-optional arg found after optional args");
	ARETURN(AV(env));
      }
      /* Ordinary symbol arg. */
      add_env_name(c, AV(nframe), car(AV(args)), AV(idx));
      FIXINC(idx);
      FIXINC(regargs);
    } else if (CONS_P(car(AV(args)))
	       && car(car(AV(args))) == ARC_BUILTIN(c, S_O)) {
      value oarg, oargname, oargdef;

      /* Optional arg */
      WV(optargbegin, CTRUE);
      oarg = car(AV(args));
      oargname = cadr(oarg);
      if (!SYMBOL_P(oargname)) {
	arc_err_cstrfmt_line(c, get_lineno(c, oarg),
			     "optional arg is not an identifier");
	ARETURN(AV(env));
      }
      oargdef = (NIL_P(cddr(oarg))) ? CNIL : car(cddr(oarg));
      /* jump if bound check -- if we are bound, then don't overwrite
	 the optional value  */
      arc_emit2(c, AV(ctx), ilde, INT2FIX(0), AV(idx), get_lineno(c, oarg));
      WV(jumpaddr, CCTX_VCPTR(AV(ctx)));
      arc_emit1(c, AV(ctx), ijbnd, INT2FIX(0), get_lineno(c, oarg));
      /* compile the optional argument's definition */
      AFCALL(arc_mkaff(c, arc_compile, CNIL), oargdef, AV(ctx), AV(env), CNIL);
      arc_emit2(c, AV(ctx), iste, INT2FIX(0), AV(idx),
		get_lineno(c, car(AV(args))));
      arc_jmpoffset(c, AV(ctx), FIX2INT(AV(jumpaddr)), 
		    FIX2INT(CCTX_VCPTR(AV(ctx))));
      add_env_name(c, AV(nframe), cadr(car(AV(args))), AV(idx));
      FIXINC(idx);
      FIXINC(optargs);
    } else if (CONS_P(car(AV(args)))) {
      /* We have a destructuring bind argument.  In this case, we have
	 to store the index of the bind first.  We will generate
	 instructions to perform unbinding later. */
      WV(dsb, cons(c, cons(c, car(AV(args)), AV(idx)), AV(dsb)));
      FIXINC(idx);
      FIXINC(regargs);
    } else if (NIL_P(car(AV(args)))) {
      /* increment the index and the regular args without creating
	 a name for the arg in that position if we see a nil. */
      FIXINC(idx);
      FIXINC(regargs);
    } else {
      arc_err_cstrfmt_line(c, get_lineno(c, AV(args)), "invalid fn arg");
      ARETURN(AV(env));
    }

    if (SYMBOL_P(cdr(AV(args)))) {
      /* rest arg */
      add_env_name(c, AV(nframe), cdr(AV(args)), AV(idx));
      /* change to envr instr. */
      SVINDEX(CCTX_VCODE(AV(ctx)), FIX2INT(AV(envptr)), INT2FIX(ienvr));
      FIXINC(idx);
      break;
    } else if (NIL_P(cdr(AV(args)))) {
      /* done */
      break;
    }
    WV(args, cdr(AV(args)));
  }

  WV(oldidx, AV(idx));
  while (!NIL_P(AV(dsb))) {
    value elem = car(AV(dsb));

    /* To begin a destructuring bind, we first load the value of the
       argument to be destructured ... */
    arc_emit2(c, AV(ctx), ilde, FIX2INT(0), cdr(elem), get_lineno(c, AV(args)));
    /* ... then we generate car and cdr instructions to reach each of
       the names to which we do the destructuring. */
    AFCALL(arc_mkaff(c, destructure, CNIL),
	   car(elem), AV(ctx), AV(env), AV(idx));
    WV(idx, AFCRV);
    WV(dsb, cdr(AV(dsb)));
  }
  WV(dsbargs, INT2FIX(FIX2INT(AV(idx)) - FIX2INT(AV(oldidx))));

  /* adjust the env instruction based on the number of args we
     actually have */
  SVINDEX(CCTX_VCODE(AV(ctx)), FIX2INT(AV(envptr)) + 1, AV(regargs));
  SVINDEX(CCTX_VCODE(AV(ctx)), FIX2INT(AV(envptr)) + 2, AV(dsbargs));
  SVINDEX(CCTX_VCODE(AV(ctx)), FIX2INT(AV(envptr)) + 3, AV(optargs));
  ARETURN(AV(env));
  AFEND;
}
AFFEND

static AFFDEF(compile_fn)
{
  AARG(expr, ctx, env, cont);
  AVAR(args, body, nctx, nenv, newcode, stmts);
  AFBEGIN;

  WV(stmts, INT2FIX(0));
  WV(args, car(AV(expr)));
  WV(body, cdr(AV(expr)));
  WV(nctx, arc_mkcctx(c));
  /* copy the CODE_SRC from the original ctx to this one */
  SCCTX_SRC(AV(nctx), CCTX_SRC(AV(ctx)));
  AFCALL(arc_mkaff(c, compile_args, CNIL),
	 AV(args), AV(nctx), AV(env));
  WV(nenv, AFCRV);
  /* the body of a fn works as an implicit do/progn */
  for (; AV(body); WV(body, cdr(AV(body)))) {
    /* The last statement in the body gets compiled with the 
       continuation flag set true. */
    AFCALL(arc_mkaff(c, arc_compile, CNIL),
	   car(AV(body)), AV(nctx), AV(nenv),
	   (NIL_P(cdr(AV(body)))) ? CTRUE : CNIL);
    WV(stmts, INT2FIX(FIX2INT(AV(stmts)) + 1));
  }
  /* if we have an empty list of statements add a nil instruction */
  if (AV(stmts) == INT2FIX(0)) {
    arc_emit(c, AV(nctx), inil, get_lineno(c, cdr(AV(expr))));
    arc_emit(c, AV(nctx), iret, get_lineno(c, cdr(AV(expr))));
  }
  /* convert the new context into a code object and generate an
     instruction in the present context to load it as a literal,
     then create a closure using the code object and the current
     environment. */
  WV(newcode, arc_cctx2code(c, AV(nctx)));
  arc_emit1(c, AV(ctx), ildl, find_literal(c, AV(ctx), AV(newcode)),
	    get_lineno(c, AV(expr)));
  arc_emit(c, AV(ctx), icls, get_lineno(c, AV(expr)));
  ARETURN(compile_continuation(c, AV(ctx), AV(cont)));
  AFEND;
}
AFFEND

static AFFDEF(compile_quote)
{
  AARG(expr, ctx, env, cont);
  AFBEGIN;
  (void)env;	    /* not used */
  /* Anything that is quoted becomes a literal */
  ARETURN(compile_literal(c, car(AV(expr)), AV(ctx), AV(cont)));
  AFEND;
}
AFFEND

#if 0

/* This implementation of quasiquote more or less follows Common Lisp
   semantics, and is not compatible with reference Arc, which doesn't seem
   to handle nested quasiquotes in a reasonable manner.  It is based on
   code provided by fallintothis:

   https://bitbucket.org/fallintothis/qq/src/04a5dfbc592e5bed58b7a12fbbc34dcd5f5f254f/qq.arc?at=default
   http://arclanguage.org/item?id=9962
*/
static int qqexpand(arc *c, value thr);

/* Test whether the given expr may yield multiple list epelements.  Note:
   not only does ,@x splice, but so does ,,@x (unlike in reference Arc) */
static AFFDEF(splicing)
{
  AARG(expr);
  AFBEGIN;
  if (!CONS_P(AV(expr)))
    ARETURN(CNIL);
  if (car(AV(expr)) == ARC_BUILTIN(c, S_UNQUOTESP))
    ARETURN(CTRUE);
  if (car(AV(expr)) == ARC_BUILTIN(c, S_UNQUOTE))
    AFTCALL(arc_mkaff(c, splicing, CNIL), cadr(AV(expr)));
  ARETURN(CNIL);
  AFEND;
}
AFFEND

static AFFDEF(qqcons)
{
  AARG(expr1, expr2);
  value operator;
  AFBEGIN;
  AFCALL(arc_mkaff(c, splicing, CNIL), AV(expr1));
  operator = (NIL_P(AFCRV)) ? ARC_BUILTIN(c, S_DLIST), ARC_BUILTIN(c, S_CONS);
  /* XXX unoptimised version -- we'll add in other optimisations later */
  ARETURN(cons(c, operator, cons(c, AV(expr1), cons(c, AV(expr2), CNIL))));
  AFEND;
}
AFFEND

static AFFDEF(qqlist)
{
  AARG(expr);
  AFBEGIN;
  AFTCALL(arc_mkaff(c, qqcons, CNIL), AV(expr), CNIL);
  AFEND;
}
AFFEND

/* Do the transformations for elements in qq-expand-list that aren't the
   dotted tail.  Also, handle nested quasiquotes. */
static AFFDEF(qqtransform)
{
  AARG(expr);
  AVAR(expansion);
  AFBEGIN;
  if (car(AV(expr)) == ARC_BUILTIN(c, S_UNQUOTE))
    AFTCALL(arc_mkaff(c, qqlist, CNIL), cadr(AV(expr)));
  if (car(AV(expr)) == ARC_BUILTIN(c, S_UNQUOTESP))
    ARETURN(cadr(AV(expr)));
  if (car(AV(expr)) == ARC_BUILTIN(c, S_QQUOTE)) {
    AFCALL(arc_mkaff(c, qqexpand, CNIL), cadr(AV(expr)));
    WV(expansion, AFCRV);
    AFTCALL(arc_mkaff(c, qqlist, CNIL),
	    cons(c, ARC_BUILTIN(c, S_QQUOTE),
		 cons(c, AV(expansion), CNIL)));
  }
  AFCALL(arc_mkaff(c, qqexpand, CNIL), AV(expr));
  AFTCALL(arc_mkaff(c, qqlist, CNIL), AFCRV);
  AFEND;
}
AFFEND

/*  Produce a list of forms suitable for append.
    Note: if we see 'unquote or 'unquote-splicing in the middle of a list, we
    assume it's from dotting, since (a . (unquote b)) == (a unquote b).
    This is a "problem" if the user does something like `(a unquote b c d),
    which we interpret as `(a . ,b). */
static AFFDEF(qqexpandlist)
{
  AARG(expr);
  AVAR(trans);
  AFBEGIN;
  if (!CONS_P(AV(expr))) {
    ARETURN(cons(c,
		 cons(c, ARC_BUILTIN(c, S_QUOTE),
		      cons(c, AV(expr), CNIL)), CNIL));
  }

  if (car(AV(expr)) == ARC_BUILTIN(c, S_UNQUOTE))
    ARETURN(cons(c, cadr(AV(expr)), CNIL));

  if (car(AV(expr)) == ARC_BUILTIN(c, S_UNQUOTESP)) {
    arc_err_cstrfmt_line(c, get_lineno(c, AV(expr)), "invalid use of unquote-splicing");
    ARETURN(CNIL);
  }
  AFCALL(arc_mkaff(c, qqtransform, CNIL), car(AV(expr)));
  WV(trans, AFCRV);
  AFCALL(arc_mkaff(c, qqexpandlist, CNIL), cdr(AV(expr)));
  ARETURN(cons(c, AV(trans), AFCRV));
  AFEND;
}
AFFEND

/* The behaviour is more or less dictated by the Common Lisp Hyperspec's
   general description of backquote:
   `atom/nil -->  'atom/nil
   `,expr     -->  expr
   `,@expr    -->  error
   ``expr     -->  `expr-expanded
   `list-expr -->  expand each element & handle dotted tails:
   `(x1 x2 ... xn)     -->  (append y1 y2 ... yn)
   `(x1 x2 ... . xn)   -->  (append y1 y2 ... 'xn)
   `(x1 x2 ... . ,xn)  -->  (append y1 y2 ... xn)
   `(x1 x2 ... . ,@xn) -->  error
   where each yi is the output of (qq-transform xi).
*/
static AFFDEF(qqexpand)
{
  AARG(expr);
  AFBEGIN;
  if (!CONS_P(AV(expr)))
    ARETURN(cons(c, ARC_BUILTIN(c, S_QUOTE), cons(c, AV(expr), CNIL)));
  if (car(AV(expr)) == ARC_BUILTIN(c, S_UNQUOTE))
    ARETURN(cadr(AV(expr)));
  if (car(AV(expr)) == ARC_BUILTIN(c, S_UNQUOTESP)) {
    arc_err_cstrfmt_line(c, get_lineno(c, AV(expr)), "invalid use of unquote-splicing");
    ARETURN(CNIL);
  }
  if (car(AV(expr)) == ARC_BUILTIN(c, S_QQUOTE)) {
    AFCALL(arc_mkaff(c, qqexpand, CNIL), cadr(AV(expr)));
    ARETURN(cons(c, ARC_BUILTIN(c, S_QQUOTE), cons(c, AFCRV, CNIL)));
  }
  AFCALL(arc_mkaff(c, qqexpandlist, CNIL), AV(expr));
  AFTCALL(arc_mkaff(c, qqappends, CNIL), AFCRV);
  AFEND;
}
AFFEND

static AFFDEF(compile_quasiquote)
{
  AARG(expr, ctx, env, cont);
  AFBEGIN;
  AFCALL(arc_mkaff(c, qqexpand, CNIL), car(AV(expr)));
  AFTCALL(arc_mkaff(c, arc_compile, CNIL), AFCRV, AV(ctx), AV(env), AV(cont));
  AFEND;
}
AFFEND

#endif

static AFFDEF(qquote)
{
  AARG(expr, ctx, env);
  AFBEGIN;
  if (CONS_P(AV(expr)) && car(AV(expr)) == ARC_BUILTIN(c, S_UNQUOTE)) {
    AFCALL(arc_mkaff(c, arc_compile, CNIL), cadr(AV(expr)), AV(ctx),
	   AV(env), CNIL);
    /* no splice */
    ARETURN(CNIL);
  }

  if (CONS_P(AV(expr)) && car(AV(expr)) == ARC_BUILTIN(c, S_UNQUOTESP)) {
    AFCALL(arc_mkaff(c, arc_compile, CNIL), cadr(AV(expr)), AV(ctx),
	   AV(env), CNIL);
    /* splice */
    ARETURN(CTRUE);
  }

  if (CONS_P(AV(expr))) {
    /* If we see a cons, we need to recurse into the cdr of the
       argument first, generating the code for that, then push
       the result, then generate the code for the car of the
       argument, and then generate code to cons them together,
       or splice them together if the return so indicates. */
    AFCALL(arc_mkaff(c, qquote, CNIL), cdr(AV(expr)), AV(ctx), AV(env));
    arc_emit(c, AV(ctx), ipush, get_lineno(c, AV(expr)));
    AFCALL(arc_mkaff(c, qquote, CNIL), car(AV(expr)), AV(ctx), AV(env));
    arc_emit(c, AV(ctx), (NIL_P(AFCRV)) ? iconsr : ispl,
	     get_lineno(c, AV(expr)));
    ARETURN(CNIL);
  }

  compile_literal(c, AV(expr), AV(ctx), CNIL);
  ARETURN(CNIL);
  AFEND;
}
AFFEND

static AFFDEF(compile_quasiquote)
{
  AARG(expr, ctx, env, cont);
  AFBEGIN;
  AFCALL(arc_mkaff(c, qquote, CNIL), car(AV(expr)), AV(ctx), AV(env));
  ARETURN(compile_continuation(c, AV(ctx), AV(cont)));
  AFEND;
}
AFFEND

static AFFDEF(compile_assign)
{
  AARG(expr, ctx, env, cont);
  int frameno, idx;
  AVAR(a, val, envvar);
  AFBEGIN;
  while (AV(expr) != CNIL) {
    AFCALL(arc_mkaff(c, macex, CNIL), car(AV(expr)), CTRUE);
    WV(a, AFCRV);
    WV(val, cadr(AV(expr)));
    if (AV(a) == CNIL) {
      arc_err_cstrfmt_line(c, get_lineno(c, AV(expr)), "can't rebind nil");
    } else if (AV(a) == ARC_BUILTIN(c, S_T)) {
      arc_err_cstrfmt_line(c, get_lineno(c, AV(expr)), "Can't rebind t");
    } else {
      AFCALL(arc_mkaff(c, arc_compile, CNIL), AV(val), AV(ctx),
	     AV(env), CNIL);
      WV(envvar, find_var(c, AV(a), AV(env), &frameno, &idx));
      if (AV(envvar) == CTRUE) {
	arc_emit2(c, AV(ctx), iste, INT2FIX(frameno), INT2FIX(idx),
		  get_lineno(c, AV(expr)));
      } else {
	/* global symbol */
	arc_emit1(c, AV(ctx), istg, find_literal(c, AV(ctx), AV(a)),
		  get_lineno(c, AV(expr)));
      }
    }
    WV(expr, cddr(AV(expr)));
  }
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
  if (ARC_BUILTIN(c, S_QUOTE) == ident)
    return(compile_quote);
  if (ARC_BUILTIN(c, S_QQUOTE) == ident)
    return(compile_quasiquote);
  if (ARC_BUILTIN(c, S_ASSIGN) == ident)
    return(compile_assign);
  return(NULL);
}

#define INLINE_FUNC(name, instr, nargs)					\
  static AFFDEF(inline_##name)						\
  {									\
    AARG(expr, ctx, env, cont);						\
    AVAR(count);       							\
    AFBEGIN;								\
    WV(expr, cdr(AV(expr)));						\
    for (WV(count, FIX2INT(0)); AV(expr); WV(expr, cdr(AV(expr))),	\
	   FIXINC(count)) {						\
      AFCALL(arc_mkaff(c, arc_compile, CNIL), car(AV(expr)),	        \
	     AV(ctx), AV(env), CNIL);					\
      if (cdr(AV(expr)) != CNIL)					\
	arc_emit(c, AV(ctx), ipush, get_lineno(c, AV(expr)));		\
    }									\
    if (AV(count) != INT2FIX(nargs)) {					\
      arc_err_cstrfmt_line(c, get_lineno(c, AV(expr)),			\
			   "inline procedure expects %d arguments (%d passed)", \
			   nargs, FIX2INT(AV(count)));			\
    } else {								\
      arc_emit(c, AV(ctx), instr, get_lineno(c, AV(expr)));		\
    }									\
    AFEND;								\
    ARETURN(compile_continuation(c, AV(ctx), AV(cont)));		\
  }									\
  AFFEND

INLINE_FUNC(cons, icons, 2);
INLINE_FUNC(car, icar, 1);
INLINE_FUNC(cdr, icdr, 1);

static AFFDEF(compile_inlinen)
{
  AARG(inst, expr, ctx, env, cont, base);
  AFBEGIN;
  AFCALL(arc_mkaff(c, arc_compile, CNIL), AV(base), AV(ctx), AV(env), CNIL);
  for (WV(expr, cdr(AV(expr))); AV(expr); WV(expr,cdr(AV(expr)))) {
    arc_emit(c, AV(ctx), ipush, get_lineno(c, AV(expr)));
    AFCALL(arc_mkaff(c, arc_compile, CNIL), car(AV(expr)), AV(ctx),
	   AV(env), CNIL);
    arc_emit(c, AV(ctx), AV(inst), get_lineno(c, AV(expr)));
  }
  ARETURN(compile_continuation(c, AV(ctx), AV(cont)));
  AFEND;
}
AFFEND

static AFFDEF(compile_inlinen2)
{
  AARG(inst, expr, ctx, env, cont, base);
  AVAR(xexpr, xelen);
  AFBEGIN;
  WV(xexpr, cdr(AV(expr)));
  WV(xelen, arc_list_length(c, AV(xexpr)));
  if (AV(xelen) == INT2FIX(0)) {
    arc_err_cstrfmt_line(c, get_lineno(c, AV(expr)),
			 "operator requires at least one argument");
    ARETURN(CNIL);
  } else if (AV(xelen) == INT2FIX(1)) {
    AFTCALL(arc_mkaff(c, compile_inlinen, CNIL), AV(inst),
	    AV(expr), AV(ctx), AV(env), AV(cont), AV(base));
  }
  AFTCALL(arc_mkaff(c, compile_inlinen, CNIL), AV(inst),
	  cons(c, car(AV(expr)), cdr(AV(xexpr))), AV(ctx),
	  AV(env), AV(cont), car(AV(xexpr)));
  AFEND;
}
AFFEND

static AFFDEF(inline_plus)
{
  AARG(expr, ctx, env, cont);
  value xexpr, xelen;
  AFBEGIN;
  xexpr = cdr(AV(expr));
  xelen = arc_list_length(c, xexpr);
  if (xelen == INT2FIX(0))
    AFTCALL(arc_mkaff(c, arc_compile, CNIL), INT2FIX(0), AV(ctx),
	    AV(env), AV(cont));
  if (xelen == INT2FIX(1))
    AFTCALL(arc_mkaff(c, arc_compile, CNIL), car(xexpr), AV(ctx),
	    AV(env), AV(cont));
  AFTCALL(arc_mkaff(c, compile_inlinen2, CNIL), iadd,
	  AV(expr), AV(ctx), AV(env), AV(cont), INT2FIX(0));
  AFEND;
}
AFFEND

static AFFDEF(inline_times)
{
  AARG(expr, ctx, env, cont);
  AFBEGIN;
  AFTCALL(arc_mkaff(c, compile_inlinen, CNIL), imul,
	  AV(expr), AV(ctx), AV(env), AV(cont), INT2FIX(1));
  AFEND;
}
AFFEND

static AFFDEF(inline_minus)
{
  AARG(expr, ctx, env, cont);
  AFBEGIN;
  AFTCALL(arc_mkaff(c, compile_inlinen2, CNIL), isub,
	  AV(expr), AV(ctx), AV(env), AV(cont), INT2FIX(0));
  AFEND;
}
AFFEND

static AFFDEF(inline_div)
{
  AARG(expr, ctx, env, cont);
  AFBEGIN;
  AFTCALL(arc_mkaff(c, compile_inlinen2, CNIL), idiv,
	  AV(expr), AV(ctx), AV(env), AV(cont), INT2FIX(1));
  AFEND;
}
AFFEND

static int (*inline_func(arc *c, value ident))(arc *, value)
{
  if (ident == ARC_BUILTIN(c, S_CONS)) {
    return(inline_cons);
  } else if (ident == ARC_BUILTIN(c, S_CAR)) {
    return(inline_car);
  } else if (ident == ARC_BUILTIN(c, S_CDR)) {
    return(inline_cdr);
  }else if (ident == ARC_BUILTIN(c, S_PLUS)) {
    return(inline_plus);
  } else if (ident == ARC_BUILTIN(c, S_TIMES)) {
    return(inline_times);
  } else if (ident == ARC_BUILTIN(c, S_MINUS)) {
    return(inline_minus);
  } else if (ident == ARC_BUILTIN(c, S_DIV)) {
    return(inline_div);
  }
  return(NULL);
}

static value fold(arc *c, value expr, value final)
{
  if (cdr(expr) == CNIL)
    return(cons(c, car(expr), final));
  return(cons(c, car(expr), cons(c, fold(c, cdr(expr), final), CNIL)));
}

static AFFDEF(compile_compose)
{
  AARG(expr, ctx, env, cont);
  value composer, cargs;
  AFBEGIN;

  composer = cdr(car(AV(expr)));
  cargs = cdr(AV(expr));
  AFTCALL(arc_mkaff(c, arc_compile, CNIL), fold(c, composer, cargs),
	  AV(ctx), AV(env), AV(cont));
  AFEND;
}
AFFEND

static AFFDEF(compile_complement)
{
  AARG(expr, ctx, env, cont);
  value complemented, cargs, result;
  AFBEGIN;

  complemented = cdr(car(AV(expr)));
  cargs = cdr(AV(expr));

  if (!NIL_P(cdr(complemented))) {
    arc_err_cstrfmt_line(c, get_lineno(c, AV(expr)),
			 "complement: wrong number of arguments (1 required)");
    return(CNIL);
  }
  complemented = car(complemented);
  result = cons(c, ARC_BUILTIN(c, S_NO),
		cons(c, cons(c, complemented, cargs), CNIL));
  AFTCALL(arc_mkaff(c, arc_compile, CNIL), result,
	  AV(ctx), AV(env), AV(cont));
  AFEND;
}
AFFEND

static AFFDEF(compile_apply)
{
  AARG(expr, ctx, env, cont);
  AVAR(fname, args, nahd, contaddr, nargs);
  value mac;
  AFBEGIN;

  WV(fname, car(AV(expr)));
  WV(args, cdr(AV(expr)));

  /* Check to see if this is a macro application */
  if (SYMBOL_P(AV(fname)) && !NIL_P(mac = ismacro(c, AV(fname)))) {
    /* Apply the macro by calling it.  Compile the results. */
    AFCALL2(arc_rep(c, mac), AV(args));
    AFTCALL(arc_mkaff(c, arc_compile, CNIL), AFCRV, AV(ctx), AV(env), AV(cont));
    /* tail call: doesn't return -- never gets here */
    ARETURN(AV(ctx));
  }

  /* There are two possible cases here.  If this is not a tail call,
     cont will be nil, so we need to make a continuation. */
  if (NIL_P(AV(cont))) {
    WV(contaddr, CCTX_VCPTR(AV(ctx)));
    arc_emit1(c, AV(ctx), icont, FIX2INT(0), get_lineno(c, AV(expr)));
  }
  WV(nahd, AV(args));
  /* Traverse the arguments, compiling each and pushing them on the stack */
  for (WV(nargs, INT2FIX(0)); AV(nahd); WV(nahd, cdr(AV(nahd))),
	 WV(nargs, INT2FIX(FIX2INT(AV(nargs)) + 1))) {
    AFCALL(arc_mkaff(c, arc_compile, CNIL), car(AV(nahd)),
	   AV(ctx), AV(env), CNIL);
    arc_emit(c, AV(ctx), ipush, get_lineno(c, AV(expr)));
  }
  /* compile the function name, which should load it into the value register */
  AFCALL(arc_mkaff(c, arc_compile, CNIL), AV(fname), AV(ctx), AV(env), CNIL);

  /* If this is a tail call, create a menv instruction to overwrite the
     current environment just before performing the application */
  if (!NIL_P(AV(cont))) {
    arc_emit1(c, AV(ctx), imenv, AV(nargs), get_lineno(c, AV(expr)));
  }
  /* create the apply instruction that will perform the application */
  arc_emit1(c, AV(ctx), iapply, AV(nargs), get_lineno(c, AV(expr)));
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

static AFFDEF(compile_andf)
{
  AARG(expr, ctx, env, cont);
  value andfuncs, andargs, uniqs, body;
  AFBEGIN;

  uniqs = CNIL;
  for (andargs = cdr(AV(expr)); andargs; andargs = cdr(andargs))
    uniqs = cons(c, arc_uniq(c), uniqs);
  andargs = cdr(AV(expr));
  body = CNIL;
  for (andfuncs = cdr(car(AV(expr))); andfuncs; andfuncs = cdr(andfuncs))
    body = cons(c, cons(c, car(andfuncs), uniqs), body);
  body = arc_list_reverse(c, body);
  body = cons(c, cons(c, ARC_BUILTIN(c, S_AND), body), CNIL);
  body = cons(c, cons(c, ARC_BUILTIN(c, S_FN),
		      cons(c, uniqs, body)), andargs);
  AFTCALL(arc_mkaff(c, arc_compile, CNIL), body, AV(ctx),
	  AV(env), AV(cont));
  AFEND;
}
AFFEND

static AFFDEF(compile_list)
{
  AARG(nexpr, ctx, env, cont);
  AVAR(expr, xs);
  int (*fun)(arc *, value) = NULL;
  AFBEGIN;

  /* Special forms: if/fn/quote/quasiquote/assign */
  if ((fun = spform(c, car(AV(nexpr)))) != NULL) {
    AFTCALL(arc_mkaff(c, fun, CNIL), cdr(AV(nexpr)), AV(ctx), AV(env),
	    AV(cont));
  }

  /* expand all ssyntax within the expression if it isn't a special form */
  WV(expr, CNIL);
  WV(xs, AV(nexpr));
  while (!NIL_P(AV(xs))) {
    value result = CNIL;

    if (SYMBOL_P(car(AV(xs)))) {
      AFCALL(arc_mkaff(c, arc_ssexpand, CNIL), car(AV(xs)));
      result = AFCRV;
    }
    if (NIL_P(result))
      result = car(AV(xs));
    WV(expr, cons(c, result, AV(expr)));
    WV(xs, cdr(AV(xs)));
  }
  WV(expr, arc_list_reverse(c, AV(expr)));

  /* Inline functions (cons, car, cdr, +, -, *, /) */
  if ((fun = inline_func(c, car(AV(expr)))) != NULL) {
    AFTCALL(arc_mkaff(c, fun, CNIL), AV(expr), AV(ctx), AV(env), AV(cont));
  }

  /*  the next three clauses could be removed without changing semantics
      ... except that they work for macros (so prob should do this for
      every elt of expr, not just the car)
      (this is also a comment in ac.scm, as this is exactly the same logic) 
  */

  /* compose in a functional position */
  if (CONS_P(car(AV(expr)))
      && car(car(AV(expr))) == ARC_BUILTIN(c, S_COMPOSE)) {
    AFTCALL(arc_mkaff(c, compile_compose, CNIL), AV(expr), AV(ctx), AV(env),
	    AV(cont));
  }

  /* complement in a functional position */
  if (CONS_P(car(AV(expr)))
      && car(car(AV(expr))) == ARC_BUILTIN(c, S_COMPLEMENT)) {
    AFTCALL(arc_mkaff(c, compile_complement, CNIL), AV(expr), AV(ctx),
	    AV(env), AV(cont));
  }

  /* andf in a functional position */
  if (CONS_P(car(AV(expr))) && car(car(AV(expr))) == ARC_BUILTIN(c, S_ANDF)) {
    AFTCALL(arc_mkaff(c, compile_andf, CNIL), AV(expr), AV(ctx),
	    AV(env), AV(cont));
  }

  AFTCALL(arc_mkaff(c, compile_apply, CNIL), AV(expr), AV(ctx), AV(env),
	  AV(cont));
  AFEND;
}
AFFEND

/* Given an expression nexpr, a compilation context ctx, and a continuation
   flag, return the compilation context after the expression is compiled. */
AFFDEF(arc_compile)
{
  AARG(nexpr, ctx, env, cont);
  AVAR(expr, ssx);
  AFBEGIN;

  /* Get the line number associated with the current sexpr */
  get_lineno(c, AV(nexpr));
  WV(expr, AV(nexpr));

  if (LITERAL_P(AV(expr))) {
    ARETURN(compile_literal(c, AV(expr), AV(ctx), AV(cont)));
  }

  if (SYMBOL_P(AV(expr))) {
    AFCALL(arc_mkaff(c, arc_ssexpand, CNIL), AV(expr));
    WV(ssx, AFCRV);
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
  arc_err_cstrfmt_line(c, get_lineno(c, AV(expr)), "invalid_expression");
  ARETURN(AV(ctx));
  AFEND;
}
AFFEND

AFFDEF(arc_macex)
{
  AARG(e);
  AFBEGIN;
  AFTCALL(arc_mkaff(c, macex, CNIL), AV(e), CTRUE);
  AFEND;
}
AFFEND

AFFDEF(arc_macex1)
{
  AARG(e);
  AFBEGIN;
  AFTCALL(arc_mkaff(c, macex, CNIL), AV(e), CNIL);
  AFEND;
}
AFFEND

#define UNIQ_START_VAL 2874
#define UNIQ_PREFIX 'g'

value arc_uniq(arc *c)
{
  static unsigned long long uniqnum = UNIQ_START_VAL;
  char buffer[1024];

  snprintf(buffer, sizeof(buffer)/sizeof(char), "g%llu", uniqnum++);
  return(arc_intern_cstr(c, buffer));
}

/* What we do here is store the line number hash inside a continuation
   mark named lndata.  This use of dynamic-wind ensures that the
   continuation mark gets cleared should the compilation end for
   whatever reason. */
static AFFDEF(beforethunk)
{
  value lndata;
  AFBEGIN;
  lndata = __arc_getenv(c, thr, 1, 1);
  if (BOUND_P(lndata))
    arc_scmark(c, ARC_BUILTIN(c, S_LNDATA), lndata);
  AFEND;
}
AFFEND

static AFFDEF(duringthunk)
{
  AFBEGIN;
  AFTCALL(arc_mkaff(c, arc_compile, CNIL),
	  __arc_getenv(c, thr, 1, 0), /* AV(expr) */
	  __arc_getenv(c, thr, 1, 2), /* AV(ctx) */
	  CNIL, CTRUE);
  AFEND;
}
AFFEND

static AFFDEF(afterthunk)
{
  value lndata;
  AFBEGIN;
  lndata = __arc_getenv(c, thr, 1, 1);
  if (BOUND_P(lndata))
    arc_ccmark(c, ARC_BUILTIN(c, S_LNDATA));
  AFEND;
}
AFFEND

AFFDEF(arc_eval)
{
  AARG(expr);
  AOARG(lndata);
  AVAR(ctx);
  value code, clos;
  AFBEGIN;
  (void)expr;
  __arc_reset_lineno(c, AV(lndata));
  WV(ctx, arc_mkcctx(c));
  if (BOUND_P(AV(lndata)))
    arc_cctx_mksrc(c, AV(ctx));
  AFCALL(arc_mkaff(c, arc_dynamic_wind, CNIL),
	 arc_mkaff2(c, beforethunk, CNIL, TENVR(thr)),
	 arc_mkaff2(c, duringthunk, CNIL, TENVR(thr)),
	 arc_mkaff2(c, afterthunk, CNIL, TENVR(thr)));
  /*
  AFCALL(arc_mkaff(c, arc_compile, CNIL), AV(expr), AV(ctx), CNIL, CTRUE);
  */
  code = arc_cctx2code(c, AV(ctx));
  clos = arc_mkclos(c, code, CNIL);
  return(__arc_affapply(c, thr, CNIL, clos, CLASTARG));
  AFEND;
}
AFFEND
