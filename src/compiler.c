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
#include "carcvm-gen.i"

#define CONT_RETURN 1
#define CONT_NEXT 3

void gen_inst(Inst **vmcodepp, Inst i)
{
  **vmcodepp = i;
  (*vmcodepp)++;
}

void genarg_i(Inst **vmcodepp, value i)
{
  *((value *) *vmcodepp) = i;
  (*vmcodepp)++;
}

void genarg_target(Inst **vmcodepp, Inst *target)
{
  *((Inst **) *vmcodepp) = target;
  (*vmcodepp)++;
}

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

/* XXX: This static array of instructions is the maximum that a single
   s-expression can produce.  This should probably go away sometime for
   something a bit more dynamic, but for now I think this should be
   alright.  Once all the code has been generated here, it gets copied
   off to the actual code object. */
#define MAX_CODELEN 65536
static Inst tmpcode[MAX_CODELEN];
static Inst *vmcodep;

/* Similarly for compile-time literals.  This should be used to fill
   in the literals vector. */
#define MAX_LITERALS 4096
static value literals[MAX_LITERALS];
static int literalptr;

/* This is a list of offsets for all jump instructions that require
   relocation.  Jump instructions are generated with an absolute
   offset of 0, so the base address of the code block into which the
   instructions are copied must be addred to all instructions. */
static value reloc;

static void compile_expr(carc *, value, value, value);

/* Compile an expression at the top-level.  This returns a closure object. */
value carc_compile(carc *c, value expr, value env, value fname)
{
  value vmccode, code;
  int len;

  literalptr = 0;
  vmcodep = tmpcode;
  reloc = CNIL;
  compile_expr(c, expr, env, CONT_RETURN);
  /* Turn the generated code into a proper T_CODE object */
  len = vmcodep - tmpcode;
  vmccode = carc_mkvmcode(c, len);
  memcpy(&VINDEX(vmccode, 0), tmpcode, len*sizeof(Inst *));
  /* Perform relocation fixups for compiled jumps */
  while (reloc != CNIL) {
    VINDEX(vmccode, FIX2INT(car(reloc))) += (value)&VINDEX(vmccode, 0);
    reloc = cdr(reloc);
  }
  code = carc_mkcode(c, vmccode, fname, CNIL, literalptr);
  memcpy(&CODE_LITERAL(code, 0), literals, literalptr*sizeof(value));
  return(carc_mkclosure(c, code, CNIL));
}

static void compile_continuation(carc *c, value cont)
{
  switch (cont) {
  case CONT_RETURN:
    gen_ret(&vmcodep);
    break;
  case CONT_NEXT:
    break;
  }
}

static void (*spl_form(carc *c, value func))(carc *, value, value, value)
{
  return(NULL);
}

static void (*inl_func(carc *c, value func))(Inst **)
{
  value inlf;

  inlf = carc_hash_lookup(c, c->inlfuncs, func);
  if (inlf == CNIL)
    return(NULL);
  return((void (*)(Inst **))(REP(inlf)._cfunc.fnptr));
}

static void compile_nary(carc *c, void (*instr)(Inst **), value args,
			 value env, value cont)
{
}

static void compile_call(carc *c, value expr, value env, value cont)
{
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
static int find_literal(carc *c, value lit)
{
  int i;

  for (i=0; i<literalptr; i++) {
    if (carc_equal(c, literals[i], lit) == CTRUE)
      return(i);
  }
  /* Not found, add it */
  literals[literalptr] = lit;
  return(literalptr++);
}

static void compile_ident(carc *c, value sym, value env, value cont)
{
  int level, offset;

  /* First, try to look through the environment to see if the symbol
     is represented there, and if it is generate an instruction to
     load it.  If not, generate an instruction to try to load the
     symbol from the global environment. */
  if (find_var(c, sym, env, &level, &offset))
    gen_lde(&vmcodep, level, offset);
  else
    gen_ldg(&vmcodep, find_literal(c, sym));
  compile_continuation(c, cont);
}

/* Compile a literal */
static void compile_literal(carc *c, value lit, value cont)
{
  switch (TYPE(lit)) {
  case T_NIL:
    gen_nil(&vmcodep);
    break;
  case T_TRUE:
    gen_true(&vmcodep);
    break;
  case T_FIXNUM:
    gen_ldi(&vmcodep, lit);
    break;
  default:
    /* anything else, look it up (or put it in) the literal table and
       generate an ldl */
    gen_ldl(&vmcodep, find_literal(c, lit));
    break;
  }
  compile_continuation(c, cont);
}

static void compile_expr(carc *c, value expr, value env, value cont)
{
  value func;
  void (*compile_sf)(carc *, value, value, value);
  void (*instr)(Inst **);

  switch (TYPE(expr)) {
  case T_CONS:
    func = car(expr);
    if (SYMBOL_P(func)) {
      /* See if it's a special form. */
      compile_sf = spl_form(c, func);
      if (compile_sf != NULL) {
	(compile_sf)(c, expr, env, cont);
	return;
      }

      /* See if it's an inlinable n-ary function */
      instr = inl_func(c, func);
      if (instr != NULL) {
	compile_nary(c, instr, cdr(expr), env, cont);
	return;
      }
    }
    compile_call(c, expr, env, cont);
    break;
  case T_SYMBOL:
    compile_ident(c, expr, env, cont);
    break;
  default:
    compile_literal(c, expr, cont);
    break;
  }
}

value compile_fn(carc *c, value expr, value env, value cont)
{
  return(CNIL);
}

value compile_if(carc *c, value expr, value env, value cont)
{
  return(CNIL);
}

value compile_quasiquote(carc *c, value expr, value env, value cont)
{
  return(CNIL);
}

value compile_quote(carc *c, value expr, value env, value cont)
{
  return(CNIL);
}

value compile_set(carc *c, value expr, value env, value cont)
{
  return(CNIL);
}

static struct {
  char *name;
  value (*sfcompiler)();
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
