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
#include "arcueid.h"
#include "../config.h"
#include "hash.h"

/* Arcueid's hash function is a Cuckoo hash with by default three
   hashes (which are implemented by using three different seeds for
   MurmurHash3) with a stash for difficult keys. */


/* The hash function here is a hash based on the Murmur3 hash function.
   Arcueid always hashes blocks of data in 64-bit chunks, even on 32-bit
   systems using the 32-bit hash. */

#if SIZEOF_LONG >= 8

#define HASH64BITS

static inline uint64_t rotl64(uint64_t x, int8_t r)
{
  return((x << r) | (x >> (64 - r)));
}

static inline uint64_t fmix64(uint64_t k)
{
  k ^= k >> 33;
  k *= 0xff51afd7ed558ccdULL;
  k ^= k >> 33;
  k *= 0xc4ceb9fe1a85ec53ULL;
  k ^= k >> 33;
  return(k);
}

#define C1 0x87c37b91114253d5ULL
#define C2 0x4cf5ad432745937fULL

void __arc_hash_init(struct hash_ctx *ctx, uint64_t seed)
{
  ctx->h1 = ctx->h2 = seed;
  ctx->len = 0;
}

void __arc_hash_update(struct hash_ctx *ctx, const uint64_t *data,
		       const int len)
{
  int i;

  ctx->len += len*8;
  for (i=0; i<len; i++) {
    uint64_t k = data[i];

    if ((i & 0x1) == 0) {
      k *= C1;
      k = rotl64(k,31);
      k *= C2;
      ctx->h1 ^= k;
      ctx->h1 = rotl64(ctx->h1, 27);
      ctx->h1 += ctx->h2;
      ctx->h1 = ctx->h1*5 + 0x52dce729;
    } else {
      k *= C2;
      k = rotl64(k,33);
      k *= C1;
      ctx->h2 ^= k;
      ctx->h2 = rotl64(ctx->h2, 31);
      ctx->h2 += ctx->h1;
      ctx->h2 = ctx->h2*5 + 0x38495ab5;
    }
  }
}

uint64_t __arc_hash_final(struct hash_ctx *ctx)
{
  ctx->h1 ^= ctx->len;
  ctx->h2 ^= ctx->len;
  ctx->h1 += ctx->h2;
  ctx->h2 += ctx->h1;
  ctx->h1 = fmix64(ctx->h1);
  ctx->h2 = fmix64(ctx->h2);
  ctx->h1 += ctx->h2;
  ctx->h2 += ctx->h1;
  return(ctx->h1);
}

static const uint64_t hashseeds[NHASH] = {
  0x93c467e37db0c7a4ULL,	/* Euler-Mascheroni constant */
  0xb7e151628aed2a6aULL,	/* fractional digits of e */
  0x9e3779b9f4a7c15fULL		/* Golden ratio */
  /* more seeds need to be defined if more hashes are wanted. */
};

#else

#define HASH32BITS
#define C1 0xcc9e2d51
#define C2 0x1b873593

static inline uint32_t rotl32(uint32_t x, int8_t r)
{
  return((x << r) | (x >> (32 - r)));
}
			 
void __arc_hash_init(struct hash_ctx *ctx, uint64_t seed)
{
  ctx->h1 = seed;
  ctx->len = 0;
}

void __arc_hash_update(struct hash_ctx *ctx, const uint64_t *data,
		       const int len)
{
  int i, j;
  uint32_t h1;

  ctx->len += len*8;
  for (i=0; i<len; i++) {
    union { uint64_t v; uint32_t b[2]; } u;
    u.v = data[i];
    for (j=0; j<2; j++) {
      uint32_t k1 = u.b[j];
      k1 *= C1;
      k1 = rotl32(k1, 15);
      k1 *= C2;
      h1 = (uint32_t)ctx->h1;
      h1 ^= k1;
      h1 = rotl32(h1, 13);
      h1 = h1*5 + 0xe6546b64;
      ctx->h1 = h1;
    }
  }
}

uint64_t __arc_hash_final(struct hash_ctx *ctx)
{
  uint32_t h = ctx->h1;

  h ^= (uint32_t)ctx->len;
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;
  return(h);
}

#endif

uint64_t __arc_hash(arc *c, value v, uint64_t seed)
{
  arctype *t = arc_type(v);
  return(t->hash(c, v, seed));
}

static void tmark(arc *c, value v,
		  void (*marker)(struct arc *, value, int),
		  int depth)
{
  hashtbl *ht = (hashtbl *)v;
  int i;

  for (i=0; i<NHASH; i++) {
    marker(c, ht->k, depth);
    marker(c, ht->v, depth);
  }
}

arctype __arc_tbl_t = { NULL, tmark, NULL, NULL, NULL, NULL };

#define MIN(x,y) (((x) > (y)) ? (y) : (x))
#define MAX(x,y) (((x) > (y)) ? (x) : (y))

static void push_keys(arc *c, value tbl, value k, value v,
		      uint64_t *hashes);
static void resize(arc *c, value tbl);

static int stashsize(int bits)
{
  return(MAX(3, bits*2));
}

static value default_getkey(arc *c, value tbl, uint64_t hash)
{
  return(HIDX(tbl, k, hash));
}


static value default_getval(arc *c, value tbl, uint64_t hash)
{
  return(HIDX(tbl, v, hash));
}

static void default_setkey(arc *c, value tbl, uint64_t hash, value key)
{
  SHIDX(c, tbl, k, hash, key);
}

static void default_setval(arc *c, value tbl, uint64_t hash, value val)
{
  SHIDX(c, tbl, v, hash, val);
}

static value weak_getkey(arc *c, value tbl, uint64_t hash)
{
  value wk = default_getkey(c, tbl, hash);
  value k = arc_wrefv(wk);
  if (NILP(k)) {
    default_setkey(c, tbl, hash, CUNBOUND);
    default_setval(c, tbl, hash, CUNBOUND);
    USAGE(tbl, --);
    k = CUNBOUND;
  }
  return(k);
}

static value weak_getval(arc *c, value tbl, uint64_t hash)
{
  value wv = default_getval(c, tbl, hash);
  value v = arc_wrefv(wv);
  if (NILP(v)) {
    default_setkey(c, tbl, hash, CUNBOUND);
    default_setval(c, tbl, hash, CUNBOUND);
    USAGE(tbl, --);
    v = CUNBOUND;
  }
  return(v);
}

static void weak_setkey(arc *c, value tbl, uint64_t hash, value key)
{
  value wk = arc_wref_new(c, key);
  default_setkey(c, tbl, hash, wk);
}

static void weak_setval(arc *c, value tbl, uint64_t hash, value val)
{
  value wv = arc_wref_new(c, val);
  default_setval(c, tbl, hash, wv);
}

static value getkey(arc *c, value tbl, uint64_t hash)
{
  return(((hashtbl *)tbl)->getkey(c, tbl, hash));
}

static value getval(arc *c, value tbl, uint64_t hash)
{
  return(((hashtbl *)tbl)->getval(c, tbl, hash));
}

static void setkey(arc *c, value tbl, uint64_t hash, value key)
{
  return(((hashtbl *)tbl)->setkey(c, tbl, hash, key));
}

static void setval(arc *c, value tbl, uint64_t hash, value val)
{
  return(((hashtbl *)tbl)->setval(c, tbl, hash, val));
}

value arc_tbl_new_flags(arc *c, int nbits, unsigned int flags)
{
  value vht = arc_new(c, &__arc_tbl_t, sizeof(hashtbl));
  hashtbl *ht = (hashtbl *)vht;
  int size, i;
  
  ht->nbits = nbits;
  size = HASHSIZE(nbits);
  ht->stashsize = stashsize(nbits);
  ht->threshold = (size * MAXLOAD)/100;
  if (flags & HASH_WEAK_KEY) {
    ht->getkey = weak_getkey;
    ht->setkey = weak_setkey;
  } else {
    ht->getkey = default_getkey;
    ht->setkey = default_setkey;
  }
  if (flags & HASH_WEAK_VAL) {
    ht->getval = weak_getval;
    ht->setval = weak_setval;
  } else {
    ht->getval = default_getval;
    ht->setval = default_setval;
  }
  ht->usage = 0;
  ht->k = arc_vector_new(c, size + ht->stashsize);
  ht->v = arc_vector_new(c, size + ht->stashsize);
  for (i=0; i<size; i++) {
    SVIDX(c, ht->k, i, CUNBOUND);
    SVIDX(c, ht->v, i, CUNBOUND);
  }
  return(vht);
}

value arc_tbl_new(arc *c, int nbits)
{
  return(arc_tbl_new_flags(c, nbits, 0));
}

value __arc_tbl_insert(arc *c, value tbl, const value k, const value v)
{
  uint64_t hashes[NHASH];
  value key;
  int i;

  USAGE(tbl, ++);
  /* Check for existing keys */
  for (i=0; i<NHASH; i++) {
    hashes[i] = __arc_hash(c, k, hashseeds[i]);
    if (__arc_is(c, getkey(c, tbl, hashes[i]), k)) {
      setval(c, tbl, hashes[i], v);
      return(v);
    }
  }

  /* Key is not there yet. Check for empty buckets. Put the key in an empty
     bucket if possible. */
  for (i=0; i<NHASH; i++) {
    key = getkey(c, tbl, hashes[i]);
    if (key == CUNBOUND) {
      setkey(c, tbl, hashes[i], k);
      setval(c, tbl, hashes[i], v);
      return(v);
    }
  }
  /* Otherwise, push the keys around until we find a place */
  push_keys(c, tbl, k, v, hashes);
  return(v);
}

/* Push keys around until we have a new space to put a key to be inserted */
static void push_keys(arc *c, value tbl, value k, value v,
		      uint64_t *hashes)
{
  value ek, ev, nk;
  struct ranctx rctx;
  uint64_t ei, eh;
  int i, p;
  int push_iterations, bits;

  /* max(min(size, 8), sqrt(size)/8) */
  bits = HASHBITS(tbl);
  push_iterations = MAX(MIN(HASHSIZE(bits), 8), HASHSIZE(bits >> 1) >> 3);
  
  /* Use first hash as seed for the PRNG */
  __arc_srand(&rctx, hashes[0]);
  for (p=0; p<push_iterations; p++) {
    /* Evict a key at random */
    ei = __arc_random(&rctx, NHASH);
    eh = hashes[ei];
    ek = getkey(c, tbl, eh);
    ev = getval(c, tbl, eh);
    setkey(c, tbl, eh, k);
    setval(c, tbl, eh, v);

    /* See if the evicted key hashes to an empty bucket */
    for (i=0; i<NHASH; i++) {
      hashes[i] = __arc_hash(c, ek, hashseeds[i]);
      nk = getkey(c, tbl, hashes[i]);
      if (nk == CUNBOUND) {
	setkey(c, tbl, hashes[i], ek);
	setval(c, tbl, hashes[i], ev);
	/* If the table usage exceeds threshold, resize the table */
	if (USAGE(tbl, ) >= THRESHOLD(tbl))
	  resize(c, tbl);
	return;
      }
    }
    k = ek;
    v = ev;
  }
  /* We have pushed stuff around as much as we could but failed to
     find a place to put the current evicted key. Resize the table,
     and then put this last evicted key in. This should definitely
     succeed at this point, without any further contortions. */
  /* Reduce the usage since the evicted key is removed until it is
     re-added below. */
  USAGE(tbl, --);
  resize(c, tbl);
  __arc_tbl_insert(c, tbl, k, v);
 }

static void resize(arc *c, value tbl)
{
  hashtbl *ht = (hashtbl *)tbl;
  int size, oldsize, i;
  value oldk, oldv, k, v;

  oldsize = HASHSIZE(ht->nbits);
  oldk = ht->k;
  oldv = ht->v;
  ht->nbits++;
  ht->stashsize = stashsize(ht->nbits);
  size = HASHSIZE(ht->nbits);
  ht->threshold = (size * MAXLOAD)/100;
  ht->k = arc_vector_new(c, size + ht->stashsize);
  ht->v = arc_vector_new(c, size + ht->stashsize);
  ht->usage = 0;
  for (i=0; i<size; i++) {
    SVIDX(c, ht->k, i, CUNBOUND);
    SVIDX(c, ht->v, i, CUNBOUND);
  }
  /* Move the old keys and values to the new hash */
  for (i=0; i<oldsize; i++) {
    k = VIDX(oldk, i);
    if (k == CUNBOUND)
      continue;
    v = VIDX(oldv, i);
    __arc_tbl_insert(c, tbl, k, v);
  }
}

value __arc_tbl_lookup_full(arc *c, value tbl, value k,
			    uint64_t (*hash)(arc *, value, uint64_t),
			    int (*is)(arc *, value, value))
{
  int i;
  uint64_t hashv;

  for (i=0; i<NHASH; i++) {
    hashv = hash(c, k, hashseeds[i]);
    if (is(c, getkey(c, tbl, hashv), k))
      return(getval(c, tbl, hashv));
  }
  return(CUNBOUND);
}

value __arc_tbl_lookup(arc *c, value tbl, value k)
{
  return(__arc_tbl_lookup_full(c, tbl, k, __arc_hash, __arc_is));
}

static uint64_t sshash(arc *c, value s, uint64_t seed)
{
  const char *ss = (char *)s;
  return(__arc_utfstrhash(c, ss, seed));
}

static int ssis(arc *c, value v1, value v2)
{
  if (arc_type(v1) != &__arc_string_t)
    return(0);
  return(arc_is_str_cstr(c, (char *)v2, v1));
}

value __arc_tbl_lookup_cstr(arc *c, value tbl, const char *key)
{
  return(__arc_tbl_lookup_full(c, tbl, (value)key, sshash, ssis));
}

value __arc_tbl_delete(arc *c, value tbl, value k)
{
  int i;
  uint64_t hash;
  value oldv;

  for (i=0; i<NHASH; i++) {
    hash = __arc_hash(c, k, hashseeds[i]);
    if (__arc_is(c, getkey(c, tbl, hash), k)) {
      oldv = getval(c, tbl, hash);
      setkey(c, tbl, hash, CUNBOUND);
      setval(c, tbl, hash, CUNBOUND);
      USAGE(tbl, --);
      return(oldv);
    }
  }
  return(CUNBOUND);  
}
