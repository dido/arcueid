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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "carc.h"
#include "vmengine.h"
#include "alloc.h"
#include "arith.h"

#define CONT_RETURN 1
#define CONT_NEXT 3

/* A code generation context (cctx) is a vector with the following
   items as indexes:

   0. A code pointer into the vmcode object (usually a fixnum)
   1. A vmcode object.
   2. A pointer into the literal vector (usually a fixnum)
   3. A vector of literals

   The following functions are intended to manage the data
   structure, and to generate code and literals for the system.
 */

#define CCTX_VCPTR(cctx) (VINDEX(cctx, 0))
#define CCTX_VCODE(cctx) (VINDEX(cctx, 1))
#define CCTX_LPTR(cctx) (VINDEX(cctx, 2))
#define CCTX_LITS(cctx) (VINDEX(cctx, 3))

/* Expand the vmcode object of a cctx, doubling it in size.  All entries
   are copied, and the old vmcode object is freed manually.  The fit
   parameter, if true, will instead resize the cctx to exactly the number
   of instructions which are already present. */
static value __resize_vmcode(carc *c, value cctx, int fit)
{
  value nvcode, vcode;
  int size, vptr;

  vptr = FIX2INT(VINDEX(cctx, 0));
  vcode = VINDEX(cctx, 1);
  size = (fit) ? vptr : 2*VECLEN(vcode);
  nvcode = carc_mkvmcode(c, size);
  memcpy(&VINDEX(nvcode, 0), &(VINDEX(vcode, 0)), vptr*sizeof(value));
  VINDEX(cctx, 1) = nvcode;
  c->free_block(c, (void *)vcode);
  return(nvcode);
}

static value expand_vmcode(carc *c, value cctx)
{
  return(__resize_vmcode(c, cctx, 0));
}

static value fit_vmcode(carc *c, value cctx)
{
  return(__resize_vmcode(c, cctx, 1));
}

#define VMCODEP(cctx) ((Inst *)(&VINDEX(VINDEX(cctx, 1), FIX2INT(VINDEX(cctx, 0)))))

static void gcode(carc *c, value cctx, void (*igen)(Inst **))
{
  value vcode;
  int vptr;
  Inst *vmcodep;

  vptr = FIX2INT(VINDEX(cctx, 0));
  vcode = VINDEX(cctx, 1);
  if (vptr >= VECLEN(vcode))
    vcode = expand_vmcode(c, cctx);
  vmcodep = (Inst *)(&VINDEX(vcode, vptr));
  igen(&vmcodep);
  vptr++;
  VINDEX(cctx, 0) = INT2FIX(vptr);
}

static void gcode1(carc *c, value cctx, void (*igen)(Inst **, value),
		   value arg)
{
  value vcode;
  int vptr;
  Inst *vmcodep;

  vptr = FIX2INT(VINDEX(cctx, 0));
  vcode = VINDEX(cctx, 1);
  if (vptr+1 >= VECLEN(vcode))
    vcode = expand_vmcode(c, cctx);
  vmcodep = (Inst *)(&VINDEX(vcode, vptr));
  igen(&vmcodep, arg);
  vptr += 2;
  VINDEX(cctx, 0) = INT2FIX(vptr);
}

static void gcode2(carc *c, value cctx,
			  void (*igen)(Inst **, value, value),
			  value arg1, value arg2)
{
  value vcode;
  int vptr;
  Inst *vmcodep;

  vptr = FIX2INT(VINDEX(cctx, 0));
  vcode = VINDEX(cctx, 1);
  if (vptr+2 >= VECLEN(vcode))
    vcode = expand_vmcode(c, cctx);
  vmcodep = (Inst *)(&VINDEX(vcode, vptr));
  igen(&vmcodep, arg1, arg2);
  vptr += 2;
  VINDEX(cctx, 0) = INT2FIX(vptr);
}

static void gen_inst(Inst **vmcodepp, Inst i)
{
  **vmcodepp = i;
  (*vmcodepp)++;
}

static void genarg_i(Inst **vmcodepp, value i)
{
  *((value *) *vmcodepp) = i;
  (*vmcodepp)++;
}

/*
static void genarg_target(Inst **vmcodepp, Inst *target)
{
  *((Inst **) *vmcodepp) = target;
  (*vmcodepp)++;
}
*/

#include "carcvm-gen.i"

value carc_mkvmcode(carc *c, int length)
{
  value code = carc_mkvector(c, length);

  BTYPE(code) = T_VMCODE;
  return(code);
}

value carc_mkcode(carc *c, value vmccode, value fname, value args, int nlits)
{
  value code = carc_mkvector(c, nlits+3);

  CODE_CODE(code) = vmccode;
  CODE_ARGS(code) = args;
  CODE_NAME(code) = fname;
  BTYPE(code) = T_CODE;
  return(code);
}

value carc_mkccode(carc *c, int argc, value (*cfunc)())
{
  value code;

  code = c->get_cell(c);
  BTYPE(code) = T_CCODE;
  REP(code)._cfunc.fnptr = cfunc;
  REP(code)._cfunc.argc = argc;
  return(code);
}

static void compile_expr(carc *, value, value, value, value);

#define INITIAL_VMCODE_SIZE 128 /* Initial number of instruction slots in
				   the vmcode */
#define INITIAL_LITERAL_SIZE 16	/* Initial number of literal slots */

/* Compile an expression.  This returns a closure object. */
value carc_compile(carc *c, value expr, value env, value fname)
{
  value cctx, code;
  int nlits;

  /* create a new code context */
  cctx = carc_mkvector(c, 4);
  VINDEX(cctx, 0) = INT2FIX(0);
  VINDEX(cctx, 1) = carc_mkvmcode(c, INITIAL_VMCODE_SIZE);
  VINDEX(cctx, 2) = INT2FIX(0);
  VINDEX(cctx, 3) = carc_mkvector(c, INITIAL_LITERAL_SIZE);
  compile_expr(c, cctx, expr, env, CONT_RETURN);

  /* Turn the code context into a closure */
  nlits = FIX2INT(VINDEX(cctx, 2));
  code = carc_mkcode(c, fit_vmcode(c, cctx), fname, CNIL, nlits);
  memcpy(&CODE_LITERAL(code, 0), &VINDEX(VINDEX(cctx, 3), 0), nlits*sizeof(value));
  return(carc_mkclosure(c, code, env));
}

static void compile_continuation(carc *c, value cctx, value cont)
{
  switch (cont) {
  case CONT_RETURN:
    gcode(c, cctx, gen_ret);
     break;
  case CONT_NEXT:
    break;
  }
}

/* Find a variable in the environment.  This returns 0 or 1 depending on
   whether the variable was found or not, and the pointers to level and
   offset are set accordingly. */
static int find_var(carc *c, value sym, value env, int *plev, int *poff)
{
  int level, offset;
  value e, a;

  for (e = env, level = 0; ENV_P(e); e = cdr(e), ++level) {
    for (a = ENV_NAMES(e), offset=0; CONS_P(a); a = cdr(a), ++offset) {
      if (sym == car(a)) {
	*plev = level;
	*poff = offset;
	return(1);
      }
    }
  }
  return(0);
}

/* Return the index into the literal table for the literal lit, or
   add the literal into the table if it's not already there, and return
   its offset. */
static int find_literal(carc *c, value cctx, value lit)
{
  int i, literalptr;

  literalptr = FIX2INT(CCTX_LPTR(cctx));
  for (i=0; i<literalptr; i++) {
    if (carc_equal(c, VINDEX(CCTX_LITS(cctx), i), lit) == CTRUE)
      return(i);
  }
  /* Not found, add it */
  if (VECLEN(CCTX_LITS(cctx)) >= literalptr) {
    /* Alloc new, copy, and free old */
    value nlitvect = carc_mkvector(c, 2*literalptr);

    memcpy(&VINDEX(nlitvect, 0), &VINDEX(CCTX_LITS(cctx), 0),
	   literalptr*sizeof(value));
    c->free_block(c, (void *)CCTX_LITS(cctx));
    CCTX_LITS(cctx) = nlitvect;
  }
  VINDEX(CCTX_LITS(cctx), literalptr) = lit;
  CCTX_LPTR(cctx) = INT2FIX(literalptr+1);
  return(literalptr++);
}

static void compile_ident(carc *c, value cctx, value sym, value env,
			  value cont)
{
  int level, offset;

  /* First, try to look through the environment to see if the symbol
     is represented there, and if it is generate an instruction to
     load it.  If not, generate an instruction to try to load the
     symbol from the global environment. */
  if (find_var(c, sym, env, &level, &offset))
    gcode2(c, cctx, gen_lde, level, offset);
  else
    gcode1(c, cctx, gen_ldg, find_literal(c, cctx, sym));
  compile_continuation(c, cctx, cont);
}

/* Compile a literal */
static void compile_literal(carc *c, value cctx, value lit, value cont)
{
  switch (TYPE(lit)) {
  case T_NIL:
    gcode(c, cctx, gen_nil);
    break;
  case T_TRUE:
    gcode(c, cctx, gen_true);
    break;
  case T_FIXNUM:
    gcode1(c, cctx, gen_ldi, lit);
    break;
  default:
    /* anything else, look it up (or put it in) the literal table and
       generate an ldl */
    gcode1(c, cctx, gen_ldl, find_literal(c, cctx, lit));
    break;
  }
  compile_continuation(c, cctx, cont);
}

static value (*spl_form(carc *c, value func))()
{
  value sf;

  sf = carc_hash_lookup(c, c->splforms, func);
  if (sf == CUNBOUND || sf == CNIL)
    return(NULL);
  return(REP(sf)._cfunc.fnptr);
}

static void compile_expr(carc *c, value cctx, value expr,
			 value env, value cont)
{
  value func;
  value (*compile_sf)();

  switch (TYPE(expr)) {
  case T_CONS:
    func = car(expr);
    if (SYMBOL_P(func)) {
      /* Special forms */
      compile_sf = spl_form(c, func);
      if (compile_sf != NULL) {
	(compile_sf)(c, cctx, expr, env, cont);
	return;
      }
      /* XXX: See about inlinable functions */
    }
    /* XXX: see about function calls */
    break;
  case T_SYMBOL:
    compile_ident(c, cctx, expr, env, cont);
    break;
  default:
    compile_literal(c, cctx, expr, cont);
    break;
  }
}

static value compile_fn(carc *c, value cctx, value expr, value env, value cont)
{
  value args, body;

  args = cadr(expr);
  body = caddr(expr);
  return(CNIL);
}

static void compile_if_core(carc *c, value cctx, value ifbody, value env,
			    value cont, int nested)
{
  value cond, ifpart, elsepart;
  Inst *jump_addr, *jump_addr2;

  /* On entering this function, the car of ifbody should be the condition.
     cadr is the then portion, and caddr is the else portion. */
  if (ifbody == CNIL) {
    gcode(c, cctx, gen_nil);
    compile_continuation(c, cctx, cont);
    return;
  }
  if (!CONS_P(ifbody)) {
    compile_continuation(c, cctx, cont);
    return;
  }
  cond = car(ifbody);
  if (CONS_P(cdr(ifbody))) {
    ifpart = cadr(ifbody);
    elsepart = cddr(ifbody);
  } else {
    ifpart = CNIL;
    elsepart = CNIL;
  }

  /* Now, try to compile the cond. */
  compile_expr(c, cctx, cond, env, CONT_NEXT);
  /* Add a conditional jump so we can jump over the ifpart to the
     elsepart.  We need to patch this later once the elsepart
     target address is known. */
  jump_addr = VMCODEP(cctx);
  /* If this is a supposed nested statement and the branches
     are all empty, we are done */
  if (nested && ifpart == CNIL && elsepart == CNIL) {
    compile_continuation(c, cctx, cont);
    return;
  }
  gcode1(c, cctx, gen_jf, 0);
  compile_expr(c, cctx, ifpart, env, CONT_NEXT);
  /* Generate an unconditional jump so that we jump over the else portion */
  jump_addr2 = VMCODEP(cctx);
  gcode1(c, cctx, gen_jmp, 0);
  /* The address of the else portion is here, jump to this location */
  *(jump_addr+1) = (Inst)(VMCODEP(cctx) - jump_addr);
  /* Recurse to compile the else portion */
  compile_if_core(c, cctx, elsepart, env, cont, 1);
  /* Fix the target address of the jump over the else portion */
  *(jump_addr2+1) = (Inst)(VMCODEP(cctx) - jump_addr2);
  /* Finished */
  compile_continuation(c, cctx, cont);
}

static value compile_if(carc *c, value cctx, value expr, value env, value cont)
{
  /* dump the 'if */
  compile_if_core(c, cctx, cdr(expr), env, cont, 0);
  return(CNIL);
}

value compile_quasiquote(carc *c, value cctx, value expr, value env, value cont)
{
  return(CNIL);
}

value compile_quote(carc *c, value cctx, value expr, value env, value cont)
{
  return(CNIL);
}

value compile_set(carc *c, value cctx, value expr, value env, value cont)
{
  return(CNIL);
}

static struct {
  char *name;
  value (*sfcompiler)(carc *, value, value, value, value);
} splformtbl[] = {
  { "fn", compile_fn },
  { "if", compile_if },
  { "quasiquote", compile_quasiquote },
  { "quote", compile_quote },
  { "set", compile_set },
  { NULL, NULL }
};

/* This table is still incomplete */
static struct {
  char *name;
  void (*emitfn)(Inst **);
} inl_functbl[] = {
  { "+", gen_add },
  { "-", gen_sub },
  { "*", gen_mul },
  { "/", gen_div },
  { "cons", gen_cons },
  { "car", gen_car },
  { "cdr", gen_cdr },
  { "scar", gen_scar },
  { "scdr", gen_scdr },
  { "is", gen_is },
  { NULL, NULL }
};

void carc_init_compiler(carc *c)
{
  int i;

  c->splforms = carc_mkhash(c, 4);
  for (i=0; splformtbl[i].name != NULL; i++) {
    carc_hash_insert(c, c->splforms,
		     carc_intern(c, carc_mkstringc(c, splformtbl[i].name)),
		     carc_mkccode(c, 4, splformtbl[i].sfcompiler));
  }

  c->inlfuncs = carc_mkhash(c, 4);
  for (i=0; inl_functbl[i].name != NULL; i++) {
    carc_hash_insert(c, c->inlfuncs,
		     carc_intern(c, carc_mkstringc(c, inl_functbl[i].name)),
		     carc_mkccode(c, 0, (value (*)())inl_functbl[i].emitfn));
  }

}
