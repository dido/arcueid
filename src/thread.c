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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include "arcueid.h"
#include "vmengine.h"
#include "arith.h"
#include "builtins.h"
#include "../config.h"

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

#define DEFAULT_QUANTUM 65536

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
  WV(dw, arc_mkaff(c, __arc_disp_write, CNIL));
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
  /* The stack has to be treated specially.  In a gc cycle, we have
     to clear out the unused portions before marking the stack vector
     itself.  XXX - we could also do this by non-recursively marking
     the stack vector itself and then marking only the used portions.
  */
  for (p = TSBASE(thr); p == TSP(thr); p++) {
    __arc_wb(*p, CNIL);
    *p = CNIL;
  }
  mark(c, TSTACK(thr), depth);

  mark(c, TWAITFD(thr), depth);
  mark(c, TCH(thr), depth);
  mark(c, TEXH(thr), depth);
  mark(c, TCM(thr), depth);
  mark(c, TRVCH(thr), depth);
}

value arc_mkthread(arc *c)
{
  value thr;

  thr = arc_mkobject(c, sizeof(struct vmthread_t), T_THREAD);
  SFUNR(thr, CNIL);
  SENVR(thr, CNIL);
  SVALR(thr, CNIL);
  SCONR(thr, CNIL);
  TSTACK(thr) = arc_mkvector(c, c->stksize);
  TSBASE(thr) = &XVINDEX(TSTACK(thr), 0);
  TSP(thr) = TSTOP(thr) = &XVINDEX(TSTACK(thr), c->stksize-1);
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
  TACELL(thr) = 0;
  TRVCH(thr) = arc_mkchan(c);
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
  SVALR(thr, val);
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
  WV(cm, TCM(thr));
  WV(val, arc_hash_lookup(c, AV(cm), AV(key)));
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
  WV(cm, TCM(thr));
  WV(bind, arc_hash_lookup(c, AV(cm), AV(key)));
  if (AV(bind) == CUNBOUND)
    WV(bind, CNIL);
  WV(bind, cons(c, AV(val), AV(bind)));
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
  WV(cm, TCM(thr));
  WV(bind, arc_hash_lookup(c, AV(cm), AV(key)));
  if (AV(bind) == CUNBOUND)
    ARETURN(CNIL);
  WV(val, car(AV(bind)));
  WV(bind, cdr(AV(bind)));
  if (NIL_P(bind)) {
    arc_hash_delete(c, AV(cm), AV(key));
  } else {
    arc_hash_insert(c, AV(cm), AV(key), AV(bind));
  }
  ARETURN(AV(val));
  AFEND;
}
AFFEND


#ifdef HAVE_SYS_EPOLL_H

#include <sys/epoll.h>

#define MAX_EVENTS 1048576

/* Version of process_iowait using epoll */
static void process_iowait(arc *c, value iowaitdata, int eptimeout)
{
  value thr, iowaittbl;
  int niowait=0, n, nfds;
  static int epollfd = -1;
  struct epoll_event *epevents, ev;

  if (epollfd < 0)
    epollfd = epoll_create(MAX_EVENTS);

  iowaittbl = arc_mkhash(c, ARC_HASHBITS);
  /* add fd's in list to epollfd */
  for (thr = iowaitdata; thr; thr = cdr(thr)) {
    niowait++;
    ev.events = (TWAITRW(car(thr))) ? EPOLLOUT : EPOLLIN;
    ev.data.u64 = 0LL;
    ev.data.fd = TWAITFD(car(thr));
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, TWAITFD(car(thr)), &ev) < 0) {
      int en = errno;
      if (errno != EEXIST) {
	arc_err_cstrfmt(c, "error setting epoll for thread on blocking fd (%s; errno=%d)", strerror(en), en);
	return;
      }
    }
    arc_hash_insert(c, iowaittbl, INT2FIX(TWAITFD(car(thr))), car(thr));
  }

  epevents = (struct epoll_event *)alloca(sizeof(struct epoll_event) * (niowait+2));
  nfds = epoll_wait(epollfd, epevents, niowait, eptimeout);
  if (nfds < 0) {
    int en = errno;
    arc_err_cstrfmt(c, "error waiting for Tiowait fds (%s; errno=%d)",
		    strerror(en), en);
    return;
  }

  for (n=0; n<nfds; n++) {
    int fd;
    value thr;
    fd  = epevents[n].data.fd;
    thr = arc_hash_lookup(c, iowaittbl, INT2FIX(fd));
    if (BOUND_P(thr)) {
      TWAITFD(thr) = -1;
      TSTATE(thr) = Tready;
    }
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
  }
}

#elif HAVE_SYS_SELECT_H

#include <sys/select.h>
#include <unistd.h>

/* Version of process_iowait using select */
static void process_iowait(arc *c, value iowaitdata, int eptimeout)
{
  fd_set rfds, wfds;
  struct timeval tv;
  int retval;
  value thr;
  int niowait=0, nfds;

  FD_ZERO(&rfds);
  FD_ZERO(&wfds);
  nfds = 0;
  for (thr = iowaitdata; thr; thr = cdr(thr)) {
    niowait++;
    if (nfds < TWAITFD(car(thr)))
      nfds = TWAITFD(car(thr));
    if (TWAITRW(car(thr))) {
      FD_SET(TWAITFD(car(thr)), &wfds);
    } else {
      FD_SET(TWAITFD(car(thr)), &rfds);
    }
  }

  tv.tv_sec = eptimeout / 1000;
  tv.tv_usec = (eptimeout % 1000) * 1000L;

  retval = select(nfds+1, &rfds, &wfds, NULL, &tv);
  if (retval == -1) {
    int en = errno;
    arc_err_cstrfmt(c, "error waiting for Tiowait fds (%s; errno=%d)",
		    strerror(en), en);
    return;
  }

  /* nothing to do? */
  if (retval == 0)
    return;

  /* Wake up all the waiting threads with fds for which select said ok */
  for (thr = iowaitdata; thr; thr = cdr(thr)) {
    int result;

    result = (TWAITRW(car(thr))) ? FD_ISSET(TWAITFD(car(thr)), &wfds)
      : FD_ISSET(TWAITFD(car(thr)), &rfds);
    if (result) {
      TWAITFD(car(thr)) = -1;
      TSTATE(car(thr)) = Tready;
    }
  }
}

#else

/* versions of process_iowait that use other mechanisms for selecting
   usable file descriptors go here. */

#endif

extern value __arc_send_rvchan(arc *c, value chan, value val);
extern int __arc_recv_rvchan(arc *c, value thr);

/* Main dispatcher.  Will run each thread in the thread list for
   at most c->quanta cycles or until the thread leaves ready state.
   Also runs garbage collections periodically.  This should
   be called with at least one thread already in the run queue.
   Terminates when no more threads are available.

   全く, this is beginning to look a lot a like the reactor pattern!
*/
void arc_thread_dispatch(arc *c)
{
  value thr, prev;
  int nthreads, blockedthreads, iowait, sleepthreads, runthreads;
  int stoppedthreads, need_select;
  unsigned long long minsleep;
  value iowaitdata;
  int eptimeout;

  for (;;) {
    nthreads = blockedthreads = iowait = stoppedthreads = 0;
    sleepthreads = runthreads = need_select = 0;
    minsleep = ULLONG_MAX;
    iowaitdata = CNIL;
    /* XXX - see if we need more extensive use of the write barrier when
       doing these thread manipulations */
    for (c->vmqueue = c->vmthreads, prev = CNIL; c->vmqueue;
	 prev = c->vmqueue, c->vmqueue = cdr(c->vmqueue)) {
      thr = car(c->vmqueue);
      __arc_wb(c->curthread, thr);
      c->curthread = thr;
      ++nthreads;
      switch (TSTATE(thr)) {
	/* Remove a thread in Trelease or Tbroken state from the
	   queue. */
      case Trelease:
      case Tbroken:
	stoppedthreads++;
	if (prev == CNIL)
	  c->vmthreads = cdr(c->vmqueue);
	else
	  scdr(prev, cdr(c->vmqueue));
	if (c->vmthrtail == c->vmqueue)
	  c->vmthrtail = prev;

	/* This will serve to wake up all the threads waiting on
	   the return value channel of the thread, so they can pick
	   up the return value now that it is available. */
	if (TYPE(TRVCH(thr)) == T_CHAN)
	  __arc_send_rvchan(c, TRVCH(thr), TVALR(thr));
	__arc_wb(TRVCH(thr), TVALR(thr));
	TRVCH(thr) = TVALR(thr);
	break;
      case Talt:
      case Tsend:
      case Trecv:
	/* increment count of threads in blocked states */
	++blockedthreads;
	break;
      case Tsleep:
	/* Wake up a sleeping thread if the wakeup time is reached */
	if (__arc_milliseconds() >= TWAKEUP(thr)) {
	  TSTATE(thr) = Tready;
	  SVALR(thr, CNIL);
	} else {
	  sleepthreads++;
	  if (TWAKEUP(thr) < minsleep)
	    minsleep = TWAKEUP(thr);
	  continue;
	}
	/* fall through and let the thread run if so */
      case Tready:
	/* let the thread run */
	if (TQUANTA(thr) <= 0)
	  TQUANTA(thr) = c->quantum;
	__arc_thr_trampoline(c, thr, TR_RESUME);
	break;
      case Tcritical:
	/* If we are in a critical section, allow the thread to keep
	   running by itself until it leaves the critical state. */
	while (TSTATE(thr) == Tcritical) {
	  if (TQUANTA(thr) <= 0)
	    TQUANTA(thr) = c->quantum;
	  __arc_thr_trampoline(c, thr, TR_RESUME);
	}
	break;
      case Tiowait:
	/* Build up the list of threads for which I/O is pending */
	iowait++;
	need_select = 1;
	iowaitdata = cons(c, thr, iowaitdata);
	break;
      }

      if (RUNNABLE(thr))
	runthreads++;
    }

    /* XXX - should we print a warning message if we abort when all
       threads are blocked?  I suppose it should be up to the caller
       to decide whether this is a bad thing or no.  It isn't an
       issue for the REPL. */
    if (nthreads == 0 || nthreads == blockedthreads)
      return;

    /* We have now finished running all threads.  See if we need to
       do some post-cleanup work */
    if (need_select) {
      if ((iowait + blockedthreads) == nthreads) {
	/* If all threads are blocked on I/O or are waiting on channels,
	   make epoll wait indefinitely until I/O is possible. */
	eptimeout = -1;
      } else if ((iowait + sleepthreads + blockedthreads) == nthreads) {
	/* If all threads are either asleep, blocked, or waiting on
	 I/O, wait for at most the time until the first sleep expires. */
	eptimeout = (minsleep - __arc_milliseconds());
      } else {
	/* do not wait if there are any other threads which can run */
	eptimeout = 0;
      }
      process_iowait(c, iowaitdata, eptimeout);
    }

    /* If all threads are asleep, use nanosleep to wait the the shortest
       time until it's time for a thread to wake up */
    if (sleepthreads == nthreads) {
      unsigned long long st;
      struct timespec req;

      st = minsleep - __arc_milliseconds();
      req.tv_sec = st/1000;
      req.tv_nsec = ((st % 1000) * 1000000L);
      nanosleep(&req, NULL);
    }
    /* Perform garbage collection: should be done after every cycle
       with VCGC, as though it were a thread in our scheduler. */
    c->gc(c);
    /* XXX - detect deadlock */
  }
}

value arc_spawn(arc *c, value thunk)
{
  value thr;
  typefn_t *tfn;

  tfn = __arc_typefn(c, thunk);
  if (tfn == NULL || tfn->apply == NULL) {
    arc_err_cstrfmt(c, "cannot apply thunk for spawn");
    return(CNIL);
  }
  thr = arc_mkthread(c);
  SFUNR(thr, SVALR(thr, thunk));
  /* XXX - copy continuation marks from the spawning thread to the new
     thread if there is a parent thread. */

  /* If the state on first application is not TR_RESUME, then the thread
     terminates right then and there and is never enqueued. */
  if (tfn->apply(c, thr, TVALR(thr)) != TR_RESUME) {
    TSTATE(thr) = Trelease;
  } else {
    /* Otherwise, queue the new thread and enqueue it in the dispatcher. */
    __arc_enqueue(c, thr, &c->vmthreads, &c->vmthrtail);
  }
  return(thr);
}

AFFDEF(arc_sleep)
{
  AARG(sleeptime);
  value timetowake;
  AFBEGIN;

  AFCALL(arc_mkaff(c, arc_coerce, CNIL), AV(sleeptime),
	 ARC_BUILTIN(c, S_FLONUM));
  timetowake = AFCRV;
  if (REPFLO(timetowake) < 0.0) {
    arc_err_cstrfmt(c, "negative sleep time");
    return(CNIL);
  }
  TWAKEUP(thr) = __arc_milliseconds() + (unsigned long long)(REPFLO(timetowake)*1000.0);
  TSTATE(thr) = Tsleep;
  AYIELD();
  AFEND;
}
AFFEND

value arc_dead(arc *c, value thr)
{
  TYPECHECK(thr, T_THREAD);
  return((TSTATE(thr) == Trelease || TSTATE(thr) == Tbroken) ? CTRUE : CNIL);
}

AFFDEF(arc_atomic_cell)
{
  AOARG(val);
  AFBEGIN;
  if (!BOUND_P(AV(val)))
    ARETURN((TACELL(thr) == 0) ? CNIL : CTRUE);
  TACELL(thr) = (NIL_P(AV(val))) ? 0 : 1;
  ARETURN(AV(val));
  AFEND;
}
AFFEND

AFFDEF(arc_kill_thread)
{
  AARG(tthr);
  AVAR(achan);
  AFBEGIN;
  TSTATE(AV(tthr)) = Tbroken;
  if (TACELL(AV(tthr))) {
    /* release atomic cell */
    TACELL(AV(tthr)) = 0;
    WV(achan, arc_gbind(c, "__achan__"));
    if (BOUND_P(AV(achan))) {
      AFCALL(arc_mkaff(c, arc_recv_channel, CNIL), AV(achan));
    }
  }
  ARETURN(AV(tthr));
  AFEND;
}
AFFEND

/* If the thread is running, paused on I/O, or sleeping, this will cause
   the thread to become running again and resume in a call to arc_err.
   Note that unlike in reference Arc, it is possible to use on-err to
   capture the break exception.

   This function cannot break a thread that is waiting to send to
   or receive from a channel. */
value arc_break_thread(arc *c, value tthr)
{
  typefn_t *tfn;

  /* do nothing if the thread is in either state */
  if (!(TSTATE(tthr) == Tready || TSTATE(tthr) == Tsleep
	|| TSTATE(tthr) == Tiowait))
    return(tthr);

  /* force the thread to become ready */
  TSTATE(tthr) = Tready;

  /* make the thread resume at a call to arc_err */
  SVALR(tthr, arc_mkaff(c, arc_err, CNIL));
  CPUSH(tthr, arc_mkstringc(c, "user break"));
  SFUNR(tthr, TVALR(tthr));
  tfn = __arc_typefn(c, TVALR(tthr));
  tfn->apply(c, tthr, TVALR(tthr));
  return(tthr);
}

AFFDEF(arc_join_thread)
{
  AARG(jthr);
  AFBEGIN;
  while (TYPE(TRVCH(AV(jthr))) == T_CHAN) {
    AFCALL(arc_mkaff(c, __arc_recv_rvchan, CNIL), TRVCH(AV(jthr)));
  }
  ARETURN(TRVCH(AV(jthr)));
  AFEND;
}
AFFEND

value arc_current_thread(arc *c)
{
  return(c->curthread);
}

void arc_init_threads(arc *c)
{
  c->vmthreads = CNIL;
  c->vmthrtail = CNIL;
  c->vmqueue = CNIL;
  c->curthread = CNIL;
  c->tid_nonce = 0;
  c->stksize = TSTKSIZE;
  c->here = cons(c, INT2FIX(0xdead), CNIL);
  c->quantum = DEFAULT_QUANTUM;
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
