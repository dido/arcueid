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

extern void __arc_fatal(const char *errmsg, int errnum);

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
