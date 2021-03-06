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

/* Arcueid's environments come in two flavours:
   1. Stack-based environments
   2. Heap-based environments

   Function application in Arcueid basically boils down to doing the
   following:

   1. Create a continuation, saving the contents of the environment
      register, instruction pointer (setting it up so it points to
      just after the actual apply instruction), and the current value
      of the continuation register.  This goes on the stack.
   2. Push the arguments of the continuation onto the stack.
   3. Put the function itself (which can be either a closure or a
      foreign function) into the value register.
   4. Transfer control to the called function.

   It is to be noted that after the saved continuation on the stack,
   the passed parameters may actually be used as the environment
   itself once control is transferred to the applied function,
   with some additional information, most importantly the actual
   size of the environment, which is stored as a number just after
   the last stack entry and a pointer to the lexically superior
   environment, if any.

   If an environment needs to be saved on the heap it is copied into a
   vector, and all lexically superior environments need to be similarly
   copied.  Essentially boils down to crunching the stack so that any
   environments that were directly stored on it are copied to the heap.

   This may happen as well when the stack actually fills up.  All
   environments on the stack may be moved to the heap.

   This strategy is what Clinger et. al. [1] call the stack/heap
   strategy.

   Environments that have been saved on the heap are Arcueid vectors
   with a specific structure:

   [1] William D. Clinger, Anne H. Hartheimer, and Eric M. Ost.
       Implementation Strategies for continuations.  In Proceedings
       Record of the 1988 ACM Symposium on Lisp and Functional
       Programming, ACM, 1988.
 */
#include <string.h>
#include "arcueid.h"
#include "vmengine.h"

/* Make a new environment on the stack.  The prevsize parameter is the
   count of objects already on the stack that become part of the
   environment that is being created, and extrasize are the additional
   elements on the stack that are to be added.  These additional
   elements have the initial value CUNBOUND. */
void __arc_mkenv(arc *c, value thr, int prevsize, int extrasize)
{
  value *envstart;
  int i, esofs;

  /* Add the extra environment entries */
  for (i=0; i<extrasize; i++)
    CPUSH(thr, CUNBOUND);
  /* Add the count */
  CPUSH(thr, INT2FIX(prevsize+extrasize));
  envstart = TSP(thr);
  CPUSH(thr, TENVR(thr));
  esofs = TSTOP(thr) - envstart;
  TSFN(thr) = TSP(thr);		/* start of stack after env */
  SENVR(thr, ((value)((long)(esofs << 4) | ENV_FLAG)));
}

#define SENV_PTR(stktop, env) ((stktop) - ((int)((env) >> 4)))
#define SENV_COUNT(base) (FIX2INT(*(base + 1)))

#define VENV_NEXT(x) (VINDEX((x), 0))
#define SVENV_NEXT(x, v) (SVINDEX((x), 0, v))
#define VENV_INDEX(x, i) (XVINDEX((x), (i)+1))
#define VENV_CREATE(c, count) (arc_mkvector(c, count+1));

static value nextenv(arc *c, value thr, value env)
{
  value *envptr;

  if (env == CNIL)
    return(CNIL);

  if (TYPE(env) == T_VECTOR)
    return(VENV_NEXT(env));

  /* We have a stack-based environment.  Get the address of the
     environment pointer from the stack. */
  envptr = SENV_PTR(TSTOP(thr), env);
  return(*envptr);
}

/* Get a value from the environment pointer given. */
static value *envval(arc *c, value thr, int depth, int index)
{
  value *senv, *senvstart, env;
  int count;

  env = TENVR(thr);

  /* Find the environment frame being referenced */
  while (depth-- > 0 && !NIL_P(env))
    env = nextenv(c, thr, env);

  if (NIL_P(env))
    return(CNIL);

  if (TYPE(env) == T_VECTOR)
    return(&VENV_INDEX(env, index));

  /* For a stack-based environment, we have to do some gymnastics */
  senv = SENV_PTR(TSTOP(thr), env);
  count = SENV_COUNT(senv);
  senvstart = senv + count + 1;
  return(senvstart - index);
}

value __arc_getenv(arc *c, value thr, int depth, int index)
{
  return(*envval(c, thr, depth, index));
}

value __arc_putenv(arc *c, value thr, int depth, int index, value val)
{
  value *ptr = envval(c, thr, depth, index);
  __arc_wb(*ptr, val);
  *ptr = val;
  return(val);
}

inline value __arc_getenv0(arc *c, value thr, int iindx)
{
  if (ENV_P(TENVR(thr))) {
    value *base = SENV_PTR(TSTOP(thr), TENVR(thr));
    int count = FIX2INT(*(base + 1));
    return(*(base + count + 1 - iindx));
  }
  return(XVINDEX(TENVR(thr), iindx+1));
}

inline value __arc_putenv0(arc *c, value thr, int iindx, value val)
{
  value *ptr;

  if (ENV_P(TENVR(thr))) {
    value *base = SENV_PTR(TSTOP(thr), TENVR(thr));
    int count = FIX2INT(*(base + 1));
    ptr = (base + count + 1 - iindx);
  } else {
    ptr = &XVINDEX(TENVR(thr), iindx+1);
  }
  __arc_wb(*ptr, val);
  *ptr = val;
  return(val);
}

/* Move n elements from the top of stack, overwriting the current
   environment.  Points the stack pointer to just above the last
   element moved.  Does not do this if current environment is on
   the heap.  It will, in either case, set the environment register to
   the parent environment (possibly leaving the heap environment as
   garbage to be collected)

   This is essentially used to support tail calls and tail recursion.
   When this mechanism is used, the new arguments are on the stack,
   over the previous environment.  It will move all of the new arguments
   onto the old environment and adjust the stack pointer appropriately.
   When control is transferred to the tail called function the call
   to __arc_mkenv will turn the stuff on the stack into a new
   environment.
*/
void __arc_menv(arc *c, value thr, int n)
{
  value *src, *dest, *penv;
  value parentenv;
  int i;

  /* do nothing if we have no env */
  if (NIL_P(TENVR(thr)))
    return;

  parentenv = nextenv(c, thr, TENVR(thr));
  /* We should only bother if both the present environment and its
     parent (which should be superseded) are both on the stack. */
  if (TYPE(TENVR(thr)) == T_ENV) {
    /* Run the write barrier on the environment that is about to be
       overwritten.  Required for most incremental and concurrent GC
       algorithms. */
    penv = SENV_PTR(TSTOP(thr), TENVR(thr));
    for (i=0; i<SENV_COUNT(penv); i++)
      __arc_wb(*envval(c, thr, 0, i), CNIL);
    /* source of our copy is the last value pushed on the stack */
    src = TSP(thr)+1;
    /* Destination of our copy is the (n-1)th element of the environment.
       May be larger or smaller than the actual size of the environment.
       Doesn't matter, as long as it remains inside the stack! */
    dest = envval(c, thr, 0, n-1);
    /* use memmove because the new environment might be larger than the old
       one. */
    memmove(dest, src, n*sizeof(value));
    /* new stack pointer points to just outside previous one */
    TSP(thr) = dest-1;
  }
  /* If the current environment was on the heap, none of this black
     magic needs to be done.  It is enough to just set the environment
     register to point to the parent environment. */
  SENVR(thr, parentenv);
}

/* Convert a single stack environment into a heap-based environment */
static value heap_env(arc *c, value thr, value env)
{
  value *senv, *senvstart, henv;
  int count, i;

  senv = SENV_PTR(TSTOP(thr), env);
  count = SENV_COUNT(senv);
  henv = VENV_CREATE(c, count);
  senvstart = senv + count + 1;
  for (i=0; i<count; i++)
    VENV_INDEX(henv, i) = *(senvstart - i);
  SVENV_NEXT(henv, nextenv(c, thr, env));
  return(henv);
}

/* Move the current environment and all of its parent environments into
   the heap.  Also adjusts the environment pointers in continuations
   referring to them accordingly. */
value __arc_env2heap(arc *c, value thr, value env)
{
  value nenv = env, oldenv;

  env = CNIL;
  while (!NIL_P(nenv)) {
    /* Do nothing if the environment is already on the heap */
    oldenv = nenv;
    nenv = (TYPE(oldenv) == T_VECTOR) ? oldenv : heap_env(c, thr, oldenv);
    if (NIL_P(env))
      env = nenv;
    __arc_update_cont_envs(c, thr, oldenv, nenv);
    nenv = nextenv(c, thr, nenv);
  }
  return(env);
}
