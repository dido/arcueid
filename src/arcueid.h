/* 
  Copyright (C) 2013 Rafael R. Sevilla

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

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <time.h>
#include <setjmp.h>

typedef unsigned long value;

/* Definitions for hashing */
typedef struct {
  unsigned long s[3];
  int state;
} arc_hs;			/* hash state */

typedef struct arc {
  /* Low-level allocation functions (bypass memory management--use only
     from within an allocator or garbage collector).  The mem_alloc function
     should always return addresses aligned to at least 4 bits/16 bytes. */
  void *(*mem_alloc)(size_t);
  void (*mem_free)(void *);

  /* Higher-level allocation */
  void *(*alloc)(struct arc *, size_t);
  void (*free)(struct arc *, void *, void *); /* should be used only by gc */

  /* Garbage collector entry point */
  void (*gc)(struct arc *);
  void (*markroots)(struct arc *);

  int over_percent;    /* additional free space over heap expansion */
  size_t minexp;       /* minimum expansion amount */
} arc;

enum arc_types {
  T_NIL=0,
  T_TRUE=1,
  T_FIXNUM=2,
  T_BIGNUM=3,
  T_FLONUM=4,
  T_RATIONAL=5,
  T_COMPLEX=6,
  T_CHAR=7,
  T_STRING=8,
  T_SYMBOL=9,
  T_CONS=10,
  T_TABLE=11,
  T_TBUCKET=12,
  T_TAGGED=13,
  T_INPUT=14,
  T_OUTPUT=15,
  T_EXCEPTION=16,
  T_PORT=17,
  T_THREAD=18,
  T_VECTOR=19,

  T_CONT = 20,			/* continuation */
  T_CLOS = 21,			/* closure */
  T_CODE = 22,			/* actual compiled code */
  T_ENV = 23,			/* environment */
  T_VMCODE = 24,		/* a VM code block */
  T_CCODE = 25,			/* a C function */
  T_CUSTOM = 26,		/* custom type */
  T_XCONT = 27,			/* CC4 x-continuation */
  T_CHAN = 28,			/* channel */
  T_MAX = 28,

  T_NONE=64
};

struct cell {
  /* the top two bits of the _type are used for garbage collection purposes. */
  unsigned char _type;
  value (*pprint)(arc *, value);
  void (*marker)(arc *, value, int, void (*markfn)(arc *, value, int));
  void (*sweeper)(arc *, value);
  unsigned long (*hash)(arc *, arc_hs *, value);
  value _obj[1];
};

/* Immediate values */
/* Symbols */
#define SYMBOL_FLAG 0x0e
#define ID2SYM(x) ((value)(((long)(x))<<8|SYMBOL_FLAG))
#define SYM2ID(x) (((unsigned long)(x))>>8)
#define SYMBOL_P(x) (((value)(x)&0xff)==SYMBOL_FLAG)

/* Special constants -- non-zero and non-fixnum constants */
#define CNIL ((value)0)
#define CTRUE ((value)2)
#define CUNDEF ((value)4)	/* "tombstone" value */
#define CUNBOUND ((value)6)	/* returned when a hash has no binding value */

#define IMMEDIATE_MASK 0x07
#define IMMEDIATE_P(x) (((value)(x) & IMMEDIATE_MASK) || (value)(x) == CNIL || (value)(x) == CTRUE || (value)(x) == CUNDEF || (value)(x) == CUNBOUND)

#define BTYPE(v) (((struct cell *)(v))->_type & 0x3f)
#define STYPE(v, t) (((struct cell *)(v))->_type = (t))
#define REP(v) (((struct cell *)(v))->_obj)

#define NIL_P(v) ((v) == CNIL)

/* Definitions for Fixnums */
#define FIXNUM_MAX (LONG_MAX >> 1)
#define FIXNUM_MIN (-FIXNUM_MAX - 1)
#define FIXNUM_FLAG 0x01
#define INT2FIX(i) ((value)(((long)(i))<< 1 | FIXNUM_FLAG))
/* FIXME: portability to systems that don't preserve sign bit on
   right shifts. */
#define FIX2INT(x) ((long)(x) >> 1)
#define FIXNUM_P(f) (((long)(f))&FIXNUM_FLAG)

/* Definitions for conses */
#define car(x) (REP(x)[0])
#define cdr(x) (REP(x)[1])
#define cadr(x) (car(cdr(x)))
#define cddr(x) (cdr(cdr(x)))
#define caddr(x) (car(cddr(x)))

extern value cons(arc *c, value x, value y);

#endif
