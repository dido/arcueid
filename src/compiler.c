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
#define CONT_NEXT 2

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

value carc_mkcode(carc *c, value vmccode, value fname, int nlits)
{
  value code = carc_mkvector(c, nlits+2);

  CODE_CODE(code) = vmccode;
  CODE_NAME(code) = fname;
  BTYPE(code) = T_CODE;
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

static void compile_expr(carc *, value, value, int);

/* A compile-time environment is basically an assoc list of symbol-index
   pairs, where the indexes are  */

/* Compile an expression at the top-level.  This returns a code object
   that can be bound into a closure, ready for execution. */
value carc_compile(carc *c, value expr, value fname)
{
  value vmccode, code;
  int len;

  literalptr = 0;
  vmcodep = tmpcode;
  compile_expr(c, expr, CNIL, CONT_RETURN);
  /* Turn the generated code into a proper T_CODE object */
  len = vmcodep - tmpcode;
  vmccode = carc_mkvmcode(c, len);
  memcpy(&VINDEX(vmccode, 0), tmpcode, len*sizeof(Inst *));
  code = carc_mkcode(c, vmccode, fname, literalptr);
  memcpy(&CODE_LITERAL(code, 0), literals, literalptr*sizeof(value));
  return(code);
}

static void (*spl_form(carc *c, value func))(carc *, value, int)
{
  return(NULL);
}

static void (*inl_func(carc *c, value func))(Inst **)
{
  return(NULL);
}

static void compile_nary(carc *c, void (*instr)(Inst **), value args, int cont)
{
}

static void compile_call(carc *c, value expr, int cont)
{
}

static void compile_ident(carc *c, value expr, int cont)
{
}


static void compile_literal(carc *c, value expr, int cont)
{
}

static void compile_expr(carc *c, value expr, value env, int cont)
{
  value func;
  void (*compile_sf)(carc *, value, int);
  void (*instr)(Inst **);

  switch (TYPE(expr)) {
  case T_CONS:
    func = car(expr);
    if (SYMBOL_P(func)) {
      /* See if it's a special form. */
      compile_sf = spl_form(c, func);
      if (compile_sf != NULL) {
	(compile_sf)(c, expr, cont);
	return;
      }

      /* See if it's an inlinable function */
      instr = inl_func(c, func);
      if (instr != NULL) {
	compile_nary(c, instr, cdr(expr), cont);
	return;
      }
    }
    compile_call(c, expr, cont);
    break;
  case T_SYMBOL:
    compile_ident(c, expr, cont);
    break;
  default:
    compile_literal(c, expr, cont);
    break;
  }
}

void carc_init_compiler(carc *c)
{
}
