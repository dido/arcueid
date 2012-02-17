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
#include "builtin.h"

/* eval */
value arc_eval(arc *c, value argv, value rv, CC4CTX)
{
  value expr, ctx, code, clos;

  CC4VDEFBEGIN;
  CC4VDEFEND;

  if (VECLEN(argv) != 1) {
    arc_err_cstrfmt(c, "eval: wrong number of arguments (%d for 1)", VECLEN(argv));
    return(CNIL);
  }

  CC4BEGIN(c);
  expr = VINDEX(argv, 0);
  ctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, expr, ctx, CNIL, CTRUE);
  code = arc_cctx2code(c, ctx);
  clos = arc_mkclosure(c, code, CNIL);
  CC4CALL(c, argv, clos, 0, CNIL);
  CC4END;
  return(rv);
}


/*
int macdebug;
*/

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
static value macex(arc *c, value e, value once)
{
  value op, expansion;

  do {
    if (!CONS_P(e))
      return(e);
    op = car(e);
    /* I don't know if it's possible to make any other type of atom evaluate
       to a macro. */
    if (!SYMBOL_P(op))
      return(e);
    /* Look up the symbol's binding in the global symbol table */
    while (arc_type(c, op = arc_hash_lookup(c, c->genv, op)) == T_SYMBOL)
      ;
    if (arc_type(c, op) != ARC_BUILTIN(c, S_MAC))
      return(e);			/* not a macro */
    expansion = arc_macapply(c, arc_rep(c, op), cdr(e), 0);
    /*
    if (macdebug) {
      printf("Macro: ");
      arc_print_string(c, arc_prettyprint(c, car(e)));
      printf("\nArgs: ");
      arc_print_string(c, arc_prettyprint(c, cdr(e)));
      printf("\n");
      printf("Expansion: ");
      arc_print_string(c, arc_prettyprint(c, expansion));
      printf("\n");
    }
    */
    e = expansion;
  } while (once == CTRUE);
  return(e);
}

value arc_macex(arc *c, value expr)
{
  return(macex(c, expr, CTRUE));
}

value arc_macex1(arc *c, value expr)
{
  return(macex(c, expr, CNIL));
}

static value compile_literal(arc *c, value lit, value ctx, value cont);
static value compile_ident(arc *c, value ident, value ctx, value env,
			   value cont);
static value compile_list(arc *c, value list, value ctx, value env,
			  value cont);
static value compile_continuation(arc *c, value ctx, value cont);
static int find_literal(arc *c, value ctx, value lit);

/* Given an expression nexpr, a compilation context ctx, and a continuation
   flag, return the compilation context after the expression is compiled.
   NOTE: all macros must be fully expanded before compiling! */
value arc_compile(arc *c, value nexpr, value ctx, value env, value cont)
{
  value expr, ssx;

  expr = macex(c, nexpr, CTRUE);
  if (expr == ARC_BUILTIN(c, S_T) || expr == ARC_BUILTIN(c, S_NIL)
      || expr == CNIL || expr == CTRUE
      || TYPE(expr) == T_CHAR || TYPE(expr) == T_STRING
      || TYPE(expr) == T_FIXNUM || TYPE(expr) == T_BIGNUM
      || TYPE(expr) == T_FLONUM || TYPE(expr) == T_RATIONAL
      || TYPE(expr) == T_RATIONAL || TYPE(expr) == T_COMPLEX)
    return(compile_literal(c, expr, ctx, cont));
  if (SYMBOL_P(expr)) {
    ssx = arc_ssexpand(c, expr);
    if (ssx == CNIL)
      return(compile_ident(c, expr, ctx, env, cont));
    return(arc_compile(c, ssx, ctx, env, cont));
  }
  if (CONS_P(expr))
    return(compile_list(c, expr, ctx, env, cont));
  arc_err_cstrfmt(c, "invalid_expression %p", expr);
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
  if (lits != CNIL) {
    for (i=0; i<VECLEN(lits); i++) {
      if (arc_iso(c, VINDEX(lits, i), lit))
	return(i);
    }
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

/* Find the symbol var in the environment env.  Each environment frame
   is simply a list of hash tables, each hash table key being a symbol
   name, and each value being the index inside the environment frame.
   Returns CNIL if var is a name unbound in the current environment.
   Returns CTRUE otherwise, and sets frameno to the frame number of the
   environment, and idx to the index in that environment. */
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
    arc_gcode2(c, ctx, ilde, level, offset);
  } else {
    /* If the variable is not bound in the current environment, check
       it in the global symbol table. */
    arc_gcode1(c, ctx, ildg, find_literal(c, ctx, ident));
  }
  return(compile_continuation(c, ctx, cont));
}

static value compile_if(arc *c, value args, value ctx, value env,
			value cont)
{
  int jumpaddr, jumpaddr2;

  /* If we run out of arguments, the last value becomes nil */
  if (NIL_P(args)) {
    arc_gcode(c, ctx, inil);
    return(compile_continuation(c, ctx, cont));
  }

  /* If the next is the end of the line, compile the tail end if no
     additional */
  if (NIL_P(cdr(args))) {
    return(arc_compile(c, car(args), ctx, env, cont));
  }

  /* In the final case, we have the conditional (car), the then
     portion (cadr), and the else portion (cddr). */
  /* First, compile the conditional */
  arc_compile(c, car(args), ctx, env, CNIL);
  /* this jump address will be the address of the jf instruction
     which we are about to generate.  We have to patch it with the
     address of the start of the else portion once we know it. */
  jumpaddr = FIX2INT(CCTX_VCPTR(ctx));
  /* this jf instruction has to be patched with the address of the
     else portion. */
  arc_gcode1(c, ctx, ijf, 0);
  /* compile the then portion */
  arc_compile(c, cadr(args), ctx, env, CNIL);
  /* This second jump target should be patched with the address of the
     unconditional jump at the end.  It should be patched after the else
     portion is compiled. */
  jumpaddr2 = FIX2INT(CCTX_VCPTR(ctx));
  arc_gcode1(c, ctx, ijmp, 0);
  /* patch jumpaddr so that it will jump to the address of the else
     portion which is about to be compiled */
  VINDEX(CCTX_VCODE(ctx), jumpaddr+1) = FIX2INT(CCTX_VCPTR(ctx)) - jumpaddr;
  /* the actual if portion gets compiled now */
  compile_if(c, cddr(args), ctx, env, cont);
  /* Fix the target address of the unconditional jump at the end of the
     then portion (jumpaddr2). */
  VINDEX(CCTX_VCODE(ctx), jumpaddr2+1) = FIX2INT(CCTX_VCPTR(ctx)) - jumpaddr2;
  return(compile_continuation(c, ctx, cont));
}

/* Add a new environment frame with names names to the list of
t   environments env. */
value add_env_frame(arc *c, value names, value env)
{
  value envframe;
  int idx;

  envframe = arc_mkhash(c, 8);
  for (idx=0; names; names = cdr(names), idx++) {
    arc_hash_insert(c, envframe, car(names), INT2FIX(idx));
    arc_hash_insert(c, envframe, INT2FIX(idx), car(names));
  }
  return(cons(c, envframe, env));
}

static value optarg(arc *c, value oarg, value ctx, value env, int narg,
		    value rn)
{
  int jumpaddr;
  value names, frame;

  arc_gcode1(c, ctx, imvoarg, narg);
  /* default parameters */
  if (cddr(oarg) != CNIL) {
    /* When we have an optional argument, load up whatever value
       it received from the execution of the mvoarg instruction. */
    arc_gcode2(c, ctx, ilde, 0, narg);
    /* This jt instruction is patched later.  The jump is taken when
       some value is provided for the optional argument. */
    jumpaddr = FIX2INT(CCTX_VCPTR(ctx));
    arc_gcode1(c, ctx, ijt, 0);
    /* Compile the value of the optional arg and store in the addr.
       We have to create the partial environment so that previously
       declared arguments can be referred to. */
    names = arc_list_reverse(c, rn);
    frame = add_env_frame(c, names, env);
    arc_compile(c, car(cddr(oarg)), ctx, frame, CNIL);
    arc_gcode2(c, ctx, iste, 0, narg);
    /* The jt instruction is patched with the current address */
    VINDEX(CCTX_VCODE(ctx), jumpaddr+1) = FIX2INT(CCTX_VCPTR(ctx)) - jumpaddr;
  }
  return(cadr(oarg));
}

/* To perform a destructuring bind, we attempt to traverse the args list,
   looking for every symbol in the car position.  We duplicate the
   argument for every such decision point, create a mvarg instruction,
   and go on.  Returns the list of names, and updates nargs as required.

   As is usual here, the destructuring bind compiler is heavily recursive,
   and each time it performs the following steps depending on the type of
   args:

   1. If it is a cons, check to see if either the car or cdr is nil.
      a. If both car and cdr are non-nil, duplicate the argument, and
         visit the car, generating a car instruction to visit it.  Then
         visit the cdr, generating a cdr instruction to visit it.
      b. If the car is non-nil and the cdr is nil, just visit the car (no
         need to duplicate).
      c. If the car is nil and the cdr is non-nil, just visit the cdr (no
         need to duplicate).
      d. If they are both nil, do nothing (should not normally happen in
         practice).
   2. If it is a symbol or a cons starting with the symbol o, treat it as
      a normal argument name or optional argument name as the case may be.
      This causes an argument to added to the list of names and the number
      of arguments to be incremented.
*/
static value destructuring_bind(arc *c, value args, value ctx,
				value env, int *nargs, value rn, int cflg)
{
  if (NIL_P(args))
    return(rn);

  if (SYMBOL_P(args)) {
    /* Ordinary symbol arg.  When we see this, cons it up to the list
       of names, and create a mvarg instruction for it. */
    rn = cons(c, args, rn);
    arc_gcode(c, ctx, ipush);
    arc_gcode1(c, ctx, imvarg, (*nargs)++);
    return(rn);
  }

  if (cflg && CONS_P(args) && car(args) == ARC_BUILTIN(c, S_O)) {
    /* Optional arg.  Note that this will not enforce optional args at
       the end.  Reference Arc doesn't do this either, so neither will we.
       We will consider a destructuring bind argument a possible optional
       arg only if it is the car recurstion into a destructuring bind
       expression. */
    arc_gcode(c, ctx, ipush);
    rn = cons(c, optarg(c, args, ctx, env, (*nargs)++, rn), rn);
    return(rn);
  }

  if (!CONS_P(args)) {
    arc_err_cstrfmt(c, "invalid fn arg %p", (void *)args);
    return(rn);
  }

  /* Now we have the real destructuring bind... */
  /* do nothing */
  if (NIL_P(car(args)) && NIL_P(cdr(args)))
    return(rn);
  /* visit car with a car instruction */
  if (!NIL_P(car(args)) && NIL_P(cdr(args))) {
    arc_gcode(c, ctx, icar);
    return(destructuring_bind(c, car(args), ctx, env, nargs, rn, 1));
  }

  /* visit cdr with a cdr instruction */
  if (NIL_P(car(args)) && !NIL_P(cdr(args))) {
    arc_gcode(c, ctx, icdr);
    return(destructuring_bind(c, cdr(args), ctx, env, nargs, rn, 0));
  }

  /* duplicate, then visit the car */
  arc_gcode(c, ctx, ipush);
  arc_gcode(c, ctx, icar);
  rn = destructuring_bind(c, car(args), ctx, env, nargs, rn, 1);
  /* pop the duplicated value, then visit the cdr */
  arc_gcode(c, ctx, ipop);
  arc_gcode(c, ctx, icdr);
  return(destructuring_bind(c, cdr(args), ctx, env, nargs, rn, 0));
}

static value arglist(arc *c, value args, value ctx, value env, int *nargs)
{
  value rn = CNIL, oarg;

  *nargs = 0;
  for (;;) {
    if (SYMBOL_P(car(args))) {
      /* Ordinary symbol arg.  When we see this, cons it up to the list
	 of names, and create a mvarg instruction for it. */
      rn = cons(c, car(args), rn);
      arc_gcode1(c, ctx, imvarg, (*nargs)++);
    } else if (CONS_P(car(args)) && car(car(args)) == ARC_BUILTIN(c, S_O)) {
      /* Optional arg.  Note that this will not enforce optional args at
	 the end. */
      oarg = car(args);
      rn = cons(c, optarg(c, oarg, ctx, env, (*nargs)++, rn), rn);
    } else if (CONS_P(car(args))) {
      /* For a destructuring bind, pop the argument to unbind
	 first.  This will allow subsequent instructions generated in
	 destructuring_bind to do their thing. */
      arc_gcode(c, ctx, ipop);
      rn = destructuring_bind(c, car(args), ctx, env, nargs, rn, 0);
    } else {
      arc_err_cstrfmt(c, "invalid fn arg %p", args);
      return(env);
    }

    if (SYMBOL_P(cdr(args))) {
      /* We have a rest arg here. */
      rn = cons(c, cdr(args), rn);
      arc_gcode1(c, ctx, imvrarg, (*nargs)++);
      break;
    } else if (NIL_P(cdr(args))) {
      /* done */
      break;
    }
    /* next arg */
    args = cdr(args);
  }
  /* the names in rn are reversed.  Reverse it before returning */
  return(arc_list_reverse(c, rn));
}

/* generate code to set up the new environment given the arguments. 
   After producing the code to generate the new environment, which
   generally consists of an env instruction to create an environment
   of the appropriate size and mvargs/mvoargs/mvrargs to move data from
   the stack into the appropriate environment slot as well as including
   instructions to perform any destructuring binds, return the new
   environment which includes all the names specified properly ordered.
   so that a call to find-var with the new environment can find the
   names.

   XXX - This retrieves arguments from the stack in the reverse order
   from what is expected.  Another algorithm should be devised that
   binds arguments in the proper order. */
static value compile_args(arc *c, value args, value ctx, value env)\
{
  value names, frame;
  int envinstaddr, nargs;

  /* just return the current environment if no args */
  if (args == CNIL)
    return(env);

  if (SYMBOL_P(args)) {
    /* If args is a single name, make an environment with a single
       name and a list containing the name of the sole argument. */
    frame = add_env_frame(c, cons(c, args, CNIL), env);
    arc_gcode2(c, ctx, ienv, 1, find_literal(c, ctx, frame));
    arc_gcode1(c, ctx, imvrarg, 0);
    return(frame);
  }

  if (!CONS_P(args)) {
    arc_err_cstrfmt(c, "invalid fn arg %p", args);
    return(env);
  }
  envinstaddr = FIX2INT(CCTX_VCPTR(ctx));
  /* this instruction will get patched once the true number of named
     arguments has been identified. */
  arc_gcode2(c, ctx, ienv, 0, 0);
  names = arglist(c, args, ctx, env, &nargs);
  /* patch the true number of arguments into the instruction */
  VINDEX(CCTX_VCODE(ctx), envinstaddr+1) = nargs;
  /* patch the frame */
  frame = add_env_frame(c, names, env);
  VINDEX(CCTX_VCODE(ctx), envinstaddr+2) = find_literal(c, ctx, frame);
  return(frame);
}

static value compile_fn(arc *c, value expr, value ctx, value env,
			value cont)
{
  value args, body, nctx, nenv, newcode;
  int stmts = 0;

  args = car(expr);
  body = cdr(expr);
  nctx = arc_mkcctx(c, INT2FIX(1), 0);
  nenv = compile_args(c, args, nctx, env);
  /* the body of a fn works as an implicit do/progn */
  for (; body; body = cdr(body)) {
    arc_compile(c, car(body), nctx, nenv, CNIL);
    stmts++;
  }
  /* If we have an empty list of statements add a nil */
  if (stmts == 0)
    arc_gcode(c, nctx, inil);
  compile_continuation(c, nctx, CTRUE);
  /* convert the new context into a code object and generate an
     instruction in the present context to load it as a literal,
     then create a closure using the code object and the current
     environment. */
  newcode = arc_cctx2code(c, nctx);
  arc_gcode1(c, ctx, ildl, find_literal(c, ctx, newcode));
  arc_gcode(c, ctx, icls);
  return(compile_continuation(c, ctx, cont));
}

static value compile_quote(arc *c, value expr, value ctx, value env,
			   value cont)
{
  /* compiling quotes need not be more complex... */
  arc_gcode1(c, ctx, ildl, find_literal(c, ctx, car(expr)));
  return(compile_continuation(c, ctx, cont));
}

static value qquote(arc *c, value expr, value ctx, value env)
{
  if (CONS_P(expr) && car(expr) == ARC_BUILTIN(c, S_UNQUOTE)) {
    /* to unquote something, all we need to do is evaluate it.  This
       generates a value that becomes part of the result when recombined. */
    arc_compile(c, cadr(expr), ctx, env, CNIL);
    /* do not splice */
    return(CNIL);
  }
  if (CONS_P(expr) && car(expr) == ARC_BUILTIN(c, S_UNQUOTESP)) {
    arc_compile(c, cadr(expr), ctx, env, CNIL);
    /* make it splice */
    return(CTRUE);
  }
  if (CONS_P(expr)) {
    /* If we see a cons, we need to recurse into the cdr of the
       argument first, generating the code for that, then push
       the result, then generate the code for the car of the
       argument, and then generate code to cons them together,
       or splice them together if the return so indicates. */
    qquote(c, cdr(expr), ctx, env);
    arc_gcode(c, ctx, ipush);
    if (qquote(c, car(expr), ctx, env) == CTRUE) {
      arc_gcode(c, ctx, ispl);
    } else {
      arc_gcode(c, ctx, icons);
    }
    return(CNIL);
  }
  /* all other elements are treated as literals */
  compile_literal(c, expr, ctx, CNIL);
  return(CNIL);
}

static value compile_quasiquote(arc *c, value expr, value ctx, value env, value cont)
{
  qquote(c, car(expr), ctx, env);
  return(compile_continuation(c, ctx, cont));
}

static value compile_assign(arc *c, value expr, value ctx, value env,
			    value cont)
{
  value a, val, envvar;
  int frameno, idx;

  while (expr != CNIL) {
    a = macex(c, car(expr), CTRUE);
    val = cadr(expr);
    if (a == CNIL) {
      arc_err_cstrfmt(c, "Can't rebind nil");
    } else if (a == ARC_BUILTIN(c, S_T)) {
      arc_err_cstrfmt(c, "Can't rebind t");
    } else {
      arc_compile(c, val, ctx, env, CNIL);
      envvar = find_var(c, a, env, &frameno, &idx);
      if (envvar == CTRUE) {
	arc_gcode2(c, ctx, iste, frameno, idx);
      } else {
	/* global symbol */
	arc_gcode1(c, ctx, istg, find_literal(c, ctx, a));
      }
    }
    expr = cddr(expr);
  }
  return(compile_continuation(c, ctx, cont));
}

static value (*spform(arc *c, value ident))(arc *, value, value, value, value)
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

static value compile_inline(arc *c, value inst, int narg, value expr, value ctx, value env, value cont)
{
  int count;

  expr = cdr(expr);
  for (count=0; expr; expr = cdr(expr), count++) {
    arc_compile(c, car(expr), ctx, env, CNIL);
    if (cdr(expr) != CNIL)
      arc_gcode(c, ctx, ipush);
  }
  if (count != narg) {
    arc_err_cstrfmt(c, "procedure %O expects %d arguments (%d passed)",
		    car(expr), narg, count);
  } else {
    arc_gcode(c, ctx, inst);
  }
  return(compile_continuation(c, ctx, cont));
}

#define INLINE_FUNC(name, instr, nargs) \
  static value inline_##name(arc *c, value expr, value ctx, value env, value cont) \
  { \
  return(compile_inline(c, instr, nargs, expr, ctx, env, cont)); \
  }

INLINE_FUNC(cons, iconsr, 2);
INLINE_FUNC(car, icar, 1);
INLINE_FUNC(cdr, icdr, 1);
INLINE_FUNC(scar, iscar, 2);
INLINE_FUNC(scdr, iscdr, 2);
INLINE_FUNC(is, iis, 2);

/* XXX - this trades efficiency for simplicity and generality.  There is no
   need for a base value if we have an even number of arguments. */
static value compile_inlinen(arc *c, value inst, value expr, value ctx, value env, value cont, value base)
{
  arc_compile(c, base, ctx, env, CNIL);
  for (expr = cdr(expr); expr; expr = cdr(expr)) {
    arc_gcode(c, ctx, ipush);
    arc_compile(c, car(expr), ctx, env, CNIL);
    arc_gcode(c, ctx, inst);
  }
  return(compile_continuation(c, ctx, cont));
}

static value compile_inlinen2(arc *c, value inst, value expr, value ctx, value env, value cont, value base)
{
  value xexpr, xelen;

  xexpr = cdr(expr);
  xelen = arc_list_length(c, xexpr);
  if (xelen == INT2FIX(0)) {
    arc_err_cstrfmt(c, "operator requires at least one argument");
    return(CNIL);
  } else if (xelen == INT2FIX(1)) {
    return(compile_inlinen(c, inst, expr, ctx, env, cont, base));
  }
  return(compile_inlinen(c, inst, cons(c, car(expr), cdr(xexpr)), ctx,
			 env, cont, car(xexpr)));
}

static value inline_plus(arc *c, value expr, value ctx, value env, value cont)
{
  value xexpr, xelen;

  xexpr = cdr(expr);
  xelen = arc_list_length(c, xexpr);
  if (xelen == INT2FIX(0))
    return(arc_compile(c, INT2FIX(0), ctx, env, cont));
  if (xelen == INT2FIX(1))
    return(arc_compile(c, car(xexpr), ctx, env, cont));
  return(compile_inlinen2(c, iadd, expr, ctx, env, cont, INT2FIX(0)));
}

static value inline_times(arc *c, value expr, value ctx, value env, value cont)
{
  return(compile_inlinen(c, imul, expr, ctx, env, cont, INT2FIX(1)));
}

static value inline_minus(arc *c, value expr, value ctx, value env, value cont)
{
  return(compile_inlinen2(c, isub, expr, ctx, env, cont, INT2FIX(0)));
}

static value inline_div(arc *c, value expr, value ctx, value env, value cont)
{
  return(compile_inlinen2(c, idiv, expr, ctx, env, cont, INT2FIX(1)));
}

static value (*inline_func(arc *c, value ident))(arc *, value,
						 value, value, value)
{
  if (ident == ARC_BUILTIN(c, S_CONS)) {
    return(inline_cons);
  } else if (ident == ARC_BUILTIN(c, S_CAR)) {
    return(inline_car);
  } else if (ident == ARC_BUILTIN(c, S_CDR)) {
    return(inline_cdr);
  } else if (ident == ARC_BUILTIN(c, S_SCAR)) {
    return(inline_scar);
  } else if (ident == ARC_BUILTIN(c, S_SCDR)) {
    return(inline_scdr);
  } else if (ident == ARC_BUILTIN(c, S_IS)) {
    return(inline_is);
  } else if (ident == ARC_BUILTIN(c, S_PLUS)) {
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

static value compile_apply(arc *c, value expr, value ctx, value env,
			   value cont)
{
  value fname, args, nahd;
  int contaddr, nargs;

  fname = car(expr);
  args = cdr(expr);

  /* to perform a function application, we first try to make a continuation.
     The address of the continuation will be computed later. */
  contaddr = FIX2INT(CCTX_VCPTR(ctx));
  arc_gcode1(c, ctx, icont, 0);
  nahd = args;
  /* Traverse the arguments, compiling each */
  for (nargs = 0; nahd; nahd = cdr(nahd), nargs++) {
    arc_compile(c, car(nahd), ctx, env, CNIL);
    arc_gcode(c, ctx, ipush);
  }
  /* Compile the function name, loading it to the value reg. */
  arc_compile(c, fname, ctx, env, CNIL);
  arc_gcode1(c, ctx, iapply, nargs);
  /* fix the continuation address */
  VINDEX(CCTX_VCODE(ctx), contaddr+1) = FIX2INT(CCTX_VCPTR(ctx)) - contaddr;
  return(compile_continuation(c, ctx, cont));  
}

static value fold(arc *c, value expr, value final)
{
  if (cdr(expr) == CNIL)
    return(cons(c, car(expr), final));
  return(cons(c, car(expr), cons(c, fold(c, cdr(expr), final), CNIL)));
}

static value compile_compose(arc *c, value expr, value ctx, value env,
			     value cont)
{
  value composer = cdr(car(expr));
  value cargs = cdr(expr);

  return(arc_compile(c, fold(c, composer, cargs), ctx, env, cont));
}

static value compile_complement(arc *c, value expr, value ctx, value env,
				value cont)
{
  value complemented = cdr(car(expr)), cargs = cdr(expr), result;

  if (!NIL_P(cdr(complemented))) {
    arc_err_cstrfmt(c, "complement: wrong number of arguments (%d for 1)",
		    FIX2INT(arc_len(c, complemented)));
    return(CNIL);
  }
  complemented = car(complemented);
  result = cons(c, ARC_BUILTIN(c, S_NO),
		cons(c, cons(c, complemented, cargs), CNIL));
  return(arc_compile(c, result, ctx, env, cont));
}

static value compile_andf(arc *c, value expr, value ctx, value env,
				value cont)
{
  value andfuncs, andargs, uniqs, body;

  uniqs = CNIL;
  for (andargs = cdr(expr); andargs; andargs = cdr(andargs))
    uniqs = cons(c, arc_uniq(c), uniqs);
  andargs = cdr(expr);
  body = CNIL;
  for (andfuncs = cdr(car(expr)); andfuncs; andfuncs = cdr(andfuncs)) {
    body = cons(c, cons(c, car(andfuncs), uniqs), body);
  }
  body = cons(c, cons(c, ARC_BUILTIN(c, S_AND), body), CNIL);
  body = cons(c, cons(c, ARC_BUILTIN(c, S_FN),
		      cons(c, uniqs, body)), andargs);
  return(arc_compile(c, body, ctx, env, cont));
}

static value compile_list(arc *c, value expr, value ctx, value env,
			  value cont)
{
  value (*fun)(arc *, value, value, value, value) = NULL;

  if ((fun = spform(c, car(expr))) != NULL)
    return(fun(c, cdr(expr), ctx, env, cont));

  if ((fun = inline_func(c, car(expr))) != NULL)
    return(fun(c, expr, ctx, env, cont));

  /* compose in a functional position */
  if (CONS_P(car(expr)) && car(car(expr)) == ARC_BUILTIN(c, S_COMPOSE))
    return(compile_compose(c, expr, ctx, env, cont));

  /* complement in a functional position */
  if (CONS_P(car(expr)) && car(car(expr)) == ARC_BUILTIN(c, S_COMPLEMENT))
    return(compile_complement(c, expr, ctx, env, cont));

  /* andf in a functional position */
  if (CONS_P(car(expr)) && car(car(expr)) == ARC_BUILTIN(c, S_ANDF))
    return(compile_andf(c, expr, ctx, env, cont));

  return(compile_apply(c, expr, ctx, env, cont));
}
