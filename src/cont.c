/* Copyright (C) 2017, 2018 Rafael R. Sevilla

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

/* Make a continuation on the stack. Returns the fixnum offset of the
   top of the stack after all continuation information has been saved */
value __arc_cont(arc *c, value thr, int ip)
{
  arc_thread *t = (arc_thread *)thr;
  value cont, tsfn;

  tsfn = INT2FIX(t->stktop - t->stkfn);
  CPUSH(thr, tsfn);
  CPUSH(thr, INT2FIX(ip));
  CPUSH(thr, t->env);
  CPUSH(thr, t->func);
  CPUSH(thr, INT2FIX(t->argc));
  CPUSH(thr, t->cont);
  cont = INT2FIX(t->stktop - t->sp);
  return(cont);
}

void __arc_restorecont(arc *c, value thr, value cont)
{
  int offset;
  arc_thread *t = (arc_thread *)thr;
  value tmp;

  if (FIXNUMP(cont)) {
    /* A continuation on the stack is just an offset into the stack. */
    t->sp = t->stktop - FIX2INT(cont);
    tmp = CPOP(thr);
    arc_wb(c, t->cont, tmp);
    t->cont = tmp;
    t->argc = FIX2INT(CPOP(thr));
    tmp = CPOP(thr);
    arc_wb(c, t->func, tmp);
    t->func = tmp;
    tmp = CPOP(thr);
    arc_wb(c, t->env, tmp);
    t->env = tmp;
    offset = FIX2INT(CPOP(thr));
    t->stkfn = t->stktop - FIX2INT(CPOP(thr));
  } else {
    /* XXX fill in heap-based continuations */
    __arc_fatal("__arc_restorecont for heap continuations not yet implemented", 0);
  }
  t->ip = offset;
  return;
}
