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

  V2GCH(gh, val);
  gh->colour = PROPAGATOR;
  gcc->nprop = 1;
}

void arc_wb(arc *c, value dest, value src)
{
  /* The VCGC write barrier requires that we mark the destination with
     the propagator */
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

/* This is the VCGC algorithm. This particular implementation is
   inspired by the one used by the Inferno OS. The GC roots and
   objects affected by the write barrier are marked with the
   propagator colour, which the algorithm interprets as part of the
   root set from which objects ought to be marked with the mutator
   colour.

   The algorithm works by going over the entire allocated heap, which
   is set up as a linked list. If it sees an object there which is
   marked with the propagator colour, it will recursively mark that
   object with the current mutator colour. It will stop recursing at
   a designated maximum depth, or if the algorithm's visit quantum
   runs out. In that case, the last such object encountered is
   likewise marked with the propagator colour instead so it can be
   picked up in a future GC run.

   An object with the sweeper colour is unlinked from the list of
   allocated objects and freed.

   Objects in the current mutator colour are ignored.

   Once all of the allocated objects have been visited it checks the
   new propagator flag (nprop) to see if any new propagators have been
   generated in between GC action. If there have been any new
   propagators created, the flag is cleared first, and a new sweep of
   the allocated heap is done, to find these propagators and mark
   them. If no new propagators have been created, a garbage collection
   epoch is declared ended, the mutator, marker, and sweeper colours
   are changed, and the root set is again marked.
 */
int __arc_gc(arc *c)
{
  struct gc_ctx *gcc = (struct gc_ctx *)c->gc_ctx;
  struct mm_ctx *mc = (struct mm_ctx *)c->mm_ctx;
  unsigned long long gcst, gcet;
  struct GChdr *v;
  int retval = 0;

  gcc->gcnruns++;
  gcst = __arc_milliseconds();
  if (gcc->gcptr == NULL) {
    gcc->gcpptr = NULL;
    gcc->gcptr = gcc->gcobjects;
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
      if (gcc->gcpptr == NULL) {
	/* delete from the head of the list */
	gcc->gcobjects = gcc->gcptr->next;
      } else {
	/* delete from the body of the list */
	gcc->gcpptr->next = gcc->gcptr->next;
      }
      gcc->gcptr = gcc->gcptr->next;
      __arc_free(mc, v);
      continue;
    } else {
      gcc->gct++;
    }
    gcc->gcpptr = gcc->gcptr;
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

  if (gcc->gcptr == NULL) {
    /* We have completed the epoch if we have gone through all the allocated
       objects and there are no more active propagators. When that happens,
       we increment the number of epochs, change the GC colour, and mark
       all of the roots again in preparation for the beginning of the next
       garbage collection epoch. */
    if (gcc->nprop == 0) {
      retval = 1;
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
    /* clear the propagator flag if we get to the end. */
    gcc->nprop = 0;
  }

  gcet = __arc_milliseconds();
  gcc->gc_milliseconds += gcet - gcst;
  return(retval);
}

struct gc_ctx *__arc_new_gc_ctx(arc *c)
{
  struct mm_ctx *mc = (struct mm_ctx *)c->mm_ctx;
  struct gc_ctx *gcc;

  gcc = (struct gc_ctx *)__arc_alloc(mc, sizeof(struct gc_ctx));
  gcc->gc_milliseconds = 0ULL;
  gcc->gcepochs = 0ULL;
  gcc->gcnruns = 0ULL;
  gcc->gccolour = 3ULL;
  gcc->gcquantum = GCQUANTA;
  gcc->gcptr = NULL;
  gcc->visit = 0;
  gcc->gcobjects = NULL;
  gcc->nprop = 0;
  gcc->mutator = gcc->gccolour % 3;
  gcc->marker = (gcc->gccolour-1) % 3;
  gcc->sweeper = (gcc->gccolour-2) % 3;
  gcc->gce = 0;
  gcc->gct = 1;
  return(gcc);
}

void __arc_free_gc_ctx(arc *c, struct gc_ctx *gcc)
{
  struct mm_ctx *mc = (struct mm_ctx *)c->mm_ctx;
  __arc_free(mc, gcc); 
}
