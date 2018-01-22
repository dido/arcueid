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

/*! \file arcueid.h
    \brief Main include file for Arcueid
 */
#ifndef _ARCUEID_H_

#define _ARCUEID_H_

#include <stdlib.h>
#include <stdint.h>
#include <limits.h>

/*! \typedef value
    \brief The type definition for basic Arcueid values

    All Arcueid objects are encapsulated as values. These can be
    immediate values (where the last four bits are not all zero) or
    pointers (where the last four bits are all zero).
 */
typedef unsigned long value;


/*! \struct arc
    \brief Arcueid interpreter context
 */
typedef struct arc {
  void *mm_ctx;			/*!< memory manager context */
  void *gc_ctx;			/*!< garbage collector context */
  /*! Root marking function called at the beginning of a GC cycle to
      mark the initial root set */
  void (*markroots)(struct arc *, void (*)(struct arc *, value));

  int stksize;			/*!< default stack size for threads */

  /* Threads */
  value vmthreads;		/*!< list of scheduled threads */
  value curthread;		/*!< currently executing thread  */
  value vmthrtail;		/*!< tail of vmthread queue */
  long quantum;			/*!< thread quantum */

  /* Tables */
  value runetbl;		/*!< table of runes */
  value obtbl;			/*!< obtbl for symbols */
} arc;

/*! \struct arctype
    \brief type definition structure
    All Arcueid types are given a structure of this sort, which
    contains function definitions used for garbage collector
    processing and other type-specific stuff.
 */
typedef struct {
  /*! Called before an object of this type is freed. Typically one
      would put in here actions related to the destruction of the
      object, such as closing file descriptors, freeing any extra
      memory not allocated through the usual Arcueid memory
      allocation mechanism, etc. */
  void (*free)(arc *, value);
  /*! Called whenever the garbage collector marks an object of this
      type. The function is passed the object itself, a function which
      one passes all the pointers contained in the object (the marker
      function), and the depth, which should be passed unchanged to
      the marker function. */
  void (*mark)(arc *, value, void (*)(arc *, value, int), int);
  /*! Called whenever a hash of the value is required */
  uint64_t (*hash)(arc *, value, uint64_t);
  /*! Type-specific simple equivalence (Scheme eqv?) */
  int (*is)(arc *, value, value);
  /*! Type-specific structural equivalence (Scheme equal?) */
  int (*iso)(arc *, value, value);
  /*! Type-specific initialization */
  void (*init)(arc *);
} arctype;

/*! \enum arc_trstate
    \brief Arc trampoline states
 */
enum arc_trstate {
  /*! Resume execution */
  TR_RESUME=1,
  /*! Return to the thread dispatcher if the thread has been
      suspended for whatever reason */
  TR_SUSPEND=3,
  /*! Apply the function set up in the value register. */
  TR_FNAPP=5,
  /*! Restore the last continuation in the continuation register */
  TR_RC=7
};

/*! \fn void __arc_fatal(const char *errmsg, int errnum)
    \brief Fatal error function
 */
extern void __arc_fatal(const char *errmsg, int errnum);

/*! \fn value arc_type(value val)
    \brief Get the type of an Arcueid value
    \param val Value
 */
extern arctype *arc_type(value val);

/*! \fn unsigned long long __arc_milliseconds(void)
    \brief The epoch time in milliseconds
 */
extern unsigned long long __arc_milliseconds(void);

/*! \fn void __arc_sleep(unsigned long long st)
    \brief sleep for _st_ milliseconds
 */
extern void __arc_sleep(unsigned long long st);

/*! \fn int __arc_is(arc *c, value v1, value v2)
    \brief Simple equivalence (similar to Scheme's eqv?)
 */
extern int __arc_is(arc *c, value v1, value v2);

/*! \def IMMEDIATE_MASK
    \brief Mask for immediate values
 */
#define IMMEDIATE_MASK 0x0f

/*! \def IMMEDIATEP(x)
    \brief Predicate if _x_ is an immediate value
 */
#define IMMEDIATEP(x) (((value)(x) & IMMEDIATE_MASK) || (value)(x) == CNIL)

/*  =========== Definitions for nils */
/*! \def CNIL
    \brief The nil value
    This is one of a few SPECIAL CONSTANTS defined by the interpreter.
 */
#define CNIL ((value)0)

/*! \def NILP(v)
    \brief Nil predicate
    True if v is a nil
 */
#define NILP(v) ((v) == CNIL)

/*! \var __arc_nil_t
    \brief Type definition structure for nils
 */
extern arctype __arc_nil_t;

/* Initialization */
/*! \fn value arc_init(arc *c)
    \brief Initialize an Arc context
 */
extern void arc_init(arc *c);

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

/*  =========== Definitions for Fixnums */
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

/*! \var __arc_fixnum_t
    \brief Type definition structure for fixnums
 */
extern arctype __arc_fixnum_t;

/*  =========== Definitions and prototypes for flonums */
extern arctype __arc_flonum_t;

/*! \fn value arc_flonum_new(arc *c, double f)
    \brief flonum constructor function
 */
extern value arc_flonum_new(arc *c, double f);

/*! \fn value arc_flonum(value f)
    \brief flonum value to double
    \warn Make sure that the value passed actually is a flonum
 */
static inline double arc_flonum(value f)
{
  return(*((double *)f));
}

/*  =========== Definitions and prototypes for conses */

/*! \struct cons
    \brief A cons cell
 */
typedef struct {
  value car;			/*!< car value for the cons cell */
  value cdr;			/*!< cdr value for the cons cell  */
} cons_t;

/*! \var __arc_cons_t
    \brief Type definition structure for conses
 */
extern arctype __arc_cons_t;

#define car(v) (((cons_t *)(v))->car)
#define cdr(v) (((cons_t *)(v))->cdr)

/*! \fn value arc_cons(arc *c, var car, var cdr)
    \brief Cons two values together
 */
static inline value cons(arc *c, value car, value cdr)
{
  value conscell = arc_new(c, &__arc_cons_t, sizeof(cons_t));
  cons_t *cc = (cons_t *)conscell;
  cc->car = car;
  cc->cdr = cdr;
  return(conscell);
}

/*! \fn void scar(arc *c, value v, value ncar)
    \brief Set the car of a cons cell
 */

static inline void scar(arc *c, value v, value ncar)
{
  cons_t *cc = (cons_t *)v;
  /* use the write barrier before overwriting the pointer */
  arc_wb(c, cc->car, ncar);
  cc->car = ncar;
}

/*! \fn void scdr(arc *c, value v, value ncdr)
    \brief Set the cdr of a cons cell
 */
static inline void scdr(arc *c, value v, value ncdr)
{
  cons_t *cc = (cons_t *)v;
  /* use the write barrier before overwriting the pointer */
  arc_wb(c, cc->cdr, ncdr);
  cc->cdr = ncdr;
}

/* =========== Definitions and prototypes for vectors */

/*! \var __arc_vector_t
    \brief Type definition structure for vectors
 */
extern arctype __arc_vector_t;

/*! \fn value arc_vector_new(arc *c, int size)
    \brief vector constructor function
 */
extern value arc_vector_new(arc *c, int size);

static inline int VLEN(value v)
{
  value *vec = (value *)v;
  return(FIX2INT(vec[0]));
}

/*! \fn value VIDX(value v, int i)
    \brief Get the value of an object in a vector at index i
 */
static inline value VIDX(value v, int i)
{
  value *vec = (value *)v;
  return(vec[i+1]);
}


static inline value SVIDX(arc *c, value v, int i, value x)
{
  value *vec = (value *)v;
  arc_wb(c, vec[i+1], x);
  vec[i+1] = x;
  return(x);
}

/* =========== Definitions and prototypes for weak references */

/*! \var __arc_wref_t
    \brief Type definition structure for weak references.
 */
extern arctype __arc_wref_t;

/*! \fn value arc_wref_new(arc *c, value v)
    \brief Create a new weak reference.

    Create a new weak reference referring to _v_. This has the effect
    that unless v is reachable by the garbage collector from some
    other object besides the weak reference, it becomes a target for
    garbage collection.
 */
extern value arc_wref_new(arc *c, value v);

/*! \fn value arc_wrefv(value wr)
    \brief Dereference a weak reference.

    If the weak reference _wr_ is still valid, returns its
    value. Otherwise, returns _CUNDEF_.
 */
extern value arc_wrefv(value wr);

/* =========== Definitions and prototypes for hashes */

struct hash_ctx {
  uint64_t h1;
  uint64_t h2;
  size_t len;
};

/*! \def CUNBOUND
    \brief The unbound value
    This is one of a few SPECIAL CONSTANTS defined by the interpreter.
 */
#define CUNBOUND ((value)2)


/*! \def ARC_HASHBITS
    \brief Default initial number of bits for hashes
 */
#define ARC_HASHBITS 6

/*! \var __arc_tbl_t
    \brief Type definition structure for hash tables.
 */
extern arctype __arc_tbl_t;

/*! \fn void __arc_hash_init(struct hash_ctx *ctx)
    \brief Initialize a hash context
 */
extern void __arc_hash_init(struct hash_ctx *ctx, uint64_t seed);

/*! \fn void __arc_hash_update(struct hash_ctx *ctx, const uint64_t *data,
			      const size_t len)
    \brief Update the hash with new data
 */
extern void __arc_hash_update(struct hash_ctx *ctx, const uint64_t *data,
			      const int len);
 
/*! \fn void uint64_t __arc_hash_final(struct hash_ctx *ctx)
    \brief Get the final value of the hash
 */
extern uint64_t __arc_hash_final(struct hash_ctx *ctx);

/*! \fn uint64_t __arc_immediate_hash(arc *c, value val, uint64_t seed)
    \brief Hash function for hashing all immediate values
 */
extern uint64_t __arc_immediate_hash(arc *c, value val, uint64_t seed);

/*! \fn uint64_t __arc_hash(arc *c, value v, uint64_t seed)
    \brief Hash a value using the type-defined hash function
 */
extern uint64_t __arc_hash(arc *c, value v, uint64_t seed);

/*! \fn uint64_t __arc_utfstrhash(arc *c, const char *s, uint64_t seed)
    \brief Hash a UTF-8 string
    This should compute the same hash as hashing the string itself.
 */
extern uint64_t __arc_utfstrhash(arc *c, const char *s, uint64_t seed);

/*! \var __arc_tbl_t
    \brief Arc's hash table
 */
extern arctype __arc_tbl_t;

/*! \def HASH_WEAK_KEY
    \brief Use weak references for keys
 */
#define HASH_WEAK_KEY 0x01
/*! \def HASH_WEAK_VAL
    \brief Use weak references for values
 */
#define HASH_WEAK_VAL 0x02

/*! \fn value arc_tbl_new_flags(arc *c, int hashbits, unsigned int flags)
    \brief Create a new hash table, with flags
    Create a new hash table with 2^_hashbits_ entries with flags as above
 */
extern value arc_tbl_new_flags(arc *c, int hashbits, unsigned int flags);

/*! \fn value arc_tbl_new(arc *c, int nbits)
    \brief Create a new hash table.
    The _bits_ argument is the number of bits to use for the hash mask
    as the internal hash table size is set as a power of 2.
 */
extern value arc_tbl_new(arc *c, int nbits);

/*! \fn value __arc_tbl_insert(arc *c, value tbl, const value k, const value v)
    \brief Insert _k_ mapping to _v_ into tbl
 */
extern value __arc_tbl_insert(arc *c, value tbl, const value k, const value v);

/*! \fn value __arc_tbl_lookup(arc *c, value tbl, value k)
    \brief Lookup mapping of _k_ in _tbl_, _CUNBOUND_ if not found
 */
extern value __arc_tbl_lookup(arc *c, value tbl, value k);

/*! \fn value __arc_tbl_lookup_full(arc *c, value tbl, value k, uint64_t (*hash)(arc *, value, uint64_t), int (*is)(arc *, value, value))
    \brief Lookup mapping of _k_ in _tbl_, _CUNBOUND_ if not found
    Full lookup permitting override of hash and comparison functions.
 */

extern value __arc_tbl_lookup_full(arc *c, value tbl, value k,
				   uint64_t (*hash)(arc *, value, uint64_t),
				   int (*is)(arc *, value, value));

/*! \fn value __arc_tbl_lookup_str(arc *c, value tbl, const char *key)
    \brief Look up a C UTF-8 string in a table
*/
extern value __arc_tbl_lookup_cstr(arc *c, value tbl, const char *key);


/*! \fn value __arc_tbl_delete(arc *c, value tbl, value k)
    \brief Delete mapping of _k_ (if any) in _tbl_
 */
extern value __arc_tbl_delete(arc *c, value tbl, value k);

/* =========== Definitions and prototypes for runes (characters) */

/* \typedef Rune
   \brief UCS-4 rune type
 */
typedef int32_t Rune;

/*! \var __arc_rune_t
    \brief Type definition struct for runes
 */
extern arctype __arc_rune_t;

/*! \fn value arc_rune_new(arc *c, Rune r)
    \brief Create or obtain a new rune
 */
extern value arc_rune_new(arc *c, Rune r);

/*! \fn Rune arc_rune(value r)
    \brief Get the value of a rune
 */
static inline Rune arc_rune(value r) { return(*((Rune *)r)); }

/* =========== Definitions and prototypes for strings */

/*! \var __arc_string_t
    \brief Type definition structure for strings
 */
extern arctype __arc_string_t;

/*! \fn value arc_string_new(arc *c, int len, Rune ch)
    \brief Create a new string.
    Create a new string of length _len_ filled with _ch_.
 */
extern value arc_string_new(arc *c, int len, Rune ch);

/*! \fn value arc_string_new_str(arc *c, int len, Rune *s)
    \brief Create a new string from an array of Runes
    Create a new string of length _len_ from an array of Runes _s_.
 */
extern value arc_string_new_str(arc *c, int len, Rune *s);

/*! \fn value arc_string_new_cstr(arc *c, char *str)
    \brief Create a new string from a C string
    Create a new string from a UTF-8 encoded null-terminated C string.
 */
extern value arc_string_new_cstr(arc *c, const char *cstr);

/*! \fn unsigned int arc_strlen(arc *, value v)
    \brief Get the length of a string
 */
extern unsigned int arc_strlen(arc *c, value v);

/*! \fn Rune arc_strindex(arc *c, value s, int index)
    \brief Get the Rune at _index_ in _s_
 */
extern Rune arc_strindex(arc *c, value s, int index);

/*! \fn Rune arc_strsetindex(arc *c, value s, int index, Rune r)
    \brief Set the rune at _index_ in _s_ to _r_
 */
extern Rune arc_strsetindex(arc *c, value s, int index, Rune r);

/*! \fn Rune arc_strcatrune(arc *c, value s, Rune r)
    \brief Add rune _r_ to the end of _s_
 */
extern value arc_strcatrune(arc *c, value s, Rune r);

/*! \fn value arc_substr(arc *c, value s, unsigned int sidx, unsigned int eidx)
    \brief Return a substring of _s_ between indices _sidx_ and _eidx_
 */
extern value arc_substr(arc *c, value s, unsigned int sidx, unsigned int eidx);

/*! \fn value arc_strcat(arc *c, value s1, value s2)
    \brief Return a string that is the concatenation of _s1_ and _s2_
 */
extern value arc_strcat(arc *c, value s1, value s2);

/*! \fn unsigned int arc_strutflen(arc *c, value s)
    \brief Return the length of _s_ in UTF-8 bytes
 */
extern unsigned int arc_strutflen(arc *c, value s);

/*! \fn int arc_strrune(arc *c, value s, Rune r)
    \brief Return the index in _s_ where rune _r_ is first encountered.
    Returns -1 if the _r_ is not in _s_
*/
extern int arc_strrune(arc *c, value s, Rune r);

/*! \fn char *arc_str2cstr(arc *c, value s, char *ptr)
    \brief Convert _s_ into a null-terminated UTF-8 C string in _ptr_
    _ptr_ must be big enough to hold the UTF-8 representation of _s_. This
    will not work properly if _s_ has null characters anywhere.
 */
extern char *arc_str2cstr(arc *c, value s, char *ptr);

/*! \fn int arc_is_str_cstr(arc *c, const char *s1, const value s2)
    \brief Compare a UTF-8 string and Arcueid string
 */
extern int arc_is_str_cstr(arc *c, const char *s1, const value s2);

/*! \fn value arc_strdup(arc *c, const value s)
    \brief Duplicate a string 
 */
extern value arc_strdup(arc *c, const value s);

/* =========== Definitions and prototypes for symbols */

/*! \var __arc_sym_t
    \brief Type definition structure for symbols
 */
extern arctype __arc_sym_t;

/*! \fn value arc_intern_cstr(arc *c, const char *s)
    \brief Intern a UTF-8 C string into the symbol table.
 */
extern value arc_intern_cstr(arc *c, const char *s);

/*! \fn value arc_intern(arc *c, value s)
    \brief Intern a string into the symbol table.
 */
extern value arc_intern(arc *c, value s);

/*! \fn value arc_sym2name(arc *c, value sym)
    \brief Return a string representation of a symbol
 */
extern value arc_sym2name(arc *c, value sym);

/* =========== Definitions and prototypes for threads */

/*! \var __arc_thread_t
    \brief Type definition structure for threads
 */
extern arctype __arc_thread_t;

/* =========== Definitions and prototypes for utility functions */

/*! \struct ranctx
    \brief PRNG context structure
 */
struct ranctx {
  uint64_t a;
  uint64_t b;
  uint64_t c;
  uint64_t d;
};

/*! \fn uint64_t __arc_rand(struct ranctx *x)
    \brief Pseudo-random number generator
 */
extern uint64_t __arc_rand(struct ranctx *x);
/*! \fn void __arc_srand(struct ranctx *x, uint64_t seed)
    \brief Seed the PRNG
 */
extern void __arc_srand(struct ranctx *x, uint64_t seed);

/*! \fn uint64_t __arc_random(struct ranctx *x, uint64_t n)
    \brief Return pseudo-random number between 0 and n-1
 */
uint64_t __arc_random(struct ranctx *x, uint64_t n);

#endif
