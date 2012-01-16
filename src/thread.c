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

static value __send_rvch(arc *c, value chan, value data);

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

/* Utility functions for queues */
static void enqueue(arc *c, value thr, value *head, value *tail)
{
  value cell;

  cell = cons(c, thr, CNIL);
  if (*head == CNIL && *tail == CNIL) {
    WB(head, cell);
    WB(tail, cell);
    return;
  }
  scdr(*tail, cell);
  WB(tail, cell);
}

static value dequeue(arc *c, value *head, value *tail)
{
  value thr;

  /* empty queue */
  if (*head == CNIL && *tail == CNIL)
    return(CNIL);
  thr = car(*head);
  WB(head, cdr(*head));
  if (NIL_P(*head))
    WB(tail, *head);
  return(thr);
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
  /* restore the continuation used to call arc_sleep, so when the thread
     resumes it should continue execution after sleep was called. */
  arc_return(c, thr);
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
    nthreads = blockedthreads = iowait = stoppedthreads = 0;
    sleepthreads = runthreads = need_select = 0;
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
	if (c->vmthrtail == c->vmqueue)
	  WB(&c->vmthrtail, prev);
	if (TYPE(TRVCH(thr)) == T_CHAN) {
	  __send_rvch(c, TRVCH(thr), TVALR(thr));
	  WB(&TRVCH(thr), TVALR(thr));
	}
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
	break;
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
      /* printf("Thread %d completed, state %d\n", TTID(thr), TSTATE(thr)); */
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
#ifdef HAVE_SYS_EPOLL_H
      if ((iowait + blockedthreads) == nthreads) {
	/* If all threads are blocked on I/O or are waiting on channels,
	   make epoll wait indefinitely.  The only possible thing
	   (barring deadlock conditions) that can make the threads waiting
	   on channels resume operation is one of the I/O blocked threads
	   resuming, so we should wait indefinitely for that. */
	eptimeout = -1;
      } else if ((iowait + sleepthreads + blockedthreads) == nthreads) {
	/* If all threads are either asleep, blocked, or waiting on I/O,
	   wait for at most the time until the first sleep expires. */
	eptimeout = (minsleep - __arc_milliseconds())/1000;
      } else {
	/* do not wait if there are any threads which can run */
	eptimeout = 0;
      }
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
  value thr;

  TYPECHECK(thunk, T_CLOS, 1);
  thr = arc_mkthread(c, car(thunk), c->stksize, 0);
  TARGC(thr) = 0;
  TENVR(thr) = cdr(thunk);
  /* inherit standard handles from calling thread */
  VINDEX(TSTDH(thr), 0) = arc_stdin(c);
  VINDEX(TSTDH(thr), 1) = arc_stdout(c);
  VINDEX(TSTDH(thr), 2) = arc_stderr(c);
  TTID(thr) = __arc_tidctr++;
  /* Queue the new thread  */
  enqueue(c, thr, &c->vmthreads, &c->vmthrtail);
  return(thr);
}

/* A channel is again a vector with the following entries:
   0 - whether the channel has data or not (CTRUE or CNIL)
   1 - Data in the channel, if any (whatever)
   2 - Head of list of threads waiting to receive from the channel (cons)
   3 - Tail of list of threads waiting to receive from the channel
   4 - Head of list of threads waiting to send to the channel (cons)
       This is actually a list of thread-value pairs.
   5 - Tail of list of threads waiting to send to the channel
 */
value arc_mkchan(arc *c)
{
  value chan;

  chan = arc_mkvector(c, 6);
  BTYPE(chan) = T_CHAN;
  VINDEX(chan, 0) = VINDEX(chan, 1) = VINDEX(chan, 2)  = CNIL;
  VINDEX(chan, 3) = VINDEX(chan, 4) = VINDEX(chan, 5) = CNIL;
  return(chan);
}

static value __recv_channel(arc *c, value curthread, value chan);
static value __send_channel(arc *c, value curthread, value chan, value val);

static value __recv_channel(arc *c, value curthread, value chan)
{
  value thr, val, nval;

  if (NIL_P(VINDEX(chan, 0))) {
    /* We have no value that can be received from the channel.  Enqueue
       the thread and freeze it into Trecv state.  This should never
       happen with a recursive call to __recv_channel. */
    enqueue(c, curthread, &VINDEX(chan, 2), &VINDEX(chan, 3));
    /* Change state to Trecv, which is not runnable, and toss us back
       to the virtual machine loop, which will see we are no longer
       runnable and return us to the dispatcher so some other thread
       can be made to run instead. */
    TSTATE(curthread) = Trecv;
    longjmp(TVJMP(curthread), 1);
  }

  /* There is a value that can be received from the channel.  Read it,
     and see if there is any thread waiting to send. */
  VINDEX(chan, 0) = CNIL;
  val = VINDEX(chan, 1);
  thr = dequeue(c, &VINDEX(chan, 4), &VINDEX(chan, 5));
  if (thr == CNIL) {
    /* No threads waiting to send, just return */
    return(val);
  }

  /* There is at least one thread waiting to send on this channel. 
     Wake it up. */
  nval = cdr(thr);
  thr = car(thr);
  TSTATE(thr) = Tready;
  /* Sets the value returned by arc_send_channel in the thread
     we have woken up. */
  WB(&TVALR(thr), __send_channel(c, thr, chan, nval));
  /* restore continuation used to call arc_send_channel in the thread
     we are waking up. */
  arc_return(c, thr);
  /* return the value read from the channel to our caller */
  return(val);
}

static value __send_channel(arc *c, value curthread, value chan, value val)
{
  value thr;

  if (VINDEX(chan, 0) == CTRUE) {
    /* There is a value in the channel that was written that has not
     yet been read.  Enqueue the thread and the value it wanted
     to write. Should never happen with a recursive call to
     __send_channel. */
    enqueue(c, cons(c, curthread, val), &VINDEX(chan, 4), &VINDEX(chan, 5));
    /* Change state to Tsend, which is not runnable, and toss us back
       to the virtual machine loop, which will see we are no longer
       runnable and return us to the dispatcher so some other thread
       can be made to run instead. */
    TSTATE(curthread) = Tsend;
    longjmp(TVJMP(curthread), 1);
  }


  VINDEX(chan, 0) = CTRUE;
  VINDEX(chan, 1) = val;
  thr = dequeue(c, &VINDEX(chan, 2), &VINDEX(chan, 3));
  if (thr == CNIL) {
    /* no threads waiting to receive */
    return(val);
  }
  /* There is at least one thread waiting to receive on this channel.
     Wake it up and set value register of thread so it gets the value
     written when it returns. */
  TSTATE(thr) = Tready;
  WB(&TVALR(thr),  __recv_channel(c, thr, chan));
  /* this causes the thread we woke up to restore the continuation
     created when it called arc_recv_channel. */
  arc_return(c, thr);
  /* This will return the value val to the thread which called
     arc_send_channel. */
  return(val);
}

/* Used for sending to the receive channel.  This channel is not
   like normal channels, in that (1) writing to it is unrestricted
   and (2) all receivers on the channel will wake up at the same time
   and receive the same value when this is called. */
static value __send_rvch(arc *c, value chan, value data)
{
  value thr;

  VINDEX(chan, 0) = CTRUE;
  VINDEX(chan, 1) = data;

  /* dequeue and wake up */
  while ((thr = dequeue(c, &VINDEX(chan, 2), &VINDEX(chan, 3))) != CNIL) {
    TSTATE(thr) = Tready;
    WB(&TVALR(thr), data);
    arc_return(c, thr);
  }
  return(data);
}

value arc_recv_channel(arc *c, value chan)
{
  TYPECHECK(chan, T_CHAN, 1);
  return(__recv_channel(c, c->curthread, chan));
}

value arc_send_channel(arc *c, value chan, value data)
{
  TYPECHECK(chan, T_CHAN, 1);
  return(__send_channel(c, c->curthread, chan, data));
}

value arc_atomic_cell(arc *c, int argc, value *argv)
{
  if (argc == 0)
    return((TACELL(c->curthread) == 0) ? CNIL : CTRUE);
  if (argc != 1) {
    arc_err_cstrfmt(c, "__acell__: wrong number of arguments (%d for 0 or 1)\n", argc);
    return(CNIL);
  }
  TACELL(c->curthread) = (argv[0] == CNIL) ? 0 : 1;
  return(argv[0]);
}

value arc_atomic_chan(arc *c)
{
  return(c->achan);
}

value arc_dead(arc *c, value thr)
{
  TYPECHECK(thr, T_THREAD, 1);
  return((TSTATE(thr) == Trelease || TSTATE(thr) == Tbroken) ? CTRUE : CNIL);
}

value arc_tjoin(arc *c, value thr)
{
  if (TYPE(TRVCH(thr)) != T_CHAN)
    return(TRVCH(thr));
  return(arc_recv_channel(c, TRVCH(thr)));
}

/* This will terminate a thread with extreme prejudice.  Be wary of
   using it: a thread blocked on I/O may do strange things, and no
   protect clauses or other cleanup associated with the thread will
   execute!  arc_tjoin on a thread thus killed will return undefined
   results. */
value arc_kill_thread(arc *c, value thr)
{
  TSTATE(thr) = Tbroken;
  return(thr);
}
