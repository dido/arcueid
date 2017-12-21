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
  return(GCH2V(obj));
}

static void markprop(arc *c, value val)
{
  struct gc_ctx *gcc = (struct gc_ctx *)c->gc_ctx;
  struct GChdr *gh;

  /* Do nothing if dest is a non-pointer (immediate) value */
  if (IMMEDIATEP(val))
    return;

  /* The VCGC write barrier requires that we mark the destination with
     the propagator */
  V2GCH(gh, val);
  gh->colour = PROPAGATOR;
  gcc->nprop = 1;
}

void arc_wb(arc *c, value dest, value src)
{
  markprop(c, dest);
}

static void mark(arc *c, value v, int depth)
{
  struct gc_ctx *gcc = (struct gc_ctx *)c->gc_ctx;
  struct GChdr *gh;

  /* Ignore immediate objects */
  if (IMMEDIATEP(v))
    return;

  V2GCH(gh, v);
  /* Mark the object with the propagator colour if it is not already
     in the mutator colour */
  if (gh->colour != gcc->mutator) {
    gh->colour = PROPAGATOR;
    gcc->nprop = 1;
  }

  /* If we can still visit more objects, and we are not yet at
     maximum recursion depth, change the colour of the object to 
     the mutator colour, and then recurse into the object's structure
     at increased depth. */
  if (--gcc->visit >= 0 && depth < MAX_MARK_RECURSION) {
    gcc->gce--;
    gh->colour = gcc->mutator;
    /* Recurse into the object's structure at increased depth */
    gh->t->mark(c, v, mark, depth+1);
  }
}

int __arc_gc(arc *c)
{
  struct gc_ctx *gcc = (struct gc_ctx *)c->gc_ctx;
  struct mm_ctx *mc = (struct mm_ctx *)c->mm_ctx;
  unsigned long long gcst, gcet;
  struct GChdr *v;
  int retval = 0;

  gcst = __arc_milliseconds();
  if (gcc->gcptr == NULL) {
    gcc->gcptr = gcc->gcobjects;
    gcc->gcpptr = NULL;
  }

  for (gcc->visit = gcc->gcquantum; gcc->visit > 0;) {
    if (gcc->gcptr == NULL)
      break;			/* last heap block */
    v = gcc->gcptr;
    if (v->colour == PROPAGATOR) {
      gcc->gce--;
      /* recursively mark propagators */
      mark(c, GCH2V(v), 0);
    } else if (v->colour == gcc->sweeper) {
      /* If an object has the sweeper colour, call the object's
	 free function, unlink it from the list, and free the
	 object. */
      gcc->gce++;
      v->t->free(c, GCH2V(v));
      gcc->gcptr = v->next;
      __arc_free(mc, v);
    } else {
      gcc->gct++;
    }
    gcc->gcpptr = v;
    gcc->gcptr = gcc->gcptr->next;
  }
  gcc->gcquantum = (GCMAXQUANTA - GCQUANTA)/2
    + ((GCMAXQUANTA - GCQUANTA)/20)*100*gcc->gce/gcc->gct;
  if (gcc->gcquantum < GCQUANTA)
    gcc->gcquantum = GCQUANTA;

  if (gcc->gcquantum > GCMAXQUANTA)
    gcc->gcquantum = GCMAXQUANTA;

  /* If we have not yet exhausted the entire allocated heap but have
     run out of visit quanta, stop the current GC cycle for now. */
  if (gcc->gcptr != NULL)
    goto endgc;

  /* We have completed the epoch if we have gone through all the allocated
     objects and there are no more active propagators. When that happens,
     we increment the number of epochs, change the GC colour, and mark
     all of the roots again in preparation for the beginning of the next
     garbage collection epoch. */
  if (gcc->nprop == 0) {
    retval = (gcc->gccolour % 3) == 0;
    gcc->gcepochs++;
    gcc->gccolour++;
    gcc->mutator = gcc->gccolour % 3;
    gcc->marker = (gcc->gccolour-1) % 3;
    gcc->sweeper = (gcc->gccolour-2) % 3;
    gcc->gce = 0;
    gcc->gct = 1;
    c->markroots(c, markprop);
    /* XXX This would be a good time to free any unused BiBOP pages */
#ifdef HAVE_MALLOC_TRIM
    malloc_trim(0);
#endif
  }
  gcc->nprop = 0;

 endgc:
  gcet = __arc_milliseconds();
  gcc->gc_milliseconds += gcet - gcst;
  return(retval);
}
