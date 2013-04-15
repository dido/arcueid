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

static AFFDEF(thread_pprint)
{
  AARG(sexpr, disp, fp);
  AOARG(visithash);
  AVAR(dw);
  char *coutstr;
  int len;
  value outstr;
  AFBEGIN;
  (void)disp;
  AV(dw) = arc_mkaff(c, __arc_disp_write, CNIL);
  len = snprintf(NULL, 0, "#<thread: %d>", TTID(AV(sexpr)));
  coutstr = (char *)alloca(sizeof(char)*(len+2));
  snprintf(coutstr, len+1, "#<thread: %d>", TTID(AV(sexpr)));
  outstr = arc_mkstringc(c, coutstr);
  AFTCALL(AV(dw), outstr, CTRUE, AV(fp), AV(visithash));
  AFEND;
}
AFFEND

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

  mark(c, TWAITFD(thr), depth);
  mark(c, TCH(thr), depth);
  mark(c, TEXH(thr), depth);
  mark(c, TCM(thr), depth);
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
  TCM(thr) = arc_mkhash(c, ARC_HASHBITS);
  TEXH(thr) = CNIL;
  TCH(thr) = c->here;
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

AFFDEF(arc_cmark)
{
  AARG(key);
  AVAR(cm, val);
  AFBEGIN;
  AV(cm) = TCM(thr);
  AV(val) = arc_hash_lookup(c, AV(cm), AV(key));
  if (AV(val) == CUNBOUND)
    ARETURN(CNIL);
  ARETURN(car(AV(val)));
  AFEND;
}
AFFEND

/* Do not use these functions outside the call-w/cmark macro */
AFFDEF(arc_scmark)
{
  AARG(key, val);
  AVAR(cm, bind);
  AFBEGIN;
  AV(cm) = TCM(thr);
  AV(bind) = arc_hash_lookup(c, AV(cm), AV(key));
  if (AV(bind) == CUNBOUND)
    AV(bind) = CNIL;
  bind = cons(c, val, bind);
  arc_hash_insert(c, AV(cm), AV(key), AV(bind));
  ARETURN(val);
  AFEND;
}
AFFEND

AFFDEF(arc_ccmark)
{
  AARG(key);
  AVAR(cm, bind, val);
  AFBEGIN;
  AV(cm) = TCM(thr);
  AV(bind) = arc_hash_lookup(c, AV(cm), AV(key));
  if (AV(bind) == CUNBOUND)
    ARETURN(CNIL);
  AV(val) = car(AV(bind));
  AV(bind) = cdr(AV(bind));
  if (NIL_P(bind)) {
    arc_hash_delete(c, AV(cm), AV(key));
  } else {
    arc_hash_insert(c, AV(cm), AV(key), AV(bind));
  }
  ARETURN(AV(val));
  AFEND;
}
AFFEND

void arc_init_threads(arc *c)
{
  c->vmthreads = CNIL;
  c->vmthrtail = CNIL;
  c->vmqueue = CNIL;
  c->curthread = CNIL;
  c->tid_nonce = 0;
  c->stksize = TSTKSIZE;
  c->here = cons(c, INT2FIX(0xdead), CNIL);
}

typefn_t __arc_thread_typefn__ = {
  thread_marker,
  __arc_null_sweeper,
  thread_pprint,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
