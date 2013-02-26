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
  TIP(thr) = NULL;
  TARGC(thr) = 0;

  TSTATE(thr) = Tready;
  TTID(thr) = ++c->tid_nonce;
  TQUANTA(thr) = 0;
  TTICKS(thr) = 0LL;
  TWAKEUP(thr) = 0LL;
  return(thr);
}
