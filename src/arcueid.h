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
  /*  T_TRUE=1, */
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
  T_TABLEVEC=12,
  T_TBUCKET=13,
  T_TAGGED=14,
  T_EXCEPTION=15,
  T_INPORT=16,
  T_OUTPORT=17,
  T_THREAD=18,
  T_VECTOR=19,

  T_CONT = 20,			/* continuation */
  T_CLOS = 21,			/* closure */
  T_CODE = 22,			/* actual compiled code */
  T_ENV = 23,			/* stack-based environment */
  T_CCODE = 24,			/* a C function */
  T_CUSTOM = 25,		/* custom type */
  T_CHAN = 26,			/* channel */
  T_TYPEDESC = 27,		/* type descriptor */
  T_WTABLE = 28,		/* weak table */
  T_NUM = 29,			/* number -- not a real type */
  T_INT = 30,			/* int -- not a real type */
  T_MAX = 30,

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
  /* Hasher.  Simple hasher. */
  unsigned long (*hash)(struct arc *c, value, arc_hs *);
  /* Shallow compare.  This is a normal function. */
  value (*iscmp)(struct arc *c, value, value);
  /* deep compare (isomorphism). This is an AFF. */
  int (*isocmp)(struct arc *c, value);
  /* applicator */
  int (*apply)(struct arc *c, value, value);
  /* Type coercion function.  An AFF that takes as argument an object
     of its type and a fixnum representation of the type to convert to
     and tries to convert it to that type. */
  int (*xcoerce)(struct arc *c, value);
  /* Recursive hasher.  This is used for computing possibly recursive
     hashes and is an AFF. */
  int (*xhash)(struct arc *c, value);
#if 0
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
  int (*gc)(struct arc *);
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
  value ctrue;			/* true */

  /* Threading and scheduler */
  value vmthreads;		/* virtual machine thread objects (head) */
  value vmthrtail;		/* virtual machine thread objects (tail) */
  value curthread;		/* current thread */
  int tid_nonce;		/* nonce for thread IDs */
  int stksize;			/* default stack size for threads */
  value tracethread;		/* tracing thread */
  unsigned long quantum;	/* default quantum */
  void (*errhandler)(struct arc *, value, value); /* catch-all error handler */

  /* declarations */
  value declarations;		/* declarations hash */
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
/* #define CTRUE ((value)2) */
#define CUNDEF ((value)4)	/* "tombstone" value */
#define CUNBOUND ((value)6)	/* returned when a hash has no binding value */
#define CLASTARG ((value)8)	/* last argument */

#define CTRUE (c->ctrue)

#define IMMEDIATE_MASK 0x0f
#define IMMEDIATE_P(x) (((value)(x) & IMMEDIATE_MASK) || (value)(x) == CNIL || (value)(x) == CUNDEF || (value)(x) == CUNBOUND)
#define NIL_P(v) ((v) == CNIL)
#define BOUND_P(v) ((v) != CUNBOUND)

#define BTYPE(v) (((struct cell *)(v))->_type & 0x3f)
#define STYPE(v, t) (((struct cell *)(v))->_type = (t))
#define REP(v) (((struct cell *)(v))->_obj)

/* Definitions for Fixnums */
#define FIXNUM_MAX (LONG_MAX >> 1)
#define FIXNUM_MIN (-FIXNUM_MAX)
#define FIXNUM_FLAG 0x01
#define INT2FIX(i) ((value)(((long)(i))<< 1 | FIXNUM_FLAG))
/* FIXME: portability to systems that don't preserve sign bit on
   right shifts. */
#define FIX2INT(x) ((long)(x) >> 1)
#define FIXNUM_P(f) (((long)(f))&FIXNUM_FLAG)

#define NUMERIC_P(x) ((TYPE(x) == (T_FIXNUM)) || (TYPE(x) == (T_BIGNUM)) \
		      || (TYPE(x) == (T_FLONUM)) || (TYPE(x) == (T_RATIONAL)) \
		      || (TYPE(x) == (T_COMPLEX)))

#define LITERAL_P(x) ((x) == ARC_BUILTIN(c, S_T)			\
		      || (x) == ARC_BUILTIN(c, S_NIL)			\
		      || NIL_P((x)) || (x) == CTRUE			\
		      || TYPE((x)) == T_CHAR || TYPE((x)) == T_STRING	\
		      || TYPE((x)) == T_FIXNUM || TYPE((x)) == T_BIGNUM	\
		      || TYPE((x)) == T_FLONUM || TYPE((x)) == T_RATIONAL \
		      || TYPE((x)) == T_RATIONAL || TYPE((x)) == T_COMPLEX)

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
  if (v == CUNDEF || v == CUNBOUND)
    return(T_NONE);
  if (!IMMEDIATE_P(v))
    return(BTYPE(v));

  /* Add more type values here */
  return(T_NONE);		/* unrecognized immediate type */
}

extern inline void __arc_wb(value x, value y);

#define TYPENAME(tnum) (((tnum) >= 0 && (tnum) <= T_MAX) ? (__arc_typenames[tnum]) : "unknown")

/* Definitions for conses */
#define car(x) (REP(x)[0])
#define cdr(x) (REP(x)[1])
#define cadr(x) (car(cdr(x)))
#define cddr(x) (cdr(cdr(x)))
#define caddr(x) (car(cddr(x)))

static inline value scar(value x, value y)
{
  __arc_wb(car(x), y);
  car(x) = y;
  return(y);
}

static inline value scdr(value x, value y)
{
  __arc_wb(cdr(x), y);
  cdr(x) = y;
  return(y);
}

#define CONS_P(x) (TYPE(x) == T_CONS)

extern value cons(arc *c, value x, value y);
extern int arc_list(arc *c, value thr);
extern int arc_dlist(arc *c, value thr);
extern int arc_append(arc *c, value thr);
extern int arc_reduce(arc *c, value thr);
extern int arc_rreduce(arc *c, value thr);
extern value arc_list_append(value list1, value val);
extern value arc_list_reverse(arc *c, value xs);
extern value arc_list_length(arc *c, value list);
extern value arc_car(arc *c, value x);
extern value arc_cdr(arc *c, value x);
extern value arc_scar(arc *c, value x, value y);
extern value arc_scdr(arc *c, value x, value y);
extern value arc_cadr(arc *c, value x);
extern value arc_cddr(arc *c, value x);

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
extern int arc_newstring(arc *c, value thr);

/* Definitions for vectors */
#define VECLEN(x) (FIX2INT(REP(x)[0]))
#define XVINDEX(x, i) (REP(x)[i+1])

static inline value VINDEX(value x, value i)
{
  return(REP(x)[i+1]);
}

static inline value SVINDEX(value x, value i, value v)
{
  __arc_wb(REP(x)[i+1], v);
  REP(x)[i+1] = v;
  return(v);
}

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
extern int arc_newtable(arc *c, value thr);
extern value arc_hash_lookup(arc *c, value tbl, value key);
extern value arc_hash_lookup2(arc *c, value tbl, value key);
extern value arc_hash_insert(arc *c, value hash, value key, value val);
extern value arc_hash_delete(arc *c, value hash, value key);
extern int arc_hash_length(arc *c, value hash);
extern int arc_xhash_lookup(arc *c, value thr);
extern int arc_xhash_lookup2(arc *c, value thr);
extern int arc_xhash_delete(arc *c, value thr);
extern int arc_xhash_insert(arc *c, value thr);
extern int arc_xhash_increment(arc *c, value thr);
extern int arc_xhash_iter(arc *c, value thr);
extern int arc_xhash_map(arc *c, value thr);

/* Type handling functions */
extern typefn_t *__arc_typefn(arc *c, value v);
extern void __arc_register_typefn(arc *c, enum arc_types type, typefn_t *tfn);
extern value arc_type(arc *c, value obj);
extern value arc_type_compat(arc *c, value obj);
extern value arc_rep(arc *c, value obj);
extern value arc_annotate(arc *c, value typesym, value obj);
extern int arc_coerce(arc *c, value thr);

#define TYPECHECK(arg, expected)					\
  if (TYPE(arg) != expected) {						\
    arc_err_cstrfmt(c, "%s: expected argument to be type %d given type %d", __func__, TYPE(arg), expected); \
    return(CNIL);							\
  }

/* Thread definitions and functions */
extern value arc_mkthread(arc *c);
extern void arc_thr_push(arc *c, value thr, value v);
extern value arc_thr_pop(arc *c, value thr);
extern value arc_thr_valr(arc *c, value thr);
extern value arc_thr_set_valr(arc *c, value thr, value v);
extern int arc_thr_argc(arc *c, value thr);
extern value arc_thr_envr(arc *c, value thr);
extern value arc_cmark(arc *c, value key);
extern value arc_scmark(arc *c, value key, value val);
extern value arc_ccmark(arc *c, value key);

extern int arc_apply(arc *c, value thr);

/* Foreign function API */

extern value arc_mkccode(arc *c, int argc, value (*cfunc)(),
			 value name);
extern value arc_mkaff(arc *c, int (*aff)(arc *, value), value name);
extern value arc_mkaff2(arc *c, int (*aff)(arc *, value), value name,
			value env);
extern int __arc_affapply(arc *c, value thr, value ccont, value func, ...);
extern int __arc_affapply2(arc *c, value thr, value ccont, value func,
			   value args);
extern int __arc_affyield(arc *c, value thr, int line);
extern int __arc_affiowait(arc *c, value thr, int line, int fd, int rw);
extern void __arc_affenv(arc *c, value thr, int nargs, int optargs,
			 int localvars, int rest);
extern int __arc_affip(arc *c, value thr);

/* Closures */
extern value arc_mkclos(arc *c, value code, value env);

/* String Port I/O */
extern value arc_instring(arc *c, value str, value name);
extern value arc_outstring(arc *c, value name);
extern int arc_instring2(arc *c, value thr);
extern int arc_outstring2(arc *c, value thr);
extern value arc_inside(arc *c, value sio);

/* File I/O */
extern int arc_infile(arc *c, value thr);
extern int arc_outfile(arc *c, value thr);
extern value arc_flushout(arc *c);

/* Network I/O */
extern int arc_open_socket(arc *c, value thr);
extern int arc_socket_accept(arc *c, value thr);
extern value arc_client_ip(arc *c, value sock);

/* stdin/stdout/stderr */
extern int arc_stdin(arc *c, value thr);
extern int arc_stdout(arc *c, value thr);
extern int arc_stderr(arc *c, value thr);

/* file system operations */
extern value arc_dir(arc *c, value dirname);
extern value arc_dir_exists(arc *c, value dirname);
extern value arc_file_exists(arc *c, value filename);
extern value arc_rmfile(arc *c, value filename);
extern value arc_mvfile(arc *c, value oldname, value newname);

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
extern int arc_write(arc *c, value thr);
extern int arc_disp(arc *c, value thr);
extern int __arc_disp_write(arc *c, value thr);
extern value arc_pipe_from(arc *c, value cmd);
extern value arc_portname(arc *c, value port);

/* Continuations */
extern value __arc_mkcont(arc *c, value thr, int offset);

/* The reader */
extern int arc_sread(arc *c, value thr);
extern value __arc_get_fileline(arc *c, value lndata, value obj);
extern value __arc_reset_lineno(arc *c, value lndata);

/* Special syntax handling */
extern value arc_ssyntax(arc *c, value x);
extern int arc_ssexpand(arc *c, value thr);

/* The compiler */
extern int arc_compile(arc *c, value thr);
extern int arc_eval(arc *c, value thr);
extern int arc_quasiquote(arc *c, value thr);

/* Macros */
extern int arc_macex(arc *c, value thr);
extern int arc_macex1(arc *c, value thr);
extern value arc_uniq(arc *c);

/* Utility functions */
extern void __arc_append_buffer_close(arc *c, Rune *buf, int *idx,
				      value *str);
extern void __arc_append_buffer(arc *c, Rune *buf, int *idx, int bufmax,
				Rune ch, value *str);
extern void __arc_append_cstring(arc *c, char *buf, value *ppstr);
extern Rune __arc_strgetc(arc *c, value str, int *index);
extern void __arc_strungetc(arc *c, int *index);
extern void __arc_enqueue(arc *c, value thr, value *head, value *tail);
extern value __arc_dequeue(arc *c, value *head, value *tail);
extern value __arc_ull2val(arc *c, unsigned long long ms);

/* Output */
extern int arc_write(arc *c, value thr);
extern int arc_disp(arc *c, value thr);

extern value arc_mkobject(arc *c, size_t size, int type);
extern value arc_is2(arc *c, value a, value b);
extern int arc_is(arc *c, value thr);
extern int arc_iso(arc *c, value thr);
extern int arc_iso2(arc *c, value thr);
extern value __arc_visitkey(value v);
extern value __arc_visit(arc *c, value v, value hash);
extern value __arc_visit2(arc *c, value v, value hash, value mykeyval);
extern value __arc_visitp(arc *c, value v, value hash);
extern void __arc_unvisit(arc *c, value v, value hash);
extern value arc_cmp(arc *c, value v1, value v2);


/* Symbols */
extern value arc_intern(arc *c, value name);
extern value arc_intern_cstr(arc *c, const char *name);
extern value arc_sym2name(arc *c, value sym);
extern value arc_unintern(arc *c, value sym);
extern value arc_bound(arc *c, value sym);
extern value arc_bindsym(arc *c, value sym, value binding);
extern value arc_bindcstr(arc *c, const char *csym, value binding);
extern value arc_gbind(arc *c, const char *csym);

/* Environments */
extern void __arc_mkenv(arc *c, value thr, int prevsize, int extrasize);
extern value __arc_getenv(arc *c, value thr, int depth, int index);
extern value __arc_putenv(arc *c, value thr, int depth, int index, value val);

/* Numbers and arithmetic */
extern value arc_string2num(arc *c, value str, int index, int rational);
extern value arc_expt(arc *c, value a, value b);

/* Threads and synchronisation */
extern void arc_thread_dispatch(arc *c);
extern value arc_spawn(arc *c, value thunk);
extern value arc_current_thread(arc *c);
extern value arc_break_thread(arc *c, value thr);
extern int arc_kill_thread(arc *c, value thr);
extern value arc_dead(arc *c, value thr);
extern int arc_sleep(arc *c, value thr);
extern int arc_join_thread(arc *c, value thr);
extern int arc_atomic_cell(arc *c, value thr);

extern int arc_join_thread(arc *c, value thr);
extern value arc_mkchan(arc *c);
extern int arc_recv_channel(arc *c, value thr);
extern int arc_send_channel(arc *c, value thr);

/* Initialization functions */
extern void arc_init_memmgr(arc *c);
extern void arc_init_datatypes(arc *c);
extern void arc_init_symtable(arc *c);
extern void arc_init_threads(arc *c);
extern void arc_init(arc *c);
extern void arc_deinit(arc *c);

/* Error handling */
extern void arc_err_cstrfmt(arc *c, const char *fmt, ...);
extern void arc_err_cstrfmt_line(arc *c, value fileline, const char *fmt, ...);
extern int arc_callcc(arc *c, value thr);
extern int arc_dynamic_wind(arc *c, value thr);
extern int arc_err(arc *c, value thr);
extern int arc_on_err(arc *c, value thr);
extern value arc_mkexception(arc *c, value str);
extern value arc_details(arc *c, value ex);

/* OS-dependent functions */
extern unsigned long long __arc_milliseconds(void);
extern value arc_seconds(arc *c);
extern value arc_msec(arc *c);
extern value arc_current_process_milliseconds(arc *c);
extern value arc_setuid(arc *c, value uid);
extern int arc_timedate(arc *c, value thr);
extern int arc_system(arc *c, value thr);
extern int arc_quit(arc *c, value thr);

/* Miscellaneous functions */
extern int arc_sref(arc *c, value thr);
extern value arc_len(arc *c, value obj);
extern value arc_bound(arc *c, value sym);
extern value arc_declare(arc *c, value decl, value val);
extern value arc_declared(arc *c, value decl);

/* Arcueid Foreign Functions.  This is possibly the most insane abuse
   of the C preprocessor I have ever done.  The technique used for defining
   parameters and variables using variadic macros used here is inspired by
   this:

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

#define AFFDEF(fname) int fname(arc *c, value thr) { char __nargs__ = 0; char  __optargs__ = 0; char __restarg__ = 0; char __localvars__ = 0; do

#define AFFEND while (0); return(TR_RC); }

#define ADEFARG(x) const char x = __nargs__++

#define AOPTARG(x) const char x = __nargs__ +  __optargs__++

#define ARARG(x) const char x = __nargs__ + __optargs__ + __localvars__; __restarg__ = 1

#define ADEFVAR(x) const char x = __nargs__ + __optargs__ + __localvars__++;

#define AARG(...) FOR_EACH(ADEFARG, __VA_ARGS__)

#define AOARG(...) FOR_EACH(AOPTARG, __VA_ARGS__)

#define AVAR(...) FOR_EACH(ADEFVAR, __VA_ARGS__)

#define AFBEGIN								\
  __arc_affenv(c, thr, __nargs__, __optargs__, __localvars__, __restarg__); \
  switch (__arc_affip(c, thr)) {					\
 case 0:;
#define AFEND }

#define AV(x) (__arc_getenv(c, thr, 0, x))
#define WV(x, y) (__arc_putenv(c, thr, 0, x, y))

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

/* tail call giving args as a list */
#define AFTCALL2(func, argv)						\
  do {									\
    return(__arc_affapply2(c, thr, CNIL, func, argv));			\
  } while (0)

#define AFCRV (arc_thr_valr(c, thr))

#define AYIELD()							\
  do {									\
    return(__arc_affyield(c, thr, __LINE__)); case __LINE__:;		\
  } while (0)

#define AIOWAITR(fd)							\
  do {									\
    return(__arc_affiowait(c, thr, __LINE__, fd, 0)); case __LINE__:;	\
  } while (0)

#define AIOWAITW(fd)							\
  do {									\
    return(__arc_affiowait(c, thr, __LINE__, fd, 1)); case __LINE__:;	\
  } while (0)

#define ARETURN(val)			\
  do {						\
    arc_thr_set_valr(c, thr, val);		\
    return(TR_RC);				\
  } while (0)

#endif
