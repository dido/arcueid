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
#ifndef _VMENGINE_H_

#define _VMENGINE_H_

enum vminst {
  inop=0,
  ipush=1,
  ipop=2,
  ildl=67,
  ildi=68,
  ildg=69,
  istg=70,
  ilde=135,
  iste=136,
  imvarg=73,
  imvoarg=74,
  imvrarg=75,
  icont=76,
  ienv=141,
  iapply=78,
  iret=15,
  ijmp=80,
  ijt=81,
  ijf=82,
  itrue=19,
  inil=20,
  ihlt=21,
  iadd=22,
  isub=23,
  imul=24,
  idiv=25,
  icons=26,
  icar=27,
  icdr=28,
  iscar=29,
  iscdr=30,
  ispl=31,
  iis=32,
  iiso=33,
  igt=34,
  ilt=35,
  idup=36,
  icls=37,
  iconsr=38
};

extern void arc_gcode(arc *c, value cctx, enum vminst inst);
extern void arc_gcode1(arc *c, value cctx, enum vminst inst, value arg);
extern void arc_gcode2(arc *c, value cctx, enum vminst inst, value arg1,
		       value arg2);
extern int arc_literal(arc *c, value cctx, value literal);
extern value arc_cctx2code(arc *c, value cctx);
extern value *arc_disasm_inst(arc *c, int index, value *codeptr);

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


#endif
