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
#ifndef _ARCUEID_H_

#define _ARCUEID_H_

#include <stdlib.h>

typedef unsigned long value;

typedef struct arc {
  void *mm_ctx;			/* memory manager context */
  void *gc_ctx;			/* garbage collector context */
  void (*markroots)(struct arc *, void (*)(struct arc *, value));
} arc;

typedef struct arctype {
  void (*free)(arc *, value);
  void (*mark)(arc *, value, void (*)(arc *, value, int), int);
  int size;
} arctype;

/*! \def CNIL
    \brief The nil value
 */
#define CNIL ((value)0)

/*! \def IMMEDIATE_MASK
    \brief Mask for immediate values
 */
#define IMMEDIATE_MASK 0x0f

/*! \def IMMEDIATEP(x)
    \brief Predicate if _x_ is an immediate value
 */
#define IMMEDIATEP(x) (((value)(x) & IMMEDIATE_MASK) || (value)(x) == CNIL)

/* Memory management functions */

/*! \fn value arc_new(arc *c, arctype *t, size_t extrasize)
    \brief Allocate an Arc object
    \param c The Arc context
    \param t The type descriptor for the object
    \param size The size of the object
 */
extern value arc_new(arc *c, arctype *t, size_t size);

/*! \fn value arc_wb(arc *c, value dest, value src)
    \brief Garbage collector write barrier function
    \param c The Arc context
    \param dest Destination pointer
    \param src Source pointer

    The write barrier required by certain garbage collector
    algorithms. This should always be called before any pointer _dest_
    is overwritten by some operation.
 */
extern void arc_wb(arc *c, value dest, value src);

/* Definitions for Fixnums */
/*! \def FIXNUM_MAX
    \brief Largest possible fixnum value
 */
#define FIXNUM_MAX (LONG_MAX >> 1)
/*! \def FIXNUM_IN
    \brief Smallest possible (negative) fixnum value
 */
#define FIXNUM_MIN (-FIXNUM_MAX)
/*! \def FIXNUM_FLAG
    \brief Bit flag in a value denoting a fixnum

    A Fixnum is a value whose lowest bit is set to 1.
 */
#define FIXNUM_FLAG 0x01
/*! \def INT2FIX(i)
    \brief Convert an integer to a fixnum value
    \param i The value to be converted
    Converts a normal C integer value into a fixnum.
 */
#define INT2FIX(i) ((value)(((long)(i))<< 1 | FIXNUM_FLAG))
/*! \def FIX2INT(x)
    \brief Convert a fixnum to an integer
    \warn FIXME: portability to systems that don't preserve sign bit
    on right shifts
 */
#define FIX2INT(x) ((long)(x) >> 1)
/*! \def FIXNUMP(f)
    \brief Fixnum predicate
    True if f is a fixnum
 */
#define FIXNUMP(f) (((long)(f))&FIXNUM_FLAG)

/* Definitions and prototypes for conses */

/*! \struct cons
    \brief A cons cell
 */
typedef struct {
  value car;			/*!< car value for the cons cell */
  value cdr;			/*!< cdr value for the cons cell  */
} cons;

/*! \var __arc_cons_t
    \brief Type definition structure for conses
 */
extern arctype __arc_cons_t;

#define car(v) (((cons *)(v))->car)
#define cdr(v) (((cons *)(v))->cdr)

/*! \fn value arc_cons(arc *c, var car, var cdr)
    \brief Cons two values together
 */
static inline value arc_cons(arc *c, value car, value cdr)
{
  value conscell = arc_new(c, &__arc_cons_t, sizeof(cons));
  cons *cc = (cons *)conscell;
  cc->car = car;
  cc->cdr = cdr;
  return(conscell);
}

static inline void scar(arc *c, value v, value ncar)
{
  cons *cc = (cons *)v;
  /* use the write barrier before overwriting the pointer */
  arc_wb(c, cc->car, v);
  cc->car = v;
}

static inline void scdr(arc *c, value v, value ncdr)
{
  cons *cc = (cons *)v;
  /* use the write barrier before overwriting the pointer */
  arc_wb(c, cc->cdr, v);
  cc->cdr = v;
}

extern void __arc_fatal(const char *errmsg, int errnum);

/*! \fn value arc_type(value val)
    \brief Get the type of an Arcueid value
    \param val Value
 */
extern arctype *arc_type(value val);

/*! \fn value __arc_milliseconds(void)
    \brief The epoch time in milliseconds
 */
extern unsigned long long __arc_milliseconds(void);

#endif
