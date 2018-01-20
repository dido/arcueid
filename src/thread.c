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

static void mark(arc *c, value v,
		     void (*marker)(struct arc *, value, int),
		     int depth)
{
  arc_thread *thr = (arc_thread *)v;
  value *ptr;

  marker(c, thr->func, depth);
  marker(c, thr->env, depth);
  marker(c, thr->acc, depth);
  marker(c, thr->cont, depth);
  marker(c, thr->exh, depth);
  marker(c, thr->cmarks, depth);
  marker(c, thr->rvch, depth);
  /* Mark only portions of the stack which are active */
  for (ptr = thr->sp; ptr < thr->stktop; ptr++)
    marker(c, *ptr, depth);
}

arctype __arc_thread_t = { NULL, mark, NULL, NULL, NULL, NULL };

value __arc_thread_new(arc *c, int tid)
{
  value thr = arc_new(c, &__arc_thread_t, sizeof(arc_thread));
  arc_thread *t = (arc_thread *)t;
  value *s;
  int slen;

  t->c = c;
  t->func = CNIL;
  t->env = CNIL;
  t->acc = CNIL;
  t->cont = CNIL;
  t->stack = arc_vector_new(c, c->stksize);
  s = (value *)t->stack;
  slen = VLEN(t->stack);
  t->stkbase = s+1;
  t->stktop = t->sp = s+slen;
  t->ip.ptr = NULL;
  t->argc = 0;
  t->state = Tready;
  t->tid = tid;
  t->quanta = 0L;
  t->ticks = 0LL;
  t->wuptime = 0LL;
  t->waitfd = -1;
  t->waitrw = -1;
  t->exh = CNIL;
  t->cmarks = arc_tbl_new(c, ARC_HASHBITS);
  t->rvch = CNIL;
  return(thr);
}
