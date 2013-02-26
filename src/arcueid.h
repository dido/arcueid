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

/* UCS-4 runes */
typedef int32_t Rune;

/* Definitions for hashing */
typedef struct {
  unsigned long s[3];
  int state;
} arc_hs;			/* hash state */

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
  T_CCODE = 24,			/* a C function */
  T_CUSTOM = 25,		/* custom type */
  T_CHAN = 26,			/* channel */
  T_TYPEDESC = 27,		/* type descriptor */
  T_WTABLE = 28,		/* weak table */
  T_FNAPP = 29,			/* function application */
  T_MAX = 30,

  T_NONE=64
};

struct arc;

/* Type functions */
struct typefn_t {
  /* Marker */
  void (*marker)(struct arc *c, value, int, void (*)(struct arc *, value, int));
  /* Sweeper */
  void (*sweeper)(struct arc *c, value);
  /* Pretty printer */
  value (*pprint)(struct arc *c, value, value *, value);
  /* Hasher */
  unsigned long (*hash)(struct arc *c, value, arc_hs *, value);
  /* shallow compare */
  value (*iscmp)(struct arc *c, value, value);
  /* deep compare (isomorphism) */
  value (*isocmp)(struct arc *c, value, value, value, value);
#if 0
  value (*marshal)(struct arc *c, value, value);
  value (*unmarshal)(struct arc *c, value);
#endif
};

typedef struct typefn_t typefn_t;

struct arc {
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

  void *alloc_ctx;		/* allocation/gc context */

  typefn_t *typefns[T_MAX+1];	/* type functions */
  value typedesc;		/* type descriptor hash */

  value symtable;		/* global symbol table */
  value rsymtable;		/* reverse global symbol table */
  int lastsym;			/* last symbol index created */

  int tid_nonce;		/* nonce for thread IDs */
  int stksize;			/* stack size for threads */
};

typedef struct arc arc;

extern const char *__arc_typenames[];

struct cell {
  /* the top two bits of the _type are used for garbage collection purposes. */
  unsigned char _type;
  value _obj[1];
};

extern void __arc_null_marker(arc *c, value v, int depth,
			      void (*markfn)(arc *, value, int));
extern void __arc_null_sweeper(arc *c, value v);

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
#define NIL_P(v) ((v) == CNIL)

#define BTYPE(v) (((struct cell *)(v))->_type & 0x3f)
#define STYPE(v, t) (((struct cell *)(v))->_type = (t))
#define REP(v) (((struct cell *)(v))->_obj)

/* Definitions for Fixnums */
#define FIXNUM_MAX (LONG_MAX >> 1)
#define FIXNUM_MIN (-FIXNUM_MAX - 1)
#define FIXNUM_FLAG 0x01
#define INT2FIX(i) ((value)(((long)(i))<< 1 | FIXNUM_FLAG))
/* FIXME: portability to systems that don't preserve sign bit on
   right shifts. */
#define FIX2INT(x) ((long)(x) >> 1)
#define FIXNUM_P(f) (((long)(f))&FIXNUM_FLAG)

static inline enum arc_types TYPE(value v)
{
  if (FIXNUM_P(v))
    return(T_FIXNUM);
  if (SYMBOL_P(v))
    return(T_SYMBOL);
  if (v == CNIL)
    return(T_NIL);
  if (v == CTRUE)
    return(T_TRUE);
  if (v == CUNDEF || v == CUNBOUND)
    return(T_NONE);
  if (!IMMEDIATE_P(v))
    return(BTYPE(v));

  /* Add more type values here */
  return(T_NONE);		/* unrecognized immediate type */
}

#define TYPENAME(tnum) (((tnum) >= 0 && (tnum) <= T_MAX) ? (__arc_typenames[tnum]) : "unknown")

/* Definitions for conses */
#define car(x) (REP(x)[0])
#define cdr(x) (REP(x)[1])
#define cadr(x) (car(cdr(x)))
#define cddr(x) (cdr(cdr(x)))
#define caddr(x) (car(cddr(x)))

extern value cons(arc *c, value x, value y);

/* Definitions for strings and characters */
extern value arc_mkstringlen(arc *c, int length);
extern value arc_mkstring(arc *c, const Rune *data, int length);
extern value arc_mkstringc(arc *c, const char *s);
extern value arc_mkchar(arc *c, Rune r);
extern int arc_strlen(arc *c, value v);
extern Rune arc_strindex(arc *c, value v, int index);
extern Rune arc_strsetindex(arc *c, value v, int index, Rune ch);
extern value arc_strcatc(arc *c, value v1, Rune ch);
extern value arc_substr(arc *c, value s, int sidx, int eidx);
extern value arc_strcat(arc *c, value v1, value v2);
extern int arc_strcmp(arc *c, value v1, value v2);
extern void arc_str2cstr(arc *c, value str, char *ptr);
extern value arc_strutflen(arc *c, value str);


/* Definitions for vectors */
#define VECLEN(x) (FIX2INT(REP(x)[0]))
#define VINDEX(x, i) (REP(x)[i+1])

extern value arc_mkvector(arc *c, int length);

extern void __arc_vector_marker(arc *c, value v, int depth,
				void (*markfn)(arc *, value, int));
extern void __arc_vector_sweeper(arc *c, value v);
extern value __arc_vector_hash(arc *c, value v, arc_hs *s, value visithash);
extern value __arc_vector_isocmp(arc *c, value v1, value v2, value vh1,
				 value vh2);

/* Definitions for hash tables */
/* Default initial number of bits for hashes */
#define ARC_HASHBITS 6
extern void arc_hash_init(arc_hs *s, unsigned long level);
extern void arc_hash_update(arc_hs *s, unsigned long val);
extern unsigned long arc_hash_final(arc_hs *s, unsigned long len);
extern unsigned long arc_hash_increment(arc *c, value v, arc_hs *s,
					value visithash);
extern unsigned long arc_hash(arc *c, value v, value visithash);
extern value arc_mkhash(arc *c, int hashbits);
extern value arc_mkwtable(arc *c, int hashbits);
extern value arc_hash_lookup(arc *c, value tbl, value key);
extern value arc_hash_lookup2(arc *c, value tbl, value key);
extern value arc_hash_insert(arc *c, value hash, value key, value val);
extern value arc_hash_delete(arc *c, value hash, value key);

/* Type handling functions */
typefn_t *__arc_typefn(arc *c, value v);
void __arc_register_typefn(arc *c, enum arc_types type, typefn_t *tfn);

/* Utility functions */
extern void __arc_append_buffer_close(arc *c, Rune *buf, int *idx,
				      value *str);
extern void __arc_append_buffer(arc *c, Rune *buf, int *idx, int bufmax,
				Rune ch, value *str);
extern void __arc_append_cstring(arc *c, char *buf, value *ppstr);

extern value arc_prettyprint(arc *c, value sexpr, value *ppstr,
			     value visithash);

extern value arc_mkobject(arc *c, size_t size, int type);
extern value arc_is(arc *c, value v1, value v2);
extern value arc_iso(arc *c, value v1, value v2, value vh1, value vh2);
extern value __arc_visit(arc *c, value v, value hash);
extern value __arc_visit2(arc *c, value v, value hash, value mykeyval);
extern value __arc_visitp(arc *c, value v, value hash);
extern void __arc_unvisit(arc *c, value v, value hash);

/* Thread definitions and functions */
enum threadstate {
  Talt,				/* blocked in alt instruction */
  Tsend,			/* waiting to send */
  Trecv,			/* waiting to recv */
  Tiowait,			/* I/O wait */
  Tioready,			/* I/O ready to resume */
  Tready,			/* ready to be scheduled */
  Tsleep,			/* thread sleeping */
  Tcritical,			/* critical section */
  Trelease,			/* interpreter released */
  Texiting,			/* exit because of kill or error */
  Tbroken,			/* thread crashed */
};

struct vmthread_t {
  value funr;			/* function pointer register */
  value envr;			/* environment register */
  value valr;			/* value of most recent instruction */
  value conr;			/* continuation register */
  value *spr;			/* stack pointer */
  value stack;			/* actual stack (a vector) */
  value *stkbase;		/* base pointer of the stack */
  value *stktop;		/* top of value stack */
  value *ip;			/* instruction point */
  int argc;			/* argument count register */

  enum threadstate state;	/* thread state */
  int tid;			/* thread ID */
  int quanta;			/* time slice */
  unsigned long long ticks;	/* time used */
  unsigned long long wakeuptime; /* wakeup time */
};

#define TFUNR(t) (((struct vmthread_t *)REP(t))->funr)
#define TENVR(t) (((struct vmthread_t *)REP(t))->envr)
#define TVALR(t) (((struct vmthread_t *)REP(t))->valr)
#define TCONR(t) (((struct vmthread_t *)REP(t))->conr)
#define TSP(t) (((struct vmthread_t *)REP(t))->spr)
#define TSTACK(t) (((struct vmthread_t *)REP(t))->stack)
#define TSBASE(t) (((struct vmthread_t *)REP(t))->stkbase)
#define TSTOP(t) (((struct vmthread_t *)REP(t))->stktop)
#define TIP(t) (((struct vmthread_t *)REP(t))->ip)
#define TARGC(t) (((struct vmthread_t *)REP(t))->argc)

#define TSTATE(t) (((struct vmthread_t *)REP(t))->state)
#define TTID(t) (((struct vmthread_t *)REP(t))->tid)
#define TQUANTA(t) (((struct vmthread_t *)REP(t))->quanta)
#define TTICKS(t) (((struct vmthread_t *)REP(t))->ticks)
#define TWAKEUP(t) (((struct vmthread_t *)REP(t))->wakeuptime)

#define CPUSH(thr, val) (*(TSP(thr)--) = (val))
#define CPOP(thr) (*(++TSP(thr)))

extern value arc_mkthread(arc *c);

/* Initialization functions */
extern void arc_set_memmgr(arc *c);
extern void arc_init_datatypes(arc *c);
extern void arc_init_symtable(arc *c);

#if 0

/* C functions

   The most general C function interface is as follows:

   value func(arc *c, int argc, value thr, CCONT);

   The arguments to the function are passed in the thread stack as one
   might expect.

   The macros below are inspired by Simon Tatham's C coroutines, and
   they've been warped to provide continuations in C.  A function of
   this kind can only call other functions by means of the CCALL or
   CCALLV macros defined below (and they, perversely, *return* an
   object of T_FNAPP to make the caller perform a function call!)
*/

#define CCONT value __ccont__
#define CVBEGIN int __vidx__ = 0
#define CVDEF(x) int x = __vidx__++
#define CV(x) (VINDEX(CONT_CLOS(__ccont__), x))
#define CVEND(c)						\
  if (NIL_P(__ccont__)) {					\
    __ccont__ = arc_mkccont(c, __vidx__, (void *)__func__);	\
  }
#define CBEGIN					\
  if (!NIL_P(__ccont__)) {			\
  switch (FIX2INT(CONT_OFS(__ccont__))) {	\
 case 0:;
#define CEND } }

#define CCALL(c, thr, func, fargc, ...)					\
  do {									\
    CONT_OFS(__ccont__) = INT2FIX(__LINE__); return(arc_mkapply(c, thr, __cont__, func, fargc, __VA_ARGS__)); case __LINE__:; \
  } while (0)

#define CCALLV(c, thr, func, fargv)					\
  do {									\
    CONT_OFS(__ccont__) = INT2FIX(__LINE__); return(arc_mkapplyv(c, thr, __cont__, func, fargc, __VA_ARGS__)); case __LINE__:; \
  } while (0)

#define CYIELD(c, thr)							\
  do {									\
    CONT_OFS(__ccont__) = INT2FIX(__LINE__); return(arc_mkyield(c, thr, __cont__, CNIL)); case __LINE__:; \
  } while (0)

#define CYIELDFD(c, thr, fd)						\
  do {									\
    CONT_OFS(__ccont__) = INT2FIX(__LINE__); return(arc_mkyield(c, thr, __cont__, INT2FIX(fd))); case __LINE__:; \
  } while (0)

#endif

#endif
