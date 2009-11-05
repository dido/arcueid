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
#include "carc.h"
#include "vmengine.h"
#include "alloc.h"
#include "arith.h"
#include "carcvm-gen.i"

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
  CODE_NAME(code) = fname;
  CODE_ARGS(code) = args;
  BTYPE(code) = T_CODE;
  return(code);
}
