/* 
  Copyright (C) 2009 Rafael R. Sevilla

  This file is part of CArc

  CArc is free software; you can redistribute it and/or modify it
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
#include <inttypes.h>
#include <string.h>
#include "carc.h"
#include "alloc.h"
#include "utf.h"
#include "coroutine.h"
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

void carc_hash_init(carc_hs *s, unsigned long level)
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

void carc_hash_init(carc_hs *s, unsigned long level)
{
  s->s[0] = s->s[1] = s->s[2] = 0xdeadbeef + level;
  s->state = 0;
}

#endif

void carc_hash_update(carc_hs *s, unsigned long val)
{
  s->s[s->state] += val;
  s->state = (s->state + 1) % 3;
  if (s->state == 0)
    MIX(s->s[0], s->s[1], s->s[2]);
}

unsigned long carc_hash_final(carc_hs *s, unsigned long len)
{
  s->s[2] += len << 3;
  FINAL(s->s[0], s->s[1], s->s[2]);
  return(s->s[2]);
}

#ifdef HAVE_GMP_H

static unsigned long hash_bignum(carc *c, carc_hs *s, mpz_t bignum)
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
    carc_hash_update(s, rop[i]);
  c->free_block(c, rop);
  return((unsigned long)countp);
}

#endif

/* incrementally calculate the hash code--may recurse for complex
   data structures. */
static unsigned long hash_increment(carc *c, value v, carc_hs *s)
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

  switch (TYPE(v)) {
  case T_NIL:
    carc_hash_update(s, T_NIL);
    break;
  case T_TRUE:
    carc_hash_update(s, T_TRUE);
    break;
  case T_FIXNUM:
    carc_hash_update(s, (unsigned long)FIX2INT(v));
    break;
  case T_FLONUM:
    dbl.d = REP(v)._flonum;
#if SIZEOF_LONG == 8
    carc_hash_update(s, (unsigned long)dbl.a);
#else
    carc_hash_update(s, (unsigned long)dbl.a[0]);
    carc_hash_update(s, (unsigned long)dbl.a[1]);
    length = 2;
#endif
    break;
  case T_COMPLEX:
    dbl.d = REP(v)._complex.re;
#if SIZEOF_LONG == 8
    carc_hash_update(s, (unsigned long)dbl.a);
#else
    carc_hash_update(s, (unsigned long)dbl.a[0]);
    carc_hash_update(s, (unsigned long)dbl.a[1]);
#endif
    dbl.d = REP(v)._complex.im;
#if SIZEOF_LONG == 8
    carc_hash_update(s, (unsigned long)dbl.a);
    length = 2;
#else
    carc_hash_update(s, (unsigned long)dbl.a[0]);
    carc_hash_update(s, (unsigned long)dbl.a[1]);
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
    /* XXX: This will recurse forever if the cons cells self-reference.
       We need to use a more sophisticated algorithm to handle cycles. */
    carc_hash_update(s, T_CONS);
    length += hash_increment(c, car(v), s);
    length += hash_increment(c, cdr(v), s);
    break;
  case T_STRING:
    length = carc_strlen(c, v);
    for (i=0; i<length; i++)
      carc_hash_update(s, (unsigned long)carc_strindex(c, v, i));
    break;
  }
  return(length);
}

static unsigned long hash_increment_string(const char *s, carc_hs *hs)
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
    carc_hash_update(hs, (unsigned long)rune);
    n++;
  }
  return(0);
}

/* This function should produce exactly the same results as
   carc_hash(c, carc_mkstring(c, str)) */
unsigned long carc_hash_cstr(carc *c, const char *str)
{
  unsigned long len;
  carc_hs s;

  carc_hash_init(&s, 0);
  len = hash_increment_string(str, &s);
  return(carc_hash_final(&s, len));
}

unsigned long carc_hash(carc *c, value v)
{
  unsigned long len;
  carc_hs s;

  carc_hash_init(&s, 0);
  len = hash_increment(c, v, &s);
  return(carc_hash_final(&s, len));
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

value carc_mkhash(carc *c, int hashbits)
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
static void hashtable_expand(carc *c, value hash)
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
      hv = carc_hash(c, car(oldtbl[i])); /* hash the key again */
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

static value mkhashbucket(carc *c, value key, value val, value hash, int index)
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

value carc_hash_insert(carc *c, value hash, value key, value val)
{
  unsigned int hv, index, i;
  value e;

  if (++(REP(hash)._hash.nentries) > REP(hash)._hash.loadlimit)
    hashtable_expand(c, hash);
  hv = carc_hash(c, key);
  index = hv & TABLEMASK(hash);
  /* Hash table entries are cons cells whose car is the key and the
     cdr is the value */
  /* Collision resolution with open addressing */
  i = 0;
  for (i=0; !EMPTYP(TABLEPTR(hash)[index]) && !carc_is(c, car(TABLEPTR(hash)[index]), key); i++)
    index = (index + PROBE(i)) & TABLEMASK(hash); /* quadratic probe */
  e = mkhashbucket(c, key, val, hash, index);
  WB(&TABLEPTR(hash)[index], e);
  return(val);
}

static value hash_lookup(carc *c, value hash, value key, unsigned int *index)
{
  unsigned int hv, i;
  value e;

  hv = carc_hash(c, key);
  *index = hv & TABLEMASK(hash);
  for (i=0;; i++) {
    *index = (*index + PROBE(i)) & TABLEMASK(hash);
    e = TABLEPTR(hash)[*index];
    if (e == CUNBOUND)
      return(CUNBOUND);
    if (e == CUNDEF)
      continue;
    if (carc_iso(c, REP(e)._hashbucket.key, key) == CTRUE)
      return(REP(e)._hashbucket.val);
  }
}

static value hash_lookup_cstr(carc *c, value hash, const char *key,
			      unsigned int *index)
{
  unsigned int hv, i, n, ch;
  value e, k2;
  Rune rune;
  const char *kptr;
  int equal;

  hv = carc_hash_cstr(c, key);
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
	  equal = (carc_strlen(c, k2) == n);
	  break;
	}
	kptr += chartorune(&rune, kptr);
	if (rune != carc_strindex(c, k2, n)) {
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

value carc_hash_lookup(carc *c, value hash, value key)
{
  unsigned int index;

  return(hash_lookup(c, hash, key, &index));
}

value carc_hash_lookup_cstr(carc *c, value hash, const char *key)
{
  unsigned int index;

  return(hash_lookup_cstr(c, hash, key, &index));
}

/* Slightly different version which returns the actual cons cell with
   the key and value if a binding is available. */
value carc_hash_lookup2(carc *c, value hash, value key)
{
  unsigned int index;
  value val;

  val = hash_lookup(c, hash, key, &index);
  if (val == CUNBOUND)
    return(CUNBOUND);
  return(TABLEPTR(hash)[index]);
}


value carc_hash_delete(carc *c, value hash, value key)
{
  unsigned int index;
  value v;

  v = hash_lookup(c, hash, key, &index);
  if (v != CUNBOUND)
    WB(&TABLEPTR(hash)[index], CUNDEF);
  return(v);
}

value carc_hash_iter(carc *c, value hash, ccrContParam)
{
  ccrBeginContext;
  unsigned int index;
  ccrEndContext(ctx);

  ccrBegin(ctx);
  for (ctx->index=0; ctx->index < TABLESIZE(hash); ctx->index++) {
    if (!EMPTYP(TABLEPTR(hash)[ctx->index]))
      ccrReturn(TABLEPTR(hash)[ctx->index]);
  }
  ccrFinish(CUNBOUND);
}
