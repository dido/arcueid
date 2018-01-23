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
#include "alloc.h"
#include "gc.h"

/*! \fn static void wfree(arc *c, value wr)
    \brief Finaliser for weak references
 */
static void wfree(arc *c, value wr)
{
  value *ref = (value *)wr;
  struct GChdr *gh;

  /* Nothing to do if the reference is already gone */
  if (NILP(*ref))
    return;

  /* If the reference is not gone, we have to clear out the weak reference
     in the GC header of the referent. */
  V2GCH(gh, *ref);
  gh->wref = CNIL;
}

/* Weak references have only a free function, no mark function
   (because this is a weak reference and obviously its referent should
   not be marked!) and are (for the moment) not hashable. */
arctype __arc_wref_t = { wfree, NULL, NULL, NULL, NULL, NULL, NULL };

value arc_wref_new(arc *c, value v)
{
  value wref, *ref;
  struct GChdr *gh;

  /* See if the GC header of v already has a wref. If so, return it. */
  V2GCH(gh, v);

  if (!NILP(gh->wref))
    return(gh->wref);

  /* If there is no wref, create a new one and store it in the field */
  wref = arc_new(c, &__arc_wref_t, sizeof(value));
  ref  = (value *)wref;
  *ref = v;
  gh->wref = wref;
  return(wref);
}

value arc_wrefv(value wr)
{
  return(*((value *)wr));
}

