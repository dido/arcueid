/* 
  Copyright (C) 2012 Rafael R. Sevilla

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
#include <inttypes.h>
#include <string.h>
#include "arcueid.h"
#include "alloc.h"
#include "arith.h"
#include "utf.h"
#include "../config.h"

#if SIZEOF_LONG >= 8

/* This is the 64-bit hashing function defined by Bob Jenkins in
   lookup8.c.  The hash function has been verified to be exactly the
   same on x86_64 machines. */

#define HASH64BITS

#define MIX(a,b,c)				\
  {						\
    a -= b; a -= c; a ^= (c>>43);		\
    b -= c; b -= a; b ^= (a<<9);		\
    c -= a; c -= b; c ^= (b>>8);		\
    a -= b; a -= c; a ^= (c>>38);		\
    b -= c; b -= a; b ^= (a<<23);		\
    c -= a; c -= b; c ^= (b>>5);		\
    a -= b; a -= c; a ^= (c>>35);		\
    b -= c; b -= a; b ^= (a<<49);		\
    c -= a; c -= b; c ^= (b>>11);		\
    a -= b; a -= c; a ^= (c>>12);		\
    b -= c; b -= a; b ^= (a<<18);		\
    c -= a; c -= b; c ^= (b>>22);		\
  }

#define FINAL(a,b,c) MIX(a,b,c)

void arc_hash_init(arc_hs *s, unsigned long level)
{
  s->s[0] = s->s[1] = level;
  s->s[2] = 0x9e3779b97f4a7c13LL;
  s->state = 0;
}

#else

#define HASH32BITS

/* NOTE: The 32-bit hash function here is sort of based on Bob Jenkins'
   lookup3.c, however, unlike the 64-bit hash function, this is not
   exactly the same as Jenkins' algorithm.  This is because lookup3.c
   uses the length of the hashed data at the beginning of the hashing
   process, so we can't compute the hash incrementally way and must
   have the length of the function to begin with if we are to use
   Jenkins' actual algorithm.  What we have here is a slight variant
   which uses the length near the end the way the 64-bit hash function
   does.  I cannot be sure whether this minor change might have
   affected the characteristics of the hash, but my gut doubts it. */
#define ROT(x,k) (((x)<<(k)) | ((x)>>(32-(k))))


#define MIX(a,b,c) \
{ \
  a -= c;  a ^= ROT(c, 4);  c += b; \
  b -= a;  b ^= ROT(a, 6);  a += c; \
  c -= b;  c ^= ROT(b, 8);  b += a; \
  a -= c;  a ^= ROT(c,16);  c += b; \
  b -= a;  b ^= ROT(a,19);  a += c; \
  c -= b;  c ^= ROT(b, 4);  b += a; \
}

#define FINAL(a,b,c) \
{ \
  c ^= b; c -= ROT(b,14); \
  a ^= c; a -= ROT(c,11); \
  b ^= a; b -= ROT(a,25); \
  c ^= b; c -= ROT(b,16); \
  a ^= c; a -= ROT(c,4);  \
  b ^= a; b -= ROT(a,14); \
  c ^= b; c -= ROT(b,24); \
}

void arc_hash_init(arc_hs *s, unsigned long level)
{
  s->s[0] = s->s[1] = s->s[2] = 0xdeadbeef + level;
  s->state = 0;
}

#endif

void arc_hash_update(arc_hs *s, unsigned long val)
{
  s->s[s->state] += val;
  s->state = (s->state + 1) % 3;
  if (s->state == 0)
    MIX(s->s[0], s->s[1], s->s[2]);
}

unsigned long arc_hash_final(arc_hs *s, unsigned long len)
{
  s->s[2] += len << 3;
  FINAL(s->s[0], s->s[1], s->s[2]);
  return(s->s[2]);
}

#ifdef HAVE_GMP_H

static unsigned long hash_bignum(arc *c, arc_hs *s, mpz_t bignum)
{
  unsigned long *rop;
  size_t numb = sizeof(unsigned long);
  size_t countp, calc_size;
  int i;

  calc_size = (mpz_sizeinbase(bignum,  2) + numb-1) / numb;
  rop = c->get_block(c, calc_size * numb);
  BLOCK_IMM(rop);
  mpz_export(rop, &countp, 1, numb, 0, 0, bignum);
  for (i=0; i<countp; i++)
    arc_hash_update(s, rop[i]);
  c->free_block(c, rop);
  return((unsigned long)countp);
}

#endif

/* incrementally calculate the hash code--may recurse for complex
   data structures. */
static unsigned long hash_increment(arc *c, value v, arc_hs *s)
{
  union {
    double d;
#if SIZEOF_LONG == 8
    uint64_t a;
#else
    uint32_t a[2];
#endif
  } dbl;
  unsigned long length=1;
  int i;
  value v2;

  arc_hash_update(s, TYPE(v));
  switch (TYPE(v)) {
  case T_NIL:
    break;
  case T_TRUE:
    break;
  case T_FIXNUM:
    arc_hash_update(s, (unsigned long)FIX2INT(v));
    break;
  case T_FLONUM:
    dbl.d = REP(v)._flonum;
#if SIZEOF_LONG == 8
    arc_hash_update(s, (unsigned long)dbl.a);
#else
    arc_hash_update(s, (unsigned long)dbl.a[0]);
    arc_hash_update(s, (unsigned long)dbl.a[1]);
    length = 2;
#endif
    break;
  case T_COMPLEX:
    dbl.d = REP(v)._complex.re;
#if SIZEOF_LONG == 8
    arc_hash_update(s, (unsigned long)dbl.a);
#else
    arc_hash_update(s, (unsigned long)dbl.a[0]);
    arc_hash_update(s, (unsigned long)dbl.a[1]);
#endif
    dbl.d = REP(v)._complex.im;
#if SIZEOF_LONG == 8
    arc_hash_update(s, (unsigned long)dbl.a);
    length = 2;
#else
    arc_hash_update(s, (unsigned long)dbl.a[0]);
    arc_hash_update(s, (unsigned long)dbl.a[1]);
    length = 4;
#endif
    break;
#ifdef HAVE_GMP_H
  case T_BIGNUM:
    length = hash_bignum(c, s, REP(v)._bignum);
    break;
  case T_RATIONAL:
    length = hash_bignum(c, s, mpq_numref(REP(v)._rational));
    length += hash_bignum(c, s, mpq_denref(REP(v)._rational));
    break;
#endif
  case T_CONS:
  case T_TAGGED:
  case T_CLOS:
  case T_ENV:
    /* XXX: This will recurse forever if the cons cells self-reference.
       We need to use a more sophisticated algorithm to handle cycles. */
    length += hash_increment(c, car(v), s);
    length += hash_increment(c, cdr(v), s);
    break;
  case T_STRING:
    length = arc_strlen(c, v);
    for (i=0; i<length; i++)
      arc_hash_update(s, (unsigned long)arc_strindex(c, v, i));
    break;
  case T_CHAR:
    length = 1;
    arc_hash_update(s, REP(v)._char);
    break;
  case T_SYMBOL:
    length = 1;
    arc_hash_update(s, (unsigned long)SYM2ID(v));
    break;
  case T_TABLE:
    while ((v2 = arc_hash_iter(c, v, &i)) != CUNBOUND)
      length += hash_increment(c, v2, s);
    break;
  case T_TBUCKET:
    length += hash_increment(c, REP(v)._hashbucket.key, s);
    length += hash_increment(c, REP(v)._hashbucket.val, s);
    break;
  case T_INPUT:
  case T_OUTPUT:
  case T_PORT:
  case T_CUSTOM:
    length = REP(v)._custom.hash(c, s, v);
    break;
  case T_THREAD:
    length = 1;
    arc_hash_update(s, TTID(v));
    break;
  case T_EXCEPTION:
  case T_VECTOR:
  case T_CONT:
  case T_CODE:
  case T_XCONT:
  case T_CHAN:
    for (i=0; i<VECLEN(v); i++)
      length += hash_increment(c, VINDEX(v, i), s);
    break;
  case T_VMCODE:
    length = VECLEN(v);
    for (i=0; i<length; i++)
      arc_hash_update(s, (unsigned long)VINDEX(v, i));
    break;
  case T_CCODE:
    length += hash_increment(c, REP(v)._cfunc.name, s);
    arc_hash_update(s, REP(v)._cfunc.argc);
    break;
  case T_NONE:
    arc_err_cstrfmt(c, "hash_increment: object of undefined type encountered!");
    break;
  }
  return(length);
}

unsigned long arc_hash_increment(arc *c, value v, arc_hs *s)
{
  return(hash_increment(c, v, s));
}


static unsigned long hash_increment_string(const char *s, arc_hs *hs)
{
  int c;
  long n;
  Rune rune;

  n = 0;
  for (;;) {
    c = *(unsigned char *)s;
    if (c == 0) {
      /* End of string */
      return(n);
    }
    s += chartorune(&rune, s);
    arc_hash_update(hs, (unsigned long)rune);
    n++;
  }
  return(0);
}

/* This function should produce exactly the same results as
   arc_hash(c, arc_mkstring(c, str)) */
unsigned long arc_hash_cstr(arc *c, const char *str)
{
  unsigned long len;
  arc_hs s;

  arc_hash_init(&s, 0);
  arc_hash_update(&s, T_STRING);
  len = hash_increment_string(str, &s);
  return(arc_hash_final(&s, len));
}

unsigned long arc_hash(arc *c, value v)
{
  unsigned long len;
  arc_hs s;

  arc_hash_init(&s, 0);
  len = hash_increment(c, v, &s);
  return(arc_hash_final(&s, len));
}

#define HASHSIZE(n) ((unsigned long)1 << (n))
#define HASHMASK(n) (HASHSIZE(n) - 1)
#define MAX_LOAD_FACTOR 70	/* percentage */
#define TABLESIZE(t) (HASHSIZE(REP(t)._hash.hashbits))
#define TABLEMASK(t) (HASHMASK(REP(t)._hash.hashbits))
#define TABLEPTR(t) (REP(t)._hash.table)
#define PROBE(i) ((i + i*i) >> 1)

/* An empty slot is either CUNBOUND or CUNDEF.  CUNDEF is used as a
   'tombstone' value for deleted elements.  If this is found, one may have
   to keep probing until either the actual element is found or one runs into
   a CUNBOUND, meaning the element is definitely not in the table.
   Since we enforce load factor, there will definitely be some table
   elements which remain unused. */
#define EMPTYP(x) (((x) == CUNBOUND) || ((x) == CUNDEF))

value arc_mkhash(arc *c, int hashbits)
{
  value hash;
  int i;

  hash = c->get_cell(c);
  BTYPE(hash) = T_TABLE;
  REP(hash)._hash.hashbits = hashbits;
  REP(hash)._hash.nentries = 0;
  REP(hash)._hash.loadlimit = (HASHSIZE(hashbits) * MAX_LOAD_FACTOR) / 100;
  TABLEPTR(hash) = (value *)c->get_block(c, HASHSIZE(hashbits)*sizeof(value));
  for (i=0; i<HASHSIZE(hashbits); i++)
    REP(hash)._hash.table[i] = CUNBOUND;
  BLOCK_IMM(REP(hash)._hash.table);
  return(hash);
}

/* Grow a hash table */
static void hashtable_expand(arc *c, value hash)
{
  unsigned int hv, index, i, j, nhashbits;
  value *oldtbl, *newtbl, e;

  nhashbits = REP(hash)._hash.hashbits+1;

  newtbl = (value *)c->get_block(c, HASHSIZE(nhashbits)*sizeof(value));
  for (i=0; i<HASHSIZE(nhashbits); i++)
    newtbl[i] = CUNBOUND;
  BLOCK_IMM(newtbl);
  oldtbl = TABLEPTR(hash);
  /* Search for active keys and copy them into the new table */
  for (i=0; i<TABLESIZE(hash); i++) {
    if (!EMPTYP(oldtbl[i])) {
      e = oldtbl[i];
      /* insert the old key into the new table */
      hv = arc_hash(c, car(oldtbl[i])); /* hash the key again */
      index = hv & HASHMASK(nhashbits);
      for (j=0; !EMPTYP(newtbl[index]); j++)
	index = (index + PROBE(j)) & HASHMASK(nhashbits);
      newtbl[index] = e;
      REP(e)._hashbucket.index = index; /* change index */
    }
  }
  c->free_block(c, (void *)oldtbl);
  REP(hash)._hash.hashbits = nhashbits;
  REP(hash)._hash.loadlimit = (HASHSIZE(nhashbits) * MAX_LOAD_FACTOR) / 100;
  TABLEPTR(hash) = newtbl;
}

static value mkhashbucket(arc *c, value key, value val, value hash, int index)
{
  value bucket;

  bucket = c->get_cell(c);
  BTYPE(bucket) = T_TBUCKET;
  REP(bucket)._hashbucket.key = key;
  REP(bucket)._hashbucket.val = val;
  REP(bucket)._hashbucket.hash = hash;
  REP(bucket)._hashbucket.index = index;
  return(bucket);
}

value arc_hash_insert(arc *c, value hash, value key, value val)
{
  unsigned int hv, index, i;
  value e;

  if (++(REP(hash)._hash.nentries) > REP(hash)._hash.loadlimit)
    hashtable_expand(c, hash);
  hv = arc_hash(c, key);
  index = hv & TABLEMASK(hash);
  /* Hash table entries are cons cells whose car is the key and the
     cdr is the value */
  /* Collision resolution with open addressing */
  i = 0;
  for (i=0; !EMPTYP(TABLEPTR(hash)[index]) && !arc_is(c, car(TABLEPTR(hash)[index]), key); i++)
    index = (index + PROBE(i)) & TABLEMASK(hash); /* quadratic probe */
  e = mkhashbucket(c, key, val, hash, index);
  WB(&TABLEPTR(hash)[index], e);
  return(val);
}

static value hash_lookup(arc *c, value hash, value key, unsigned int *index)
{
  unsigned int hv, i;
  value e;

  hv = arc_hash(c, key);
  *index = hv & TABLEMASK(hash);
  for (i=0;; i++) {
    *index = (*index + PROBE(i)) & TABLEMASK(hash);
    e = TABLEPTR(hash)[*index];
    if (e == CUNBOUND)
      return(CUNBOUND);
    if (e == CUNDEF)
      continue;
    if (arc_iso(c, REP(e)._hashbucket.key, key) == CTRUE)
      return(REP(e)._hashbucket.val);
  }
}

static value hash_lookup_cstr(arc *c, value hash, const char *key,
			      unsigned int *index)
{
  unsigned int hv, i, n, ch;
  value e, k2;
  Rune rune;
  const char *kptr;
  int equal;

  hv = arc_hash_cstr(c, key);
  *index = hv & TABLEMASK(hash);
  for (i=0;; i++) {
    *index = (*index + PROBE(i)) & TABLEMASK(hash);
    e = TABLEPTR(hash)[*index];
    if (e == CUNBOUND)
      return(CUNBOUND);
    if (e == CUNDEF)
      continue;
    if (TYPE(REP(e)._hashbucket.key) == T_STRING) {
      /* Compare the string of the key to this one */
      k2 = REP(e)._hashbucket.key;
      n = 0;
      equal = 0;
      kptr = key;
      for (;;) {
	ch = *(unsigned char *)kptr;
	if (ch == 0) {
	  equal = (arc_strlen(c, k2) == n);
	  break;
	}
	kptr += chartorune(&rune, kptr);
	if (rune != arc_strindex(c, k2, n)) {
	  equal = 0;
	  break;
	}
	n++;
      }
      if (equal)
	return(REP(e)._hashbucket.val);
    }
  }
}

value arc_hash_lookup(arc *c, value hash, value key)
{
  unsigned int index;

  return(hash_lookup(c, hash, key, &index));
}

value arc_hash_lookup_cstr(arc *c, value hash, const char *key)
{
  unsigned int index;

  return(hash_lookup_cstr(c, hash, key, &index));
}

/* Slightly different version which returns the actual cons cell with
   the key and value if a binding is available. */
value arc_hash_lookup2(arc *c, value hash, value key)
{
  unsigned int index;
  value val;

  val = hash_lookup(c, hash, key, &index);
  if (val == CUNBOUND)
    return(CUNBOUND);
  return(TABLEPTR(hash)[index]);
}


value arc_hash_delete(arc *c, value hash, value key)
{
  unsigned int index;
  value v;

  v = hash_lookup(c, hash, key, &index);
  if (v != CUNBOUND)
    WB(&TABLEPTR(hash)[index], CUNDEF);
  return(v);
}

value arc_hash_iter(arc *c, value hash, int *index)
{
  if ((*index) >= TABLESIZE(hash))
    return(CUNBOUND);
  for (;;) {
    if (*index >= TABLESIZE(hash))
      break;
    if (!EMPTYP(TABLEPTR(hash)[*index]))
      return(TABLEPTR(hash)[(*index)++]);
    ++(*index);
  }
  return(CUNBOUND);
}

int arc_hash_length(arc *c, value hash)
{
  int count = 0;
  int index = 0;

  while (arc_hash_iter(c, hash, &index) != CUNBOUND)
    count++;
  return(count);
}

/* maptable */
value arc_hash_map(arc *c, value argv, value rv, CC4CTX)
{
  value proc, table;
  CC4VDEFBEGIN;
  CC4VARDEF(i);
  CC4VDEFEND;

  if (VECLEN(argv) != 2) {
    arc_err_cstrfmt(c, "wrong number of arguments (%d for 2)", VECLEN(argv));
    return(CNIL);
  }
  proc = VINDEX(argv, 0);
  table = VINDEX(argv, 1);
  if (TYPE(table) != T_TABLE) {
    arc_err_cstrfmt(c, "maptable expects hash as 2nd argument, given %O",
		    table);
    return(CNIL);
  }
  if (TYPE(proc) != T_CLOS && TYPE(proc) != T_CCODE) {
    arc_err_cstrfmt(c, "maptable expects function as 1st argument, given %O",
		    table);
    return(CNIL);
  }
  CC4BEGIN(c);
  for (CC4V(i) = INT2FIX(0); FIX2INT(CC4V(i))<TABLESIZE(table);
       CC4V(i) = __arc_add2(c, CC4V(i), INT2FIX(1))) {
    value tent = TABLEPTR(table)[FIX2INT(CC4V(i))];
    if (!EMPTYP(tent))
      CC4CALL(c, argv, proc, 2, car(tent), cdr(tent));
  }
  CC4END;
  return(table);
}

#define DEFAULT_TABLE_BITS 8

value arc_table(arc *c)
{
  return(arc_mkhash(c, DEFAULT_TABLE_BITS));
}

/* This should be called only on hashes */
value __arc_hash_iso(arc *c, value v1, value v2)
{
  int i;
  value cell, tent;

  /* iterate over both tables */
  for (i=0; i<TABLESIZE(v1); i++) {
    tent = TABLEPTR(v1)[i];
    if (EMPTYP(tent))
      continue;
    cell = arc_hash_lookup2(c, v2, car(tent));
    if (cell == CUNBOUND)
      return(CNIL);
    if (arc_iso(c, car(cell), car(tent)) == CNIL
	|| arc_iso(c, cdr(cell), cdr(tent)) == CNIL)
      return(CNIL);
  }

  for (i=0; i<TABLESIZE(v2); i++) {
    tent = TABLEPTR(v2)[i];
    if (EMPTYP(tent))
      continue;
    cell = arc_hash_lookup2(c, v1, car(tent));
    if (cell == CUNBOUND)
      return(CNIL);
    if (arc_iso(c, car(cell), car(tent)) == CNIL
	|| arc_iso(c, cdr(cell), cdr(tent)) == CNIL)
      return(CNIL);
  }
  return(CTRUE);
}
