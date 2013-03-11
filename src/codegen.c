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

/* A code generation context (cctx) is a vector with the following
   items as indexes:

   0. A code pointer into the vmcode object (usually a fixnum)
   1. A vmcode object.
   2. A pointer into the literal vector (usually a fixnum)
   3. A vector of literals

   The following macros are intended to manage the data
   structure, and to generate code and literals for the
   system.
 */

#define CCTX_VCPTR(cctx) (VINDEX(cctx, 0))
#define CCTX_VCODE(cctx) (VINDEX(cctx, 1))
#define CCTX_LPTR(cctx) (VINDEX(cctx, 2))
#define CCTX_LITS(cctx) (VINDEX(cctx, 3))

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
  if (vptr >= VECLEN(vcode))
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
  if (vptr+1 >= VECLEN(vcode))
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
  if (vptr+2 >= VECLEN(vcode))
    vcode = __resize_vmcode(c, cctx);
  VINDEX(vcode, vptr++) = INT2FIX((int)inst);
  VINDEX(vcode, vptr++) = arg1;
  VINDEX(vcode, vptr++) = arg2;
  CCTX_VCPTR(cctx) = INT2FIX(vptr);
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

#if 0
static value code_pprint(arc *c, value sexpr, value *ppstr, value visithash)
{
  value src;
  __arc_append_cstring(c, "#<procedure: ", ppstr);

  src = CODE_SRC(sexpr);
  if (NIL_P(src)) {
    __arc_append_cstring(c, "(anonymous)", ppstr);
  } else {
    value fname = arc_hash_lookup(c, src, INT2FIX(SRC_FUNCNAME));

    arc_prettyprint(c, fname, ppstr, visithash);
  }
  __arc_append_cstring(c, ">", ppstr);
  return(*ppstr);
}
#endif

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
  NULL,
  NULL,
  NULL,
  __arc_vector_isocmp,
  /* Note a T_CODE object cannot be directly applied.  It has to be
     turned into a closure first. */
  NULL
};
