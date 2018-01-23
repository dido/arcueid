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

#include <setjmp.h>
#include "arcueid.h"
#include "vmengine.h"
#include "gc.h"

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

void init(arc *c)
{
  c->vmthreads = c->curthread = CNIL;
}

static enum arc_trstate apply(arc *c, value t)
{
  /* XXX fill this in */
  return(TR_RC);
}

arctype __arc_thread_t = { NULL, mark, NULL, NULL, NULL, init, apply };

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
  t->ip = 0;
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

static enum arc_trstate resume_thread(arc *c, value thr)
{
  /* XXX fill me in */
  return(TR_SUSPEND);
}

static value apply_cont(arc *c, value thr)
{
  /* XXX fill me in */
  return(CNIL);
}

void __arc_thr_trampoline(arc *c, value thr, enum arc_trstate state)
{
  value cont;
  int jmpval;
  arc_thread *t = (arc_thread *)thr;
  arctype *type;

  jmpval = setjmp(t->errjmp);
  if (jmpval == 2) {
    t->quanta = 0;
    t->state = Tbroken;
    return;
  }

  for (;;) {
    switch (state) {
    case TR_RESUME:
      state = resume_thread(c, thr);
      break;
    case TR_SUSPEND:
      /* just return to the dispatcher */
      return;
    case TR_FNAPP:
      /* Apply value in the accumulator */
      type = arc_type(t->acc);
      /* XXX - error handling should be fixed here */
      if (type->apply == NULL)
	__arc_fatal("cannot apply object", 0);
      state = type->apply(c, thr);
      break;
    case TR_RC:
    default:
      /* Restore last continuation on the continuation register. */
      cont = apply_cont(c, thr);
      if (NILP(cont)) {
	/* No available continuation. If this happens, current thread
	   should terminate. */
	t->quanta = 0;
	t->state = Trelease;
	return;
      }
      /* Else resume the thread after restoring the continuation. */
      state = TR_RESUME;
      break;
    }
  }
    
}

static void process_iowait(arc *c, value iothreads, int select_timeout)
{
  /* XXX fill this in */
}

void arc_thread_dispatch(arc *c)
{
  value thr;
  int nthreads, blockedthreads, iowait, stoppedthreads;
  int sleepthreads, runthreads;
  int gcstatus, select_timeout;
  unsigned long long minsleep;
  value vmqueue, prev, iowait_threads;
  arc_thread *t;

  for (;;) {
    nthreads = blockedthreads = iowait = stoppedthreads = sleepthreads = 0;
    sleepthreads = runthreads = 0;
    minsleep = ULLONG_MAX;
    prev = iowait_threads = CNIL;
    for (vmqueue = c->vmthreads; vmqueue; vmqueue = cdr(vmqueue)) {
      thr = car(vmqueue);
      arc_wb(c, c->curthread, thr);
      c->curthread = thr;
      ++nthreads;
      t = (arc_thread *)thr;
      switch (t->state) {
	/* Remove a thread in Trelease or Tbroken state from the queue */
      case Trelease:
      case Tbroken:
	stoppedthreads++;
	/* XXX - if RVCH is a channel (type not yet defined), send the value
	   through it first. */
	/* Write the current accumulator to the return value */
	arc_wb(c, t->rvch, t->acc);
	t->rvch = t->acc;
	/* Unlink the thread from the queue */
	if (prev == CNIL) {
	  arc_wb(c, c->vmthreads, cdr(vmqueue));
	  c->vmthreads = cdr(vmqueue);
	} else {
	  scdr(c, prev, cdr(vmqueue));
	}
	if (NILP(cdr(vmqueue))) {
	  arc_wb(c, c->vmthrtail, prev);
	  c->vmthrtail = prev;
	}
	goto finished;
	break;
      case Talt:
      case Tsend:
      case Trecv:
	++blockedthreads;
	break;
      case Tsleep:
	/* Wake up a sleeping thread if wakeup time is reached */
	if (__arc_milliseconds() >= t->wuptime) {
	  t->state = Tready;
	  t->acc = CNIL;
	} else {
	  sleepthreads++;
	  if (t->wuptime < minsleep)
	    minsleep = t->wuptime;
	  goto finish_thread;
	}
      case Tready:
	/* run the thread */
	if (t->quanta <= 0)
	  t->quanta = c->quantum;
	__arc_thr_trampoline(c, thr, TR_RESUME);
	break;
      case Tcritical:
	/* Run a thread in critical section until it relinquishes the
	   critical section. */
	while (t->state == Tcritical) {
	  if (t->quanta <= 0)
	    t->quanta = c->quantum;
	  __arc_thr_trampoline(c, thr, TR_RESUME);
	}
	break;
      case Tiowait:
	iowait++;
	iowait_threads = cons(c, thr, iowait_threads);
	break;
      }
    finish_thread:
      if (t->state == Tready || t->state == Tcritical)
	runthreads++;
      prev = vmqueue;
    finished:
      ;
    }

    /* No more threads */
    if (nthreads == 0)
      return;

    /* XXX - should we print a warning if all threads are blocked? */
    if (nthreads == blockedthreads)
      return;
    /* All runnable threads have been allowed to run. See if we need
       any post-run cleanup work before we resume the loop */
    if (iowait > 0) {
      /* XXX Select timeout is chosen based on the following:
	 1. If the GC has not yet finished an epoch, do not wait.
	 2. If all threads are blocked on I/O or are waiting on
	    channels, wait until I/O is possible.
	 3. If all threads are asleep, blocked, or waiting on I/O,
 	    wait for at most the time until the first sleep expires.
	 4. If there are any runnable threads, do not wait.
      */
      if (gcstatus != 0) {
	select_timeout = 0;
      } else if ((iowait + blockedthreads) == nthreads) {
	select_timeout = -1;
      } else if ((iowait + sleepthreads + blockedthreads) == nthreads) {
	select_timeout = minsleep - __arc_milliseconds();
      } else {
	select_timeout = 0;
      }
      process_iowait(c, iowait_threads, select_timeout);
    }

    if (sleepthreads == nthreads) {
      unsigned long long st;

      /* Sleep until it's time for a thread to wake up */
      st = minsleep - __arc_milliseconds();
      __arc_sleep(st);
    }

    gcstatus = __arc_gc(c);
  }
}
      
