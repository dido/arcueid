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

/*! \file gc.h
    \brief Definitions for Arcueid's garbage collector.
 */

#ifndef _GC_H_

#define _GC_H_

#include "arcueid.h"

/*! \struct GChdr
    \brief Garbage collector header

    Memory blocks allocated by the garbage collector are prefixed
    by this header.
 */
struct GChdr {
  arctype *t;			/*!< The type of the object  */
  struct GChdr *next;		/*!< Next allocated object */
  int colour;			/*!< Garbage collector colour */
  char _data[1];		/*!< Pointer to data of the block itself */
};

/*! \struct gc_ctx
    \brief Garbage collector context
 */
struct gc_ctx {
  unsigned long long gc_milliseconds; /*!< Milliseconds spent in
                                         garbage collector */
  unsigned long long gcepochs;	/*!< number of garbage collection
                                   epochs so far  */
  unsigned long long gcnruns;	/*!< number of gc runs */
  unsigned long long gccolour;	/*!< garbage collector colour  */
  
  int gcquantum;		/*!< Garbage collector visit max
                                   quantum */
  struct GChdr *gcptr;		/*!< running pointer used by
				  collector */
  int visit;			/*!< visited node count */
  struct GChdr *gcobjects;	/*!< List of all allocated objects */
  int nprop;			/*!< Flag set if new propagator was
                                   created since the start of the
                                   GC cycle */
  int mutator;			/*!< Current mutator colour  */
  int marker;			/*!< Current marker colour  */
  int sweeper;			/*!< Current sweeper colour */
  int gce;			/*!< GC stat, decremented when
                                   propagators are found, incremented
                                   when objects are swept.  */
  int gct;			/*!< GC stat, incremented for every
                                   mutator/marker colour object found. */
};

/*! \def PROPAGATOR
    \brief Propagator colour
 */
#define PROPAGATOR 3

/*! \def MAX_MARK_RECURSION
    \brief Maximum depth of recursive marking
 */
#define MAX_MARK_RECURSION (16)

/*! \def GCQUANTA
    \brief Basic garbage collection visit quantum
 */
#define GCQUANTA (64)

/*! \def GCMAXQUANTA
    \brief Maximum garbage collection visit quantum
 */
#define GCMAXQUANTA (GCQUANTA*64)


/*! \def GCHDRSIZE
    \brief Actual size of a GC header.

    Excluding the dummy _data piece, the size of a GChdr structure.
 */
#define GCHDRSIZE ((long)(((struct GChdr *)0)->_data))


/*! \def GCHDR_ALIGN_SIZE
    \brief Alignment size for a GChdr
 */
#define GCHDR_ALIGN_SIZE (ALIGN_SIZE(GCHDRSIZE))

/*! \def GCPAD
    \brief GC header padding
 */
#define GCHPAD (GCHDR_ALIGN_SIZE - GCHDRSIZE)

/*! \def V2GCH
    \brief Value to GC header
    Given a value _v_, assign the corresponding GChdr to _gh_.
 */
#define V2GCH(gh, v) (gh) = (struct GChdr *)(((char *)(v)) - (char *)(GCHDRSIZE + GCHPAD))

/*! \def GCH2V(gh)
    \brief GChdr to value
    Given a GChdr _gh_, return the corresponding value
 */
#define GCH2V(gh) ((value)((gh)->_data + GCHPAD))

/*! \fn void __arc_gc(arc *c)
    \brief Garbage collector entry point

    This function runs a garbage collector cycle.
 */
extern int __arc_gc(arc *c);

/*! \fn struct gc_ctx* __arc_new_gc_ctx(arc *c)
    \brief Create a new garbage collector context
 */
extern struct gc_ctx *__arc_new_gc_ctx(arc *c);

/*! \fn void __arc_free_gc_ctx(arc *c, struct gc_ctx *gcc)
    \brief Free a garbage collector context
 */
extern void __arc_free_gc_ctx(arc *c, struct gc_ctx *gcc);

#endif