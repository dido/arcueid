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
#ifndef _VMENGINE_H_

#define _VMENGINE_H_

#ifdef __GNUC__
typedef void *Label;
typedef Label Inst;

#else

typedef long Label;
typedef long Inst;

#endif

extern Inst *vm_prim;

extern void gen_inst(Inst **vmcodepp, Inst i);
extern void genarg_i(Inst **vmcodepp, value i);
extern void genarg_target(Inst **vmcodepp, Inst *target);

#endif
