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

   [1] William D. Clinger, Anne H. Hartheimer, and Eric M. Ost.
       Implementation Strategies for continuations.  In Proceedings
       Record of the 1988 ACM Symposium on Lisp and Functional
       Programming, ACM, 1988. DOI 10.1145/62678.62692
 */
#include "arcueid.h"
#include "vmengine.h"

void __arc_mkenv(arc *c, value thr, int prevsize, int extrasize)
{
  int i;
  arc_thread *t = (arc_thread *)thr;

  /* Add the extra environment entries */
  for (i=0; i<extrasize; i++)
    CPUSH(thr, CUNBOUND);
  /* Add the count */
  CPUSH(thr, INT2FIX(prevsize+extrasize));
  envstart = thr->sp;
}
