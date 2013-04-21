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

/* A channel is again a vector with the following entries:
   0 - whether the channel has data or not (CTRUE or CNIL)
   1 - Data in the channel, if any (whatever)
   2 - Head of list of threads waiting to receive from the channel (cons)
   3 - Tail of list of threads waiting to receive from the channel
   4 - Head of list of threads waiting to send to the channel (cons)
       This is actually a list of thread-value pairs.
   5 - Tail of list of threads waiting to send to the channel
 */

#define XCHAN_RHEAD(chan) (REP((chan))[3])
#define XCHAN_RTAIL(chan) (REP((chan))[4])
#define XCHAN_SHEAD(chan) (REP((chan))[5])
#define XCHAN_STAIL(chan) (REP((chan))[6])

#define CHAN_HASDATA(chan) (VINDEX(chan, 0))
#define CHAN_DATA(chan) (VINDEX(chan, 1))
#define CHAN_RHEAD(chan) (VINDEX(chan, 2))
#define CHAN_RTAIL(chan) (VINDEX(chan, 3))
#define CHAN_SHEAD(chan) (VINDEX(chan, 4))
#define CHAN_STAIL(chan) (VINDEX(chan, 5))

#define SCHAN_HASDATA(chan, val) (SVINDEX(chan, 0, val))
#define SCHAN_DATA(chan, val) (SVINDEX(chan, 1, val))
#define SCHAN_RHEAD(chan, val) (SVINDEX(chan, 2, val))
#define SCHAN_RTAIL(chan, val) (SVINDEX(chan, 3, val))
#define SCHAN_SHEAD(chan, val) (SVINDEX(chan, 4, val))
#define SCHAN_STAIL(chan, val) (SVINDEX(chan, 5, val))
#define CHAN_SIZE 6

value arc_mkchan(arc *c)
{
  value chan = arc_mkvector(c, CHAN_SIZE);

  ((struct cell *)chan)->_type = T_CHAN;
  SCHAN_HASDATA(chan, CNIL);
  SCHAN_DATA(chan, CNIL);
  SCHAN_RHEAD(chan, CNIL);
  SCHAN_RTAIL(chan, CNIL);
  SCHAN_SHEAD(chan, CNIL);
  SCHAN_STAIL(chan, CNIL);
  return(chan);
}

static AFFDEF(chan_pprint)
{
  AARG(sexpr, disp, fp);
  AOARG(visithash);
  AFBEGIN;
  (void)disp;
  (void)sexpr;
  AFTCALL(arc_mkaff(c, __arc_disp_write, CNIL), arc_mkstringc(c, "#<chan>"),
	  CTRUE, AV(fp), AV(visithash));
  AFEND;
}
AFFEND

AFFDEF(arc_recv_channel)
{
  AARG(chan);
  value val, xthr;
  AFBEGIN;

  TYPECHECK(AV(chan), T_CHAN);
  while (NIL_P(CHAN_HASDATA(AV(chan)))) {
    /* We have no value that can be received from the channel.  Enqueue
       the calling thread and freeze it into Trecv state.  This should
       never happen with a recursive call to arc_recv_channel. */
    __arc_enqueue(c, thr, &XCHAN_RHEAD(AV(chan)), &XCHAN_RTAIL(AV(chan)));
    TSTATE(thr) = Trecv;
    AYIELD();
  }

  /* If we get here, there is a value that can be received from the
     channel. Read it, and see if there is any thread waiting to send. */
  SCHAN_HASDATA(AV(chan), CNIL);
  val = CHAN_DATA(AV(chan));
  xthr = __arc_dequeue(c, &XCHAN_SHEAD(AV(chan)), &XCHAN_STAIL(AV(chan)));
  if (!NIL_P(xthr)) {
    /* There is at least one thread waiting to send on this channel.
       Wake it up so it can send already. */
    TSTATE(xthr) = Tready;
  }
  ARETURN(val);
  AFEND;
}
AFFEND

AFFDEF(arc_send_channel)
{
  AARG(chan, val);
  value xthr;
  AFBEGIN;

  TYPECHECK(AV(chan), T_CHAN);
  while (!NIL_P(CHAN_HASDATA(AV(chan)))) {
    /* There is a value in the channel that was written that has not
       yet been read.  Enqueue the thread. */
    __arc_enqueue(c, thr, &XCHAN_SHEAD(AV(chan)), &XCHAN_STAIL(AV(chan)));
    /* Change state to Tsend, which is not runnable, and toss us back
       to the virtual machine loop, which will see we are no longer
       runnable and return us to the dispatcher so some other thread
       can be made to run instead. */
    TSTATE(thr) = Tsend;
    AYIELD();
  }

  /* If we get here, we are clear to send to the channel. Read it, and
     see if there is any thread waiting to receive. */
  SCHAN_HASDATA(AV(chan), CTRUE);
  SCHAN_DATA(AV(chan), AV(val));
  xthr = __arc_dequeue(c, &XCHAN_RHEAD(AV(chan)), &XCHAN_RTAIL(AV(chan)));
  if (!NIL_P(xthr)) {
    /* There is at least one thread waiting to receive on this channel.
       Wake it up so it can receive. */
    TSTATE(xthr) = Tready;
  }
  ARETURN(AV(val));
  AFEND;
}
AFFEND


/* A thread's RVCHAN has slightly different behaviour from a normal
   channel.  When such a channel is written to, all threads waiting
   on it will wake up simultaneously. */
AFFDEF(__arc_recv_rvchan)
{
  AARG(chan);
  AFBEGIN;

  TYPECHECK(AV(chan), T_CHAN);
  while (NIL_P(CHAN_HASDATA(AV(chan)))) {
    /* We have no value that can be received from the channel.  Enqueue
       the calling thread and freeze it into Trecv state.  This should
       never happen with a recursive call to arc_recv_channel. */
    __arc_enqueue(c, thr, &XCHAN_RHEAD(AV(chan)), &XCHAN_RTAIL(AV(chan)));
    TSTATE(thr) = Trecv;
    AYIELD();
  }

  /* Return the channel data */
  ARETURN(CHAN_DATA(AV(chan)));
  AFEND;
}
AFFEND

value __arc_send_rvchan(arc *c, value chan, value val)
{
  value xthr;

  SCHAN_HASDATA(chan, CTRUE);
  SCHAN_DATA(chan, val);
  ;
  while ((xthr = __arc_dequeue(c, &XCHAN_RHEAD(chan), &XCHAN_RTAIL(chan))) != CNIL) {
    /* There is at least one thread waiting to receive on this channel.
       Wake it up so it can receive. */
    TSTATE(xthr) = Tready;
  }
  return(val);
}

typefn_t __arc_chan_typefn__ = {
  __arc_vector_marker,
  __arc_null_sweeper,
  chan_pprint,
  NULL,
  NULL,
  __arc_vector_isocmp,
  NULL,
  NULL
};
