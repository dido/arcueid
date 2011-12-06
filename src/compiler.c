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

value arc_eval(arc *c, value argv, value rv, CC4CTX)
{
  CC4VDEFBEGIN;
  CC4VARDEF(fn);
  CC4VARDEF(ctx);
  CC4VDEFEND;
  CC4BEGIN(c);
  /* code generation context */
  /* CC4V(ctx) = arc_mkcctx(c); */
  CC4V(fn) = arc_compile(c, VINDEX(argv, 0), CC4V(ctx), CNIL);
  CC4CALL(c, argv, fn, 0);
  CC4END;
  return(rv);
}

/* Given an expression nexpr, a compilation context ctx, and a continuation
   flag, return the compilation context after the expression is compiled.
   NOTE: all macros must be fully expanded before compiling!  This will
   produce an anonymous function which should be suitable for use with
   arc_apply. */
value arc_compile(arc *c, value nexpr, value ctx, value cont)
{
}
