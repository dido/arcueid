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

#define CHAN_HASDATA(chan) (VINDEX(chan, 0))
#define CHAN_DATA(chan) (VINDEX(chan, 1))
#define CHAN_RHEAD(chan) (VINDEX(chan, 2))
#define CHAN_RTAIL(chan) (VINDEX(chan, 3))
#define CHAN_SHEAD(chan) (VINDEX(chan, 4))
#define CHAN_STAIL(chan) (VINDEX(chan, 5))
#define CHAN_SIZE 6

value arc_mkchan(arc *c)
{
  value chan = arc_mkvector(c, CHAN_SIZE);

  ((struct cell *)chan)->_type = T_CHAN;
  CHAN_HASDATA(chan) = CNIL;
  CHAN_DATA(chan) = CNIL;
  CHAN_RHEAD(chan) = CNIL;
  CHAN_RTAIL(chan) = CNIL;
  CHAN_SHEAD(chan) = CNIL;
  CHAN_STAIL(chan) = CNIL;
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
       never happen with a recursive call arc_recv_channel. */
    __arc_enqueue(c, thr, &CHAN_RHEAD(AV(chan)), &CHAN_RTAIL(AV(chan)));
    TSTATE(thr) = Trecv;
    AYIELD();
  }

  /* If we get here, there is a value that can be received from the
     channel. Read it, and see if there is any thread waiting to send. */
  CHAN_HASDATA(AV(chan)) = CNIL;
  val = CHAN_DATA(AV(chan));
  xthr = __arc_dequeue(c, &CHAN_SHEAD(AV(chan)), &CHAN_STAIL(AV(chan)));
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
    __arc_enqueue(c, thr, &CHAN_SHEAD(AV(chan)), &CHAN_STAIL(AV(chan)));
    /* Change state to Tsend, which is not runnable, and toss us back
       to the virtual machine loop, which will see we are no longer
       runnable and return us to the dispatcher so some other thread
       can be made to run instead. */
    TSTATE(thr) = Tsend;
    AYIELD();
  }

  /* If we get here, we are clear to send to the channel. Read it, and
     see if there is any thread waiting to receive. */
  CHAN_HASDATA(AV(chan)) = CTRUE;
  CHAN_DATA(AV(chan)) = AV(val);
  xthr = __arc_dequeue(c, &CHAN_RHEAD(AV(chan)), &CHAN_RTAIL(AV(chan)));
  if (!NIL_P(xthr)) {
    /* There is at least one thread waiting to receive on this channel.
       Wake it up so it can receive. */
    TSTATE(xthr) = Tready;
  }
  ARETURN(AV(val));
  AFEND;
}
AFFEND

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
