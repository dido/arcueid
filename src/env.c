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

/* Arcueid's environments come in two flavours:
   1. Stack-based environments
   2. Heap-based environments

   Function application in Arcueid basically boils down to doing the
   following:

   1. Create a continuation, saving the contents of the environment
      register, instruction pointer (setting it up so it points to
      just after the actual apply instruction), and the current value
      of the continuation register.  This goes on the stack.
   2. Push the arguments of the call onto the stack.
   3. Put the function itself (which can be either a closure or a
      foreign function) into the accumulator.
   4. Transfer control to the called function.

   It is to be noted that after the saved continuation on the stack,
   the passed parameters will be used as the environment of the
   applied function itself once control is transferred to it, with
   some additional information, most importantly the actual size of
   the environment, which is stored as a number just after the last
   stack entry and a pointer to the lexically superior environment, if
   any.

   If an environment needs to be saved on the heap it is copied into a
   vector, and all lexically superior environments need to be similarly
   copied.  Essentially boils down to crunching the stack so that any
   environments that were directly stored on it are copied to the heap.

   This may happen as well when the stack actually fills up.  All
   environments on the stack may be moved to the heap.

   This strategy is what Clinger et. al. [1] call the stack/heap
   strategy.

   Environments that have been saved on the heap are Arcueid vectors
   with a specific structure. The 0th element always points to the
   next lexically superior environment, nil when there are no further
   such environments. The remaining elements are the

   The environment register in a thread contains either a fixnum,
   which is interpreted as an absolute offset into the stack pointing
   to the beginning of the environment, or a vector, which is the
   environment above as described.

   [1] William D. Clinger, Anne H. Hartheimer, and Eric M. Ost.
       Implementation Strategies for continuations.  In Proceedings
       Record of the 1988 ACM Symposium on Lisp and Functional
       Programming, ACM, 1988. DOI 10.1145/62678.62692
 */
#include "arcueid.h"
#include "vmengine.h"

/* The basic structure of a stack-based environment is as follows, from
   higher to lower stack offsets, since the stack grows down:

   1. Parameters (prevsize)
   2. Local variables (extrasize)
   3. Environment size as a fixnum (prevsize + extrasize)
   4. Next lexically superior environment

   A stack-based environment is an absolute fixnum offset from the top of
   the stack to the start of the environment.
 */
void __arc_env_new(arc *c, value thr, int prevsize, int extrasize)
{
  int i;
  arc_thread *t = (arc_thread *)thr;
  value esofs;
  value *envstart;

  /* Add the extra environment entries */
  for (i=0; i<extrasize; i++)
    CPUSH(thr, CUNBOUND);
  /* Add the count */
  CPUSH(thr, INT2FIX(prevsize+extrasize)); /* environment size */
  envstart = t->sp;
  CPUSH(thr, t->env);		/* next lexically superior environment */
  esofs = INT2FIX(t->stktop - envstart);
  t->stkfn = t->sp;
  arc_wb(c, t->env, esofs);
  t->env = esofs;
}

#define HEAPENVP(env) (arc_type(env) == &__arc_vector_t)

static value nextenv(arc *c, value thr, value env)
{
  value *envptr;
  arc_thread *t;

  if (NILP(env))
    return(CNIL);

  if (HEAPENVP(env))
    return(VIDX(env, 0));

  t = (arc_thread *)thr;
  /* We have a stack-based environment. Get the address of the
     environment pointer from the stack. */
  envptr = t->stktop - FIX2INT(env);
  return(*envptr);
}
  

/* Get a pointer to a value from the environment. This works whether
   an environment is on the stack or the heap. */
static value *envval(arc *c, value thr, int depth, int index)
{
  arc_thread *t = (arc_thread *)thr;  
  value *senv, *senvstart, env;
  int count;

  env = t->env;
  /* Find the environment frame being referenced */
  while (depth-- > 0 && !NILP(env))
    env = nextenv(c, thr, env);

  if (NILP(env))
    return(CNIL);

  if (HEAPENVP(env)) {
    value *vec = (value *)env;
    return(vec + index + 2);
  }

  /* For a stack-based environment, we have to do some gymnastics */
  senv = t->stktop - FIX2INT(env);
  count = FIX2INT(*(senv + 1));
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
  arc_wb(c, *ptr, val);
  *ptr = val;
  return(val);
}

value __arc_getenv0(arc *c, value thr, int iindx)
{
  arc_thread *t = (arc_thread *)thr;
  value *base;
  int count;

  if (HEAPENVP(t->env))
    return(VIDX(t->env, iindx+1));
  base = t->stktop - FIX2INT(t->env);
  count = FIX2INT(*(base + 1));
  return(*(base + count + 1 - iindx));
}

value __arc_putenv0(arc *c, value thr, int iindx, value val)
{
  arc_thread *t = (arc_thread *)thr;
  value *base, *ptr;
  int count;

  if (HEAPENVP(t->env))
    return(SVIDX(c, t->env, iindx+1, val));
  base = t->stktop - FIX2INT(t->env);
  count = FIX2INT(*(base + 1));
  ptr = base + count + 1 - iindx;
  arc_wb(c, *ptr, val);
  return(*ptr = val);
}
