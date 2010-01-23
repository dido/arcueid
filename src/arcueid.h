/* 
  Copyright (C) 2010 Rafael R. Sevilla

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
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA
  02110-1301 USA.
*/

#ifndef _ARCUEID_H_

#define _ARCUEID_H_

#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <gmp.h>

typedef unsigned long value;
typedef mpz_t bignum_t;
typedef mpq_t rational_t;

/* Characters are internally stored as UCS-4 */
typedef uint32_t Rune;

typedef struct arc {
  /* High-level allocation functions (allocate a cell, allocate a block) */
  value (*get_cell)(struct arc *);
  void *(*get_block)(struct arc *, size_t);
  void (*free_block)(struct arc *, void *); /* should be used only by gc */

  /* Low-level allocation functions (bypass memory management--use only
     from within an allocator or garbage collector) */
  void *(*mem_alloc)(size_t, int, void **);
  void (*mem_free)(void *, size_t);

  int over_percent;    /* additional free space over heap expansion */
  size_t minexp;       /* minimum expansion amount */

  /* Garbage collector */
  void (*rungc)(struct arc *);

  void (*signal_error)(struct arc *, const char *, ...);

  /* Reader */
  Rune (*readchar)(struct arc *, value, int *);
  void (*unreadchar)(struct arc *, value, Rune, int *);

  value vmthreads;		/* virtual machine thread objects */
  value vmqueue;		/* virtual machine run queue */

  unsigned long lastsym;	/* last symbol value */
  value symtable;		/* symbol table */
  value rsymtable;		/* reverse symbol table */
  value genv;			/* global environment */

  /* Tables used by the reader and the compiler */
  value builtin;		/* symbols for builtin functions */
  value charesctbl;		/* mzscheme-like character escapes */

  value splforms;		/* special forms */
  value inlfuncs;		/* inlinable functions */
} arc;


/* Boxed type flags */
enum arc_types {
  T_NIL=0x00,
  T_TRUE=0x01,
  T_FIXNUM=0x02,
  T_BIGNUM=0x03,
  T_FLONUM=0x04,
  T_RATIONAL=0x05,
  T_COMPLEX=0x06,
  T_CHAR=0x07,
  T_STRING=0x08,
  T_SYMBOL=0x09,
  T_CONS=0x0a,
  T_TABLE=0x0b,
  T_TBUCKET=0x0c,
  T_TAGGED=0x0d,
  T_INPUT=0x0e,
  T_OUTPUT=0x0f,
  T_EXCEPTION=0x10,
  T_SOCKET=0x11,
  T_THREAD=0x12,
  T_VECTOR=0x13,

  T_CONT = 0x14,		/* continuation */
  T_CLOS = 0x15,		/* closure */
  T_CODE = 0x16,		/* actual compiled code */
  T_ENV = 0x17,			/* environment */
  T_VMCODE = 0x18,		/* a VM code block */
  T_CCODE = 0x19,		/* a C function */

  T_NONE=0xff
};

enum threadstate {
  Talt,				/* blocked in alt instruction */
  Tsend,			/* waiting to send */
  Trecv,			/* waiting to recv */
  Tdebug,			/* debugged */
  Tready,			/* ready to be scheduled */
  Trelease,			/* interpreter released */
  Texiting,			/* exit because of kill or error */
  Tbroken,			/* thread crashed */
};


/* default thread timeslice */
#define PQUANTA 2048

/* default thread stack size */
#define TSTKSIZE 512

struct vmthread {
  /* virtual machine registers */
  value funr;		/* current function pointer register */
  value envr;		/* current environment register */
  value valr;		/* value of most recent instruction */
  value conr;		/* continuation register */
  value *spr;		/* stack pointer register */
  value stack;		/* the actual stack itself (a vector) */
  value *stkbase;	/* base pointer of the stack */
  value *stktop;	/* top of value stack */
  value *ip;		/* instruction pointer */
  int argc;		/* argument count register */
  
  /* thread scheduling variables */
  enum threadstate state;	/* thread state */
  int tid;			/* unique thread id */
  int quanta;		/* time slice */
  unsigned long long ticks;	/* time used */
};

#define TFUNR(t) (REP(t)._thread.funr)
#define TENVR(t) (REP(t)._thread.envr)
#define TVALR(t) (REP(t)._thread.valr)
#define TCONR(t) (REP(t)._thread.conr)
#define TSP(t) (REP(t)._thread.spr)
#define TSTACK(t) (REP(t)._thread.stack)
#define TSBASE(t) (REP(t)._thread.stkbase)
#define TSTOP(t) (REP(t)._thread.stktop)
#define TIP(t) (REP(t)._thread.ip)
#define TARGC(t) (REP(t)._thread.argc)
#define TSTATE(t) (REP(t)._thread.state)

/* Cells for boxed items */
struct cell {
  unsigned int _type;
  unsigned int _flags;
  union {
    struct {
      Rune *str;
      int length;
      int maxlength;
    } _str;
    Rune _char;
    bignum_t _bignum;
    rational_t _rational;
    double _flonum;
    struct {
      double re;
      double im;
    } _complex;
    struct {
      value car;
      value cdr;
    } _cons;
    struct {
      value *table;
      int hashbits;
      int nentries;
      int loadlimit;
    } _hash;
    struct {
      value key;
      value val;
      value hash;
      int index;
    } _hashbucket;
    /* The vector struct is actually used for lots of other stuff,
       such as compiled code, closures (which are three-vectors),
       and continuations (which are four-vectors). */
    struct {
      int length;
      value data[1];
    } _vector;
    struct vmthread _thread;
    /* C functions are defined here giving the actual function pointer,
       followed by the arguments/calling sequence.  If argc is
       non-negative, the C function will be called with the arc struct
       as its first argument, and that number of additional arguments.
       If argc is -1, the function will be called as:

       value fnptr(arc *c, int argc, value *argv)

       If argc is -2, the function will be alled as:

       value fnptr(arc *c, value argv)

       where argv is a Arcueid vector of the arguments. */
    struct {
      value (*fnptr)();
      int argc;
    } _cfunc;
  } obj;
};

#define VALUE_SIZE 8

/* Immediate values */
#define IMMEDIATE_MASK 0x03
#define IMMEDIATE_P(x) (((value)(x) & IMMEDIATE_MASK) || (value)(x) == CNIL || (value)(x) == CTRUE || (value)(x) == CUNDEF || (value)(x) == CUNBOUND)

#define SYMBOL_FLAG 0x0e
#define SYMBOL_P(x) (((value)(x)&0xff)==SYMBOL_FLAG)

/* Special constants -- non-zero and non-fixnum constants */
#define CNIL ((value)0)
#define CTRUE ((value)2)
#define CUNDEF ((value)4)	/* "tombstone" value */
#define CUNBOUND ((value)6)	/* returned when a hash has no binding value */

#define BTYPE(v) (((struct cell *)(v))->_type)
#define REP(v) (((struct cell *)(v))->obj)

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
#define car(x) (REP(x)._cons.car)
#define cdr(x) (REP(x)._cons.cdr)
#define cadr(x) (car(cdr(x)))
#define cddr(x) (cdr(cdr(x)))
#define caddr(x) (car(cddr(x)))

/* Definitions for vectors */
#define VECLEN(x) (REP(x)._vector.length)
#define VINDEX(x,i) (REP(x)._vector.data[(i)])

/* Definitions for hashing */
typedef struct {
  unsigned long s[3];
  int state;
} arc_hs;			/* hash state */

static inline value cons(struct arc *c, value x, value y)
{
  value cc = c->get_cell(c);

  BTYPE(cc) = T_CONS;
  car(cc) = x;
  cdr(cc) = y;
  return(cc);
}

static inline int TYPE(value v)
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

#define CONS_P(x) ((TYPE(x)) == (T_CONS))

extern value arc_genv;		/* global environment */

extern value arc_mkflonum(arc *c, double val);
extern value arc_mkcomplex(arc *c, double re, double im);
extern value arc_mkbignuml(arc *c, long val);
extern value arc_mkrationall(arc *c, long num, long den);

extern double arc_coerce_flonum(arc *c, value v);
extern void arc_coerce_complex(arc *c, value v, double *re, double *im);
extern void arc_coerce_bignum(arc *c, value v, void *bignum);
extern void arc_coerce_rational(arc *c, value v, void *rat);
extern value arc_coerce_fixnum(arc *c, value v);

extern value arc_arith_op(arc *c, int opval, value args);
extern value arc_string2num(arc *c, value v);
extern value arc_numcmp(arc *c, value v1, value v2);

extern value arc_is(arc *c, value v1, value v2);
extern value arc_iso(arc *c, value v1, value v2);
extern value arc_cmp(arc *c, value v1, value v2);

extern value scar(value x, value y);
extern value scdr(value x, value y);
extern value arc_list_append(value list, value val);
extern value arc_list_assoc(arc *c, value key, value a_list);
extern value arc_list_length(arc *c, value list);

extern value arc_mkstring(arc *c, const Rune *data, int length);
extern value arc_mkstringc(arc *c, const char *s);
extern value arc_mkchar(arc *c, Rune r);
extern int arc_strlen(arc *c, value v);
extern Rune arc_strindex(arc *c, value v, int index);
extern value arc_strcat(arc *c, value v1, value v2);
extern Rune arc_strgetc(arc *c, value src, int *index);
extern void arc_strungetc(arc *c, int *index);
extern value arc_strchr(arc *c, value str, Rune ch);
extern value arc_strcmp(arc *c, value vs1, value vs2);

extern void arc_hash_init(arc_hs *s, unsigned long level);
extern void arc_hash_update(arc_hs *s, unsigned long val);
extern unsigned long arc_hash_final(arc_hs *s, unsigned long len);
extern unsigned long arc_hash(arc *c, value v);
extern unsigned long arc_hash_cstr(arc *c, const char *str);
extern value arc_mkhash(arc *c, int hashbits);
extern value arc_hash_insert(arc *c, value hash, value key, value val);
extern value arc_hash_lookup(arc *c, value hash, value key);
extern value arc_hash_lookup2(arc *c, value hash, value key);
extern value arc_hash_lookup_cstr(arc *c, value hash, const char *key);
extern value arc_hash_delete(arc *c, value hash, value key);
extern value arc_hash_iter(arc *c, value hash, void **context);
extern value arc_intern(arc *c, value name);
extern value arc_intern_cstr(arc *c, const char *name);
extern value arc_sym2name(arc *c, value sym);

extern value arc_mkvector(arc *c, int length);

extern value arc_read(arc *c, value src, int *index, value *pval);
extern value arc_ssexpand(arc *c, value sym);

extern void arc_vmengine(arc *c, value thr, int quanta);
extern value arc_mkthread(arc *c, value funptr, int stksize, int ip);

extern value arc_mkcont(arc *c, value offset, value thr);
extern value arc_mkenv(arc *c, value parent, int size);
extern void arc_apply(arc *c, value thr, value fun);
extern void arc_return(arc *c, value thr);
extern value arc_mkvmcode(arc *c, int length);
extern value arc_mkcode(arc *c, value vmccode, value fname, value args,
			 int nlits);
extern value arc_mkclosure(arc *c, value code, value env);
extern value arc_mkccode(arc *c, int argc, value (*_cfunc)());
extern value arc_compile(arc *c, value expr, value env, value fname);

#define CODE_CODE(c) (VINDEX((c), 0))
#define CODE_NAME(c) (VINDEX((c), 1))
#define CODE_ARGS(c) (VINDEX((c), 2))
#define CODE_LITERAL(c, idx) (VINDEX((c), 3+(idx)))

#define ENV_NAMES(e) (VINDEX((e), 0))
#define ENV_VALUE(e, idx) (VINDEX((e), (idx)+1))
#define ENV_P(e) (TYPE(e) == T_ENV)

/* Initialization functions */
extern void arc_set_memmgr(arc *c);
extern void arc_init_reader(arc *c);
extern void arc_init_compiler(arc *c);

extern value arc_prettyprint(arc *c, value v);
extern void arc_print_string(arc *c, value str);

#endif
