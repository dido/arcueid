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
  T_EXCEPTION=14,
  T_INPORT=15,
  T_OUTPORT=16,
  T_THREAD=17,
  T_VECTOR=18,

  T_CONT = 19,			/* continuation */
  T_CLOS = 20,			/* closure */
  T_CODE = 21,			/* actual compiled code */
  T_ENV = 22,			/* stack-based environment */
  T_CCODE = 23,			/* a C function */
  T_CUSTOM = 24,		/* custom type */
  T_CHAN = 25,			/* channel */
  T_TYPEDESC = 26,		/* type descriptor */
  T_WTABLE = 27,		/* weak table */
  T_MAX = 28,

  T_NONE=64
};

struct arc;

/* Trampoline states */
enum tr_states_t {
  TR_RESUME=1,
  TR_SUSPEND=3,
  TR_FNAPP=5,
  TR_RC
};

/* Type functions */
struct typefn_t {
  /* Marker */
  void (*marker)(struct arc *c, value, int, void (*)(struct arc *, value, int));
  /* Sweeper */
  void (*sweeper)(struct arc *c, value);
  /* Pretty printer.  This is an AFF. */
  int (*pprint)(struct arc *c, value);
  /* Hasher */
  unsigned long (*hash)(struct arc *c, value, arc_hs *);
  /* Shallow compare.  This is a normal function. */
  value (*iscmp)(struct arc *c, value, value);
  /* deep compare (isomorphism). This is an AFF. */
  int (*isocmp)(struct arc *c, value);
  /* applicator */
  int (*apply)(struct arc *c, value, value);
  /* Coerce.  This can in general only be used to convert something
     to a numeric type. */
  value (*coerce)(struct arc *c, value, enum arc_types);
#if 0
  /* Recursive hasher.  This is used for computing possibly recursive
     hashes and is an AFF. */
  int (*rhash)(struct arc *c, value);
  /* Marshaller and unmarshaller. Also AFFs. */
  int (*marshal)(struct arc *c, value);
  int (*unmarshal)(struct arc *c, value);
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

  /* Type functions and type descriptors */
  typefn_t *typefns[T_MAX+1];	/* type functions */
  value typedesc;		/* type descriptor hash */

  /* Symbol table and global environment */
  value symtable;		/* global symbol table */
  value rsymtable;		/* reverse global symbol table */
  int lastsym;			/* last symbol index created */
  value genv;			/* global environment */
  value builtins;		/* built-in data */

  /* Threading and scheduler */
  value vmthreads;		/* virtual machine thread objects (head) */
  value vmthrtail;		/* virtual machine thread objects (tail) */
  value vmqueue;		/* virtual machine run queue */
  value curthread;		/* current thread */
  int tid_nonce;		/* nonce for thread IDs */
  int stksize;			/* default stack size for threads */

  /* declarations */
  int atstrings;		/* allow atstrings or not */
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

/* Stack-based environments */
#define ENV_FLAG 0x0c
#define ENV_P(x) (((value)(x)&0xf)==ENV_FLAG)

/* Special constants -- non-zero and non-fixnum constants */
#define CNIL ((value)0)
#define CTRUE ((value)2)
#define CUNDEF ((value)4)	/* "tombstone" value */
#define CUNBOUND ((value)6)	/* returned when a hash has no binding value */
#define CLASTARG ((value)8)	/* last argument */

#define IMMEDIATE_MASK 0x0f
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
  if (ENV_P(v))
    return(T_ENV);
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

#define scar(x, y) ((car(x)) = (y))
#define scdr(x, y) ((cdr(x)) = (y))

#define CONS_P(x) (TYPE(x) == T_CONS)

extern value cons(arc *c, value x, value y);
extern value arc_list_append(value list1, value val);
extern value arc_list_length(arc *c, value list);

/* Definitions for strings and characters */
extern value arc_mkstringlen(arc *c, int length);
extern value arc_mkstring(arc *c, const Rune *data, int length);
extern value arc_mkstringc(arc *c, const char *s);
extern value arc_mkchar(arc *c, Rune r);
extern Rune arc_char2rune(arc *c, value ch);
extern int arc_strlen(arc *c, value v);
extern Rune arc_strindex(arc *c, value v, int index);
extern Rune arc_strsetindex(arc *c, value v, int index, Rune ch);
extern value arc_strcatc(arc *c, value v1, Rune ch);
extern value arc_substr(arc *c, value s, int sidx, int eidx);
extern value arc_strcat(arc *c, value v1, value v2);
extern int arc_strcmp(arc *c, value v1, value v2);
extern void arc_str2cstr(arc *c, value str, char *ptr);
extern value arc_strutflen(arc *c, value str);
extern value arc_strchr(arc *c, value str, Rune ch);

/* Definitions for vectors */
#define VECLEN(x) (FIX2INT(REP(x)[0]))
#define VINDEX(x, i) (REP(x)[i+1])

extern value arc_mkvector(arc *c, int length);

extern void __arc_vector_marker(arc *c, value v, int depth,
				void (*markfn)(arc *, value, int));
extern void __arc_vector_sweeper(arc *c, value v);
extern int __arc_vector_isocmp(arc *c, value thr);

/* Definitions for hash tables */
/* Default initial number of bits for hashes */
#define ARC_HASHBITS 6
extern void arc_hash_init(arc_hs *s, unsigned long level);
extern void arc_hash_update(arc_hs *s, unsigned long val);
extern unsigned long arc_hash_final(arc_hs *s, unsigned long len);
extern unsigned long arc_hash_increment(arc *c, value v, arc_hs *s);
extern unsigned long arc_hash(arc *c, value v);
extern value arc_mkhash(arc *c, int hashbits);
extern value arc_mkwtable(arc *c, int hashbits);
extern value arc_hash_lookup(arc *c, value tbl, value key);
extern value arc_hash_lookup2(arc *c, value tbl, value key);
extern value arc_hash_insert(arc *c, value hash, value key, value val);
extern value arc_hash_delete(arc *c, value hash, value key);

/* Type handling functions */
extern typefn_t *__arc_typefn(arc *c, value v);
extern void __arc_register_typefn(arc *c, enum arc_types type, typefn_t *tfn);
extern value arc_type(arc *c, value obj);
extern value arc_type_compat(arc *c, value obj);
extern value arc_rep(arc *c, value obj);
extern value arc_annotate(arc *c, value typesym, value obj);

/* Thread definitions and functions */
extern value arc_mkthread(arc *c);
extern void arc_thr_push(arc *c, value thr, value v);
extern value arc_thr_pop(arc *c, value thr);
extern value arc_thr_valr(arc *c, value thr);
extern value arc_thr_set_valr(arc *c, value thr, value v);
extern int arc_thr_argc(arc *c, value thr);
extern value arc_thr_envr(arc *c, value thr);
extern int arc_cmark(arc *c, value thr);
extern int arc_scmark(arc *c, value thr);
extern int arc_ccmark(arc *c, value thr);

/* Foreign function API */

extern value arc_mkccode(arc *c, int argc, value (*cfunc)(arc *, ...),
			 value name);
extern value arc_mkaff(arc *c, int (*aff)(arc *, value), value name);
extern int __arc_affapply(arc *c, value thr, value ccont, value func, ...);
extern int __arc_affapply2(arc *c, value thr, value ccont, value func,
			   value args);
extern int __arc_affyield(arc *c, value thr, int line);
extern int __arc_affiowait(arc *c, value thr, int line, int fd);
extern void __arc_affenv(arc *c, value thr, int prevsize, int extrasize);
extern int __arc_affip(arc *c, value thr);

/* Closures */
extern value arc_mkclos(arc *c, value code, value env);

/* String Port I/O */
extern value arc_instring(arc *c, value str, value name);
extern value arc_outstring(arc *c, value name);
extern value arc_inside(arc *c, value sio);

/* File I/O */
extern value arc_infile(arc *c, value thr);
extern value arc_outfile(arc *c, value thr);

/* stdin/stdout/stderr */
extern int arc_stdin(arc *c, value thr);
extern int arc_stdout(arc *c, value thr);
extern int arc_stderr(arc *c, value thr);

/* General I/O functions */
extern int arc_readb(arc *c, value thr);
extern int arc_readc(arc *c, value thr);
extern int arc_writeb(arc *c, value thr);
extern int arc_writec(arc *c, value thr);
extern int arc_close(arc *c, value thr);
extern int arc_peekc(arc *c, value thr);
extern int arc_ungetc(arc *c, value thr);
extern int arc_seek(arc *c, value thr);
extern int arc_tell(arc *c, value thr);
extern Rune arc_ungetc_rune(arc *c, Rune r, value fd);

/* Continuations */
extern value __arc_mkcont(arc *c, value thr, int offset);
extern int arc_callcc(arc *c, value thr);

/* The reader */
extern int arc_sread(arc *c, value thr);

/* Special syntax handling */
extern value arc_ssyntax(arc *c, value x);
extern int arc_ssexpand(arc *c, value thr);

/* The compiler */
extern int arc_compile(arc *c, value thr);

/* Utility functions */
extern void __arc_append_buffer_close(arc *c, Rune *buf, int *idx,
				      value *str);
extern void __arc_append_buffer(arc *c, Rune *buf, int *idx, int bufmax,
				Rune ch, value *str);
extern void __arc_append_cstring(arc *c, char *buf, value *ppstr);
extern Rune __arc_strgetc(arc *c, value str, int *index);
extern void __arc_strungetc(arc *c, int *index);


/* Output */
extern int arc_write(arc *c, value thr);
extern int arc_disp(arc *c, value thr);

extern value arc_mkobject(arc *c, size_t size, int type);
extern value arc_is2(arc *c, value a, value b);
extern int arc_is(arc *c, value thr);
extern int arc_iso(arc *c, value thr);
extern int arc_iso2(arc *c, value thr);
extern value __arc_visit(arc *c, value v, value hash);
extern value __arc_visit2(arc *c, value v, value hash, value mykeyval);
extern value __arc_visitp(arc *c, value v, value hash);
extern void __arc_unvisit(arc *c, value v, value hash);

/* Symbols */
extern value arc_intern(arc *c, value name);
extern value arc_intern_cstr(arc *c, const char *name);
extern value arc_sym2name(arc *c, value sym);
extern value arc_unintern(arc *c, value sym);

/* Environments */
extern void __arc_mkenv(arc *c, value thr, int prevsize, int extrasize);
extern value *__arc_getenv(arc *c, value thr, int depth, int index);

/* Numbers and arithmetic */
extern value arc_string2num(arc *c, value str, int index, int rational);

/* Initialization functions */
extern void arc_init_memmgr(arc *c);
extern void arc_init_datatypes(arc *c);
extern void arc_init_symtable(arc *c);
extern void arc_init_threads(arc *c);
extern void arc_init(arc *c);

/* Error handling */
extern void arc_err_cstrfmt(arc *c, const char *fmt, ...);

/* Arcueid Foreign Functions.  This is possibly the most insane abuse
   of the C preprocessor I have ever done.  The AFFDEF macro will permit
   at least 1 and at most 8 arguments to an AFF.  For zero arguments, or
   if you want to define a foreign function that uses more than 8, use
   AFFDEF0 and pull the parameters from the stack manually using arc_thr_argc
   and arc_thr_pop.  The technique used for defining parameters and variables
   using variadic macros used here is inspired by this:

   http://stackoverflow.com/questions/1872220/is-it-possible-to-iterate-over-arguments-in-variadic-macros

   The actual mechanism that provides our continuations is based on
   Simon Tatham's Coroutines in C:

   http://www.chiark.greenend.org.uk/~sgtatham/coroutines.html

*/

#define CONCATENATE(arg1, arg2)   CONCATENATE1(arg1, arg2)
#define CONCATENATE1(arg1, arg2)  CONCATENATE2(arg1, arg2)
#define CONCATENATE2(arg1, arg2)  arg1##arg2

#define FOR_EACH_1(what, x) what(x)
#define FOR_EACH_2(what, x, ...) what(x); FOR_EACH_1(what, __VA_ARGS__)
#define FOR_EACH_3(what, x, ...) what(x); FOR_EACH_2(what, __VA_ARGS__)
#define FOR_EACH_4(what, x, ...) what(x); FOR_EACH_3(what, __VA_ARGS__)
#define FOR_EACH_5(what, x, ...) what(x); FOR_EACH_4(what, __VA_ARGS__)
#define FOR_EACH_6(what, x, ...) what(x); FOR_EACH_5(what, __VA_ARGS__)
#define FOR_EACH_7(what, x, ...) what(x); FOR_EACH_6(what, __VA_ARGS__)
#define FOR_EACH_8(what, x, ...) what(x); FOR_EACH_7(what, __VA_ARGS__)

#define NARGS(...) NARGS_(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define NARGS_(_1, _2, _3, _4, _5, _6, _7, _8, _, ...) _

#define FOR_EACH_(N, what, ...) CONCATENATE(FOR_EACH_, N)(what, __VA_ARGS__)
#define FOR_EACH(what, ...) FOR_EACH_(NARGS(__VA_ARGS__), what, __VA_ARGS__)

#define AFF_PARAM(x) int x = __vidx__++; __nparams__++

#define AFFDEF(fname, ...)			\
  int fname(arc *c, value thr)			\
  {						\
    int __vidx__ = 0; int __nparams__ = 0;	\
    FOR_EACH(AFF_PARAM, __VA_ARGS__);		\
    do

#define AFFDEF0(fname) int fname(arc *c, value thr) { int __vidx__ = arc_thr_argc(c, thr); int __nparams__ = __vidx__; do

#define AFFEND while (0); return(TR_RC); }

#define ADEFVAR(x) int x = __vidx__++

#define AVAR(...) FOR_EACH(ADEFVAR, __VA_ARGS__)

#define AFBEGIN								\
  __arc_affenv(c, thr, __nparams__, __vidx__ - __nparams__);		\
  switch (__arc_affip(c, thr)) {					\
 case 0:;
#define AFEND }

#define AV(x) (*__arc_getenv(c, thr, 0, x))

#define AFCALL(func, ...)						\
  do {									\
    return(__arc_affapply(c, thr, __arc_mkcont(c, thr, __LINE__), func, __VA_ARGS__, CLASTARG)); case __LINE__:; \
  } while (0)

/* call giving args as a list */
#define AFCALL2(func, argv)						\
  do {									\
    return(__arc_affapply2(c, thr, __arc_mkcont(c, thr, __LINE__), func, argv)); case __LINE__:; \
  } while (0)

/* Tail call -- this will pass the return value of the function called
   back to the caller of the function which invoked it. */
#define AFTCALL(func, ...)					       \
  do {								       \
    return(__arc_affapply(c, thr, CNIL, func, __VA_ARGS__, CLASTARG)); \
  } while (0)

#define AFCRV (arc_thr_valr(c, thr))

#define AYIELD()							\
  do {									\
    return(__arc_affyield(c, thr, __LINE__)); case __LINE__:;		\
  } while (0)

#define AIOWAIT(fd)							\
  do {									\
    return(__arc_affiowait(c, thr, __LINE__, fd)); case __LINE__:;	\
  } while (0)

#define ARETURN(val)			\
  do {						\
    arc_thr_set_valr(c, thr, val);		\
    return(TR_RC);				\
  } while (0)

#endif
