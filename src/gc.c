/* 
  Copyright (C) 2017,2018 Rafael R. Sevilla

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
#include <stdlib.h>
#include "arcueid.h"
#include "alloc.h"
#include "gc.h"

value arc_new(arc *c, arctype *t, size_t size)
{
  struct GChdr *obj;
  struct mm_ctx *mc = (struct mm_ctx *)c->mm_ctx;
  struct gc_ctx *gcc = (struct gc_ctx *)c->gc_ctx;

  obj = (struct GChdr *)__arc_alloc(mc, GCHDR_ALIGN_SIZE + size);
  /* Initialize type, colour, and link it into the list of all
     allocated objects. */
  obj->t = t;
  obj->colour = gcc->mutator;
  obj->next = gcc->gcobjects;
  gcc->gcobjects = obj;
  return((value)(obj->_data + GCHPAD));  
}

void arc_wb(arc *c, value dest, value src)
{
  struct gc_ctx *gcc = (struct gc_ctx *)c->gc_ctx;
  struct GChdr *gh;

  /* Do nothing if dest is a non-pointer (immediate) value */
  if (IMMEDIATEP(dest))
    return;

  /* The VCGC write barrier requires that we mark the destination with
     the propagator */
  V2GCH(gh, dest);
  gh->colour = PROPAGATOR;
  gcc->nprop = 1;
}
