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
  License along with this library; if not,  see <http://www.gnu.org/licenses/>.
*/

/* Arcueid makes use of two distinct objects for code, a compilation context
   object and an actual code object.  Compilation contexts are just vectors
   that conform to a specific format and are not in any way treated specially
   by the system: they aren't even marked as a distinct object type.  They can
   be transformed into T_CODE objects that are, though. */
#include <string.h>
#include "arcueid.h"
#include "vmengine.h"

/* Create an empty code generation context. This is just a plain vector */
value arc_mkcctx(arc *c)
{
  value cctx;

  cctx = arc_mkvector(c, 4);
  CCTX_VCPTR(cctx) = CCTX_LITS(cctx) = INT2FIX(0);
  CCTX_VCODE(cctx) = CCTX_LITS(cctx) = CNIL;
  return(cctx);
}

/* Expand the vmcode object of a code object, doubling it in size.  All
   entries are copied. */
static value __resize_vmcode(arc *c, value cctx)
{
  value nvcode, vcode;
  int size, vptr;

  vptr = FIX2INT(CCTX_VCPTR(cctx));
  vcode = CCTX_VCODE(cctx);
  size = (vcode == CNIL) ? 16 : (2*VECLEN(vcode));
  nvcode = arc_mkvector(c, size);
  memcpy(&VINDEX(nvcode, 0), &(VINDEX(vcode, 0)), vptr*sizeof(value));
  CCTX_VCODE(cctx) = nvcode;
  return(nvcode);
}

/* Expand the literals object of a cctx, doubling it in size.  All entries
   are copied. */
static value __resize_literals(arc *c, value cctx)
{
  value nlit, lit;
  int size, lptr;

  lptr = FIX2INT(CCTX_LPTR(cctx));
  lit = CCTX_LITS(cctx);
  size = (lit == CNIL) ? 16 : (2*VECLEN(lit));
  if (size == 0)
    return(CNIL);
  nlit = arc_mkvector(c, size);
  memcpy(&VINDEX(nlit, 0), &(VINDEX(lit, 0)), lptr*sizeof(value));
  CCTX_LITS(cctx) = nlit;
  return(nlit);
}

#define VMCODEP(cctx) ((Inst *)(&VINDEX(VINDEX(cctx, 1), FIX2INT(VINDEX(cctx, 0)))))

void arc_emit(arc *c, value cctx, enum vminst inst)
{
  value vcode;
  int vptr;

  vptr = FIX2INT(CCTX_VCPTR(cctx));
  vcode = CCTX_VCODE(cctx);
  if (NIL_P(vcode) || vptr >= VECLEN(vcode))
    vcode = __resize_vmcode(c, cctx);
  VINDEX(vcode, vptr++) = INT2FIX((int)inst);
  CCTX_VCPTR(cctx) = INT2FIX(vptr);
}

void arc_emit1(arc *c, value cctx, enum vminst inst, value arg)
{
  value vcode;
  int vptr;

  vptr = FIX2INT(CCTX_VCPTR(cctx));
  vcode = CCTX_VCODE(cctx);
  if (NIL_P(vcode) || vptr+1 >= VECLEN(vcode))
    vcode = __resize_vmcode(c, cctx);
  VINDEX(vcode, vptr++) = INT2FIX((int)inst);
  VINDEX(vcode, vptr++) = arg;
  CCTX_VCPTR(cctx) = INT2FIX(vptr);
}

void arc_emit2(arc *c, value cctx, enum vminst inst, value arg1, value arg2)
{
  value vcode;
  int vptr;

  vptr = FIX2INT(CCTX_VCPTR(cctx));
  vcode = CCTX_VCODE(cctx);
  if (NIL_P(vcode) || vptr+2 >= VECLEN(vcode))
    vcode = __resize_vmcode(c, cctx);
  VINDEX(vcode, vptr++) = INT2FIX((int)inst);
  VINDEX(vcode, vptr++) = arg1;
  VINDEX(vcode, vptr++) = arg2;
  CCTX_VCPTR(cctx) = INT2FIX(vptr);
}

void arc_emit3(arc *c, value cctx, enum vminst inst, value arg1, value arg2,
	       value arg3)
{
  value vcode;
  int vptr;

  vptr = FIX2INT(CCTX_VCPTR(cctx));
  vcode = CCTX_VCODE(cctx);
  if (NIL_P(vcode) || vptr+3 >= VECLEN(vcode))
    vcode = __resize_vmcode(c, cctx);
  VINDEX(vcode, vptr++) = INT2FIX((int)inst);
  VINDEX(vcode, vptr++) = arg1;
  VINDEX(vcode, vptr++) = arg2;
  VINDEX(vcode, vptr++) = arg3;
  CCTX_VCPTR(cctx) = INT2FIX(vptr);
}

/* Patch a jump offset given the address of the jump instruction
   and the destination offset. */
void arc_jmpoffset(arc *c, value cctx, int jmpinst, int destoffset)
{
  VINDEX(CCTX_VCODE(cctx), jmpinst+1) = INT2FIX(destoffset - jmpinst);
}

/* Add a literal, growing the literal vector as needed */
int arc_literal(arc *c, value cctx, value literal)
{
  int lptr, lidx;
  value lits;

  lptr = FIX2INT(CCTX_LPTR(cctx));
  lits = CCTX_LITS(cctx);
  if (lits == CNIL || lptr >= VECLEN(lits))
    lits = __resize_literals(c, cctx);
  lidx = lptr;
  VINDEX(lits, lptr++) = (value)literal;
  CCTX_LPTR(cctx) = INT2FIX(lptr);
  return(lidx);
}

static AFFDEF(code_pprint)
{
  AARG(sexpr, disp, fp);
  AOARG(visithash);
  AVAR(dw, wc);
  value src;
  AFBEGIN;
  (void)visithash;
  (void)disp;
  AV(dw) = arc_mkaff(c, __arc_disp_write, CNIL);
  AV(wc) = arc_mkaff(c, arc_writec, CNIL);
  AFCALL(AV(dw), arc_mkstringc(c, "#<procedure"), CTRUE, AV(fp), AV(visithash));
  src = CODE_SRC(AV(sexpr));
  if (!NIL_P(src)) {
    value fname;

    AFCALL(AV(wc), arc_mkchar(c, ':'), AV(fp));
    AFCALL(AV(wc), arc_mkchar(c, ' '), AV(fp));
    src = CODE_SRC(AV(sexpr));
    fname = arc_hash_lookup(c, src, INT2FIX(SRC_FUNCNAME));
    AFCALL(AV(dw), fname, CTRUE, AV(fp), AV(visithash));
  }
  AFCALL(AV(wc), arc_mkchar(c, '>'), AV(fp));
  ARETURN(CNIL);
  AFEND;
}
AFFEND

value arc_mkcode(arc *c, int ncodes, int nlits)
{
  value code = arc_mkvector(c, nlits+2);

  CODE_CODE(code) = arc_mkvector(c, ncodes);
  CODE_SRC(code) = CNIL;
  ((struct cell *)code)->_type = T_CODE;
  return(code);
}

value arc_code_setsrc(arc *c, value code, value src)
{
  CODE_SRC(code) = src;
  return(code);
}

value arc_code_setname(arc *c, value code, value name)
{
  value orgcode = code;

  if (TYPE(code) == T_CLOS) {
    code = CLOS_CODE(code);
  } else if (!TYPE(code) == T_CODE) {
    arc_err_cstrfmt(c, "invalid argument for arc-code-setname");
    return(CNIL);
  }
  if (NIL_P(CODE_SRC(code))) {
    CODE_SRC(code) = arc_mkhash(c, ARC_HASHBITS);
  }
  arc_hash_insert(c, CODE_SRC(code), INT2FIX(SRC_FUNCNAME), name);
  return(orgcode);
}

value arc_cctx2code(arc *c, value cctx)
{
  value func;

  func = arc_mkcode(c, CCTX_VCPTR(cctx), CCTX_LPTR(cctx));
  memcpy(&VINDEX(CODE_CODE(func), 0), &VINDEX(CCTX_VCODE(cctx), 0),
	 FIX2INT(CCTX_VCPTR(cctx))*sizeof(value));
  memcpy(&CODE_LITERAL(func, 0), &VINDEX(CCTX_LITS(cctx), 0),
	 FIX2INT(CCTX_LPTR(cctx))*sizeof(value));
  return(func);
}

typefn_t __arc_code_typefn__ = {
  __arc_vector_marker,
  __arc_null_sweeper,
  code_pprint,
  NULL,
  NULL,
  __arc_vector_isocmp,
  /* Note a T_CODE object cannot be directly applied.  It has to be
     turned into a closure first. */
  NULL,
  NULL
};
