/* 
  Copyright (C) 2012 Rafael R. Sevilla

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <setjmp.h>
#include <errno.h>
#include <time.h>
#include "../config.h"
#include "arcueid.h"
#include "vmengine.h"
#include "alloc.h"
#include "arith.h"
#include "symbols.h"
#include "io.h"
#ifdef HAVE_SYS_EPOLL_H
#include <sys/epoll.h>
#elif defined(HAVE_SYS_SELECT_H)
#include <sys/select.h>
#endif

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

/* This is a kludge that effectively implements continuations in C.
   It will actually save and restore the whole stack.  This code
   is adapted from a method by Dan Piponi:

   http://homepage.mac.com/sigfpe/Computing/continuations.html

   It works on x86-64 at least.  I have no idea how this rather
   unorthodox method (which courts fandango on core) will fare
   on other architectures.
 */

static long *pbos;

static void save_stack(struct ccont *c, long *pbos, long *ptos)
{
  int n = pbos-ptos;
  int i;

  c->stack = (long *)malloc(n*sizeof(long));
  c->n = n;
  for (i=0; i<n; i++)
    c->stack[i] = pbos[-i];
}

static struct ccont *saveccont(void) {
  struct ccont *c = (struct ccont *)malloc(sizeof(struct ccont));
  long tos;

  if (!setjmp(c->registers)) {
    save_stack(c, pbos, &tos);
    return(c);
  }
  return(NULL);
}

static void restore_stack(struct ccont *c, int once_more) {
  long padding[12];
  long tos;
  int i,n;

  memset(padding, 0, 0);
  if (pbos - c->n < &tos)
    restore_stack(c, 1);

  if (once_more)
    restore_stack(c, 0);

  n = c->n;
  for (i=0; i<n; i++)
    pbos[-i] = c->stack[i];
  longjmp(c->registers, 1);
}

static void restoreccont(struct ccont *c)
{
  restore_stack(c, 1);
}

static void destroyccont(struct ccont *c)
{
  free(c->stack);
  free(c);
}

/* Used when threads are waiting on file descriptors */
void arc_thread_wait_fd(volatile arc *c, volatile int fd)
{
  volatile value thr = c->curthread;
  struct ccont *cc;

  /* No thread? */
  if (thr == CNIL)
    return;

  /* Do not wait for file descriptors if we are in a critical
     section, compiling a macro, the only thread running, or
     are a thread that is about to be terminated. */
  if (TSTATE(thr) == Tcritical || TSTATE(thr) == Texiting)
    return;
  if (c->in_compile)
    return;
  if (car(c->vmthreads) == thr && cdr(c->vmthreads) == CNIL)
    return;

  TSTATE(thr) = Tiowait;
  TWAITFD(thr) = fd;
  /* save our current context so when Tiowait becomes Tioready,
     the dispatcher can bring us back to this point. */
  if ((cc = saveccont()) != NULL) {
    TRSC(thr) = cc;
    /* This jump should return us to the head of the virtual
       machine.  Since it is taken when the thread's state
       has changed to an unrunnable Tiowait, the vmengine
       call will immediately return to the dispatcher just
       as if it had finished its quantum. */
    longjmp(TVJMP(thr), 1);
  }
  /* if we are no longer in the I/O wait state, I/O can resume.
     Destroy the "continuation" we created to get ourselves back
     here. */
  destroyccont(TRSC(thr));
  TRSC(thr) = NULL;
  return;
}

value arc_sleep(arc *c, value sleeptime)
{
  double timetowake;
  value thr = c->curthread;

  timetowake = arc_coerce_flonum(c, sleeptime);
  if (timetowake < 0.0) {
    arc_err_cstrfmt(c, "negative sleep time");
    return(CNIL);
  }
  TWAKEUP(thr) = __arc_milliseconds() + (unsigned long long)(timetowake*1000.0);
  TSTATE(thr) = Tsleep;
  /* Return us to the head of the virtual machine.  Since we
     have changed thread state to Tsleep, it will return immediately,
     and will not resume until after the wakeup time has been
     reached. */
  longjmp(TVJMP(thr), 1);
  return(CNIL);
}

/* Main dispatcher.  Will run each thread in the thread list for
   at most c->quanta cycles or until the thread leaves ready state.
   Also runs garbage collector threads periodically.  This should
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
#ifdef HAVE_SYS_EPOLL_H
#define MAX_EVENTS 8192
  int epollfd;
  int eptimeout, nfds, n;
  struct epoll_event epevents[MAX_EVENTS], ev;
#endif
  long bos;

  pbos = &bos;


  c->vmqueue = c->vmthreads;
  prev = CNIL;
#ifdef HAVE_SYS_EPOLL_H
  epollfd = epoll_create(MAX_EVENTS);
#endif

  for (;;) {
    nthreads = blockedthreads = iowait = stoppedthreads
      = sleepthreads = runthreads = need_select = 0;
    minsleep = ULLONG_MAX;
    for (c->vmqueue = c->vmthreads, prev = CNIL; c->vmqueue;
	 prev = c->vmqueue, c->vmqueue = cdr(c->vmqueue)) {
      thr = car(c->vmqueue);
      c->curthread = thr;
      ++nthreads;
      switch (TSTATE(thr)) {
	/* Remove a thread in Trelease or Tbroken state from the queue. */
      case Trelease:
      case Tbroken:
	stoppedthreads++;
	if (prev == CNIL)
	  WB(&c->vmthreads, cdr(c->vmqueue));
	else
	  scdr(prev, cdr(c->vmqueue));
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
	  TVALR(thr) = CNIL;
	} else {
	  sleepthreads++;
	  if (TWAKEUP(thr) < minsleep)
	    minsleep = TWAKEUP(thr);
	  continue;    /* keep going */
	}
	/* fall through and let the thread run if so */
      case Tready:
      case Texiting:
	/* let the thread run */
	arc_vmengine(c, thr, c->quantum);
	break;
      case Tcritical:
	/* if we are in a critical section, allow the thread
	   to keep running by itself until it leaves critical */
	while (TSTATE(thr) == Tcritical)
	  arc_vmengine(c, thr, c->quantum);
      case Tiowait:
	iowait++;
	need_select = 1;
#ifdef HAVE_SYS_EPOLL_H
	ev.events = EPOLLIN;
	ev.data.fd = TWAITFD(thr);
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, TWAITFD(thr), &ev) < 0) {
	  int en = errno;
	  if (errno != EEXIST) {
	    arc_err_cstrfmt(c, "error setting epoll for thread on blocking fd (%s; errno=%d)", strerror(en), en);
	    return;
	  }
	}
	arc_hash_insert(c, c->iowaittbl, INT2FIX(TWAITFD(thr)), thr);
	break;
#else
#error no epoll!
#endif
      case Tioready:
	/* Once the select/epoll stuff has figured out that the
	   associated file descriptor is ready for use, we can
	   resume the saved context, but with our state changed from
	   Tiowait to Tready. */
	TSTATE(thr) = Tready;
	restoreccont(TRSC(thr));
	break;
      }
      if (RUNNABLE(thr))
	runthreads++;
    }

    if (nthreads == 0)
      return;

    /* We have now finished running all threads.  See if we need to
       do some post-cleanup work */
    if (need_select) {
#ifdef HAVE_SYS_EPOLL_H
      if (iowait == nthreads) {
	/* If all threads are blocked on I/O, make epoll wait
	   indefinitely. */
	eptimeout = -1;
      } else if ((iowait + sleepthreads) == nthreads) {
	/* If all threads are either asleep or waiting on I/O, wait
	   for at most the time until the first sleep expires. */
	eptimeout = (minsleep - __arc_milliseconds())/1000;
      } else {
	/* do not wait if there are any threads which can run */
	eptimeout = 0;
      }
      eptimeout = (iowait == nthreads) ? -1 : 0;
      nfds = epoll_wait(epollfd, epevents, MAX_EVENTS, eptimeout);
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
	thr = arc_hash_lookup(c, c->iowaittbl, INT2FIX(fd));
	if (thr != CUNBOUND) {
	  TWAITFD(thr) = -1;
	  TWAITFOR(thr) = 0;
	  TSTATE(thr) = Tioready;
	}
	arc_hash_delete(c, c->iowaittbl, INT2FIX(fd));
	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
      }
#else
#error no epoll!
#endif
    }
    /* If all threads are asleep, use nanosleep to wait the
       shortest time */
    if (sleepthreads == nthreads) {
      unsigned long long st;
      struct timespec req;

      st = minsleep - __arc_milliseconds();
      req.tv_sec = st / 1000;
      req.tv_nsec = ((st % 1000) * 1000000L);
      nanosleep(&req, NULL);
    }
    /* XXX - detect deadlock */
  }
}

static int __arc_tidctr = 0;

value arc_spawn(arc *c, value thunk)
{
  value thr, tmp;

  TYPECHECK(thunk, T_CLOS, 1);
  thr = arc_mkthread(c, car(thunk), c->stksize, 0);
  TARGC(thr) = 0;
  TENVR(thr) = cdr(thunk);
  /* inherit standard handles from calling thread */
  VINDEX(TSTDH(thr), 0) = arc_stdin(c);
  VINDEX(TSTDH(thr), 1) = arc_stdout(c);
  VINDEX(TSTDH(thr), 2) = arc_stderr(c);
  TTID(thr) = __arc_tidctr++;
  /* Queue the new thread so that it gets to run just after the
     current thread (which invoked the spawn) if any */
  if (c->vmqueue == CNIL) {
    /* The first thread */
    c->vmthreads = cons(c, thr, CNIL);
    c->vmqueue = c->vmthreads;
  } else {
    tmp = cons(c, thr, cdr(c->vmqueue));
    scdr(c->vmqueue, tmp);
  }
  return(thr);
}
