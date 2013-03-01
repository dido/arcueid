/* 
  Copyright (C) 2013 Rafael R. Sevilla

  This file is part of Arcueid

  Arcueid is free software: you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library. If not, see <http://www.gnu.org/licenses/>
*/
#include "arcueid.h"
#include "vmengine.h"

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
#ifndef alloca
# define alloca __builtin_alloca
#endif
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# include <stddef.h>
void *alloca (size_t);
#endif

static value thread_pprint(arc *c, value sexpr, value *ppstr, value visithash)
{
  char *outstr;
  int len;

  __arc_append_cstring(c, "#<thread: ", ppstr);
  len = snprintf(NULL, 0, "%d>", TTID(sexpr));
  outstr = (char *)alloca(sizeof(char)*(len+2));
  snprintf(outstr, len+1, "%d>", TTID(sexpr));
  __arc_append_cstring(c, outstr, ppstr);
  return(*ppstr);  
}

static void thread_marker(arc *c, value thr, int depth,
			  void (*mark)(struct arc *, value, int))
{
  value *p;

  mark(c, TFUNR(thr), depth);
  mark(c, TENVR(thr), depth);
  mark(c, TVALR(thr), depth);
  mark(c, TCONR(thr), depth);
  for (p = TSTOP(thr); p == TSP(thr); p--)
    mark(c, *p, depth);
}

value arc_mkthread(arc *c)
{
  value thr;

  thr = arc_mkobject(c, sizeof(struct vmthread_t), T_THREAD);
  TFUNR(thr) = CNIL;
  TENVR(thr) = CNIL;
  TVALR(thr) = CNIL;
  TCONR(thr) = CNIL;
  TSTACK(thr) = arc_mkvector(c, c->stksize);
  TSBASE(thr) = &VINDEX(TSTACK(thr), 0);
  TSP(thr) = TSTOP(thr) = &VINDEX(TSTACK(thr), c->stksize-1);
  TIP(thr).ipptr = NULL;
  TARGC(thr) = 0;

  TSTATE(thr) = Tready;
  TTID(thr) = ++c->tid_nonce;
  TQUANTA(thr) = 0;
  TTICKS(thr) = 0LL;
  TWAKEUP(thr) = 0LL;
  TWAITFD(thr) = CNIL;
  return(thr);
}

void arc_thr_push(arc *c, value thr, value v)
{
  /* XXX this should do overflow checks */
  CPUSH(thr, v);
}

value arc_thr_pop(arc *c, value thr)
{
  return(CPOP(thr));
}

value arc_thr_valr(arc *c, value thr)
{
  return(TVALR(thr));
}

value arc_thr_set_valr(arc *c, value thr, value val)
{
  TVALR(thr) = val;
  return(val);
}

int arc_thr_argc(arc *c, value thr)
{
  return(TARGC(thr));
}

value arc_thr_envr(arc *c, value thr)
{
  return(TENVR(thr));
}

void arc_init_threads(arc *c)
{
  c->vmthreads = CNIL;
  c->vmthrtail = CNIL;
  c->vmqueue = CNIL;
  c->curthread = CNIL;
  c->tid_nonce = 0;
  c->stksize = TSTKSIZE;
}

typefn_t __arc_thread_typefn__ = {
  thread_marker,
  __arc_null_sweeper,
  thread_pprint,
  NULL,
  NULL,
  NULL,
  NULL
};
