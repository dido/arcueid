/* 
  Copyright (C) 2010 Rafael R. Sevilla

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
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA
  02110-1301 USA.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arcueid.h"
#include "vmengine.h"
#include "alloc.h"
#include "arith.h"

/* Expand the vmcode object of a cctx, doubling it in size.  All entries
   are copied, and the old vmcode object is freed manually.  The fit
   parameter, if true, will instead resize the vmcode to exactly the number
   of instructions which are already present. */
static value __resize_vmcode(arc *c, value cctx, int fit)
{
  value nvcode, vcode;
  int size, vptr;

  vptr = FIX2INT(CCTX_VCPTR(cctx));
  vcode = CCTX_VCODE(cctx);
  size = (fit) ? vptr : 2*VECLEN(vcode);
  nvcode = arc_mkvmcode(c, size);
  memcpy(&VINDEX(nvcode, 0), &(VINDEX(vcode, 0)), vptr*sizeof(value));
  CCTX_VCODE(cctx) = nvcode;
  c->free_block(c, (void *)vcode);
  return(nvcode);
}

/* Expand the literals object of a cctx, doubling it in size.  All entries
   are copied, and the old literals object is freed manually.  The fit
   parameter, if true, will instead resize the literals object to exactly
   the number of literals already present, as per the lptr. */
static value __resize_literals(arc *c, value cctx, int fit)
{
  value nlit, lit;
  int size, lptr;

  lptr = FIX2INT(CCTX_LPTR(cctx));
  lit = CCTX_LITS(cctx);
  size = (lit == CNIL) ? 1 : ((fit) ? lptr : 2*VECLEN(lit));
  if (size == 0)
    return(CNIL);
  nlit = arc_mkvector(c, size);
  if (lit != CNIL)
    memcpy(&VINDEX(nlit, 0), &(VINDEX(lit, 0)), lptr*sizeof(value));
  CCTX_LITS(cctx) = nlit;
  c->free_block(c, (void *)lit);
  return(nlit);
}

static value expand_vmcode(arc *c, value cctx)
{
  return(__resize_vmcode(c, cctx, 0));
}

static value fit_vmcode(arc *c, value cctx)
{
  return(__resize_vmcode(c, cctx, 1));
}

static value expand_literals(arc *c, value cctx)
{
  return(__resize_literals(c, cctx, 0));
}

#define VMCODEP(cctx) ((Inst *)(&VINDEX(VINDEX(cctx, 1), FIX2INT(VINDEX(cctx, 0)))))

void arc_gcode(arc *c, value cctx, enum vminst inst)
{
  value vcode;
  int vptr;

  vptr = FIX2INT(VINDEX(cctx, 0));
  vcode = VINDEX(cctx, 1);
  if (vptr >= VECLEN(vcode))
    vcode = expand_vmcode(c, cctx);
  VINDEX(vcode, vptr++) = (value)inst;
  VINDEX(cctx, 0) = INT2FIX(vptr);
}

void arc_gcode1(arc *c, value cctx, enum vminst inst, value arg)
{
  value vcode;
  int vptr;

  vptr = FIX2INT(VINDEX(cctx, 0));
  vcode = VINDEX(cctx, 1);
  if (vptr+1 >= VECLEN(vcode))
    vcode = expand_vmcode(c, cctx);
  VINDEX(vcode, vptr++) = (value)inst;
  VINDEX(vcode, vptr++) = arg;
  VINDEX(cctx, 0) = INT2FIX(vptr);
}

void arc_gcode2(arc *c, value cctx, enum vminst inst, value arg1, value arg2)
{
  value vcode;
  int vptr;

  vptr = FIX2INT(VINDEX(cctx, 0));
  vcode = VINDEX(cctx, 1);
  if (vptr+2 >= VECLEN(vcode))
    vcode = expand_vmcode(c, cctx);
  VINDEX(vcode, vptr++) = (value)inst;
  VINDEX(vcode, vptr++) = arg1;
  VINDEX(vcode, vptr++) = arg2;
  VINDEX(cctx, 0) = INT2FIX(vptr);
}

/* Add a literal, growing the literal vector as needed */
int arc_literal(arc *c, value cctx, value literal)
{
  int lptr, lidx;
  value lits;

  lptr = FIX2INT(CCTX_LPTR(cctx));
  lits = CCTX_LITS(cctx);
  if (lptr >= VECLEN(lits))
    lits = expand_literals(c, cctx);
  lidx = lptr;
  VINDEX(lits, lptr++) = (value)literal;
  CCTX_LPTR(cctx) = INT2FIX(lptr);
  return(lidx);
}

value arc_mkvmcode(arc *c, int length)
{
  value code = arc_mkvector(c, length);

  BTYPE(code) = T_VMCODE;
  return(code);
}

/* create code and reserve space for nlits */
value arc_mkcode(arc *c, value vmccode, int nlits)
{
  value code = arc_mkvector(c, nlits+1);

  CODE_CODE(code) = vmccode;
  BTYPE(code) = T_CODE;
  return(code);
}

value arc_cctx2code(arc *c, value cctx)
{
  value func;
  int i;

  fit_vmcode(c, cctx);
  func = arc_mkcode(c, CCTX_VCODE(cctx), CCTX_LPTR(cctx));
  for (i=0; i<CCTX_LPTR(cctx); i++)
    CODE_LITERAL(func, i) = VINDEX(CCTX_LITS(cctx), i);
  return(func);
}

value arc_mkccode(arc *c, int argc, value (*cfunc)())
{
  value code;

  code = c->get_cell(c);
  BTYPE(code) = T_CCODE;
  REP(code)._cfunc.fnptr = cfunc;
  REP(code)._cfunc.argc = argc;
  return(code);
}

value arc_mkcctx(arc *c, value vcodesize, value vlitsize)
{
  value cctx;
  int codesize = FIX2INT(vcodesize), litsize = FIX2INT(vlitsize);

  cctx = arc_mkvector(c, 4);
  CCTX_LPTR(cctx) = CCTX_VCPTR(cctx) = INT2FIX(0);
  CCTX_VCODE(cctx) = arc_mkvmcode(c, codesize);
  if (litsize == 0)
    CCTX_LITS(cctx) = CNIL;
  else
    CCTX_LITS(cctx) = arc_mkvector(c, litsize);
  return(cctx);
}

value arc_vcptr(arc *c, value cctx)
{
  return(CCTX_VCPTR(cctx));
}
