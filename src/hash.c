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

/* Arcueid's hash function is a Cuckoo hash with by default three
   hashes  (which are implemented by using three different seeds for
   MurmurHash3). Rehashing occurs whenever a hash insertion fails. */

/*! \def NHASH
    \brief The number of hashes used
 */
#define NHASH 3

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

typedef struct {
  int nbits;			/*!< number of hash bits  */
  value k;			/*!< Table of keys */
  value v;			/*!< Table of values */
} hashtbl;

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

arctype __arc_tbl_t = { NULL, tmark, NULL, NULL, NULL, sizeof(hashtbl) };

#define HASHSIZE(n) ((unsigned long)1 << (n))
#define HASHMASK(tbl) (HASHSIZE(((hashtbl *)tbl)->nbits))
#define HIDX(tbl, kv, hash) (VIDX(((hashtbl *)tbl)->kv, (hash) & (HASHMASK(tbl))))
#define SHIDX(c, tbl, kv, hash, x) (SVIDX((c), ((hashtbl *)tbl)->kv, (hash) & (HASHMASK(tbl)), (x)))
/* XXX - this needs to be better defined */
#define MAX_PUSH(tbl) (8)

static void push_keys(arc *c, value tbl, value k, value v,
		      uint64_t *hashes);
static void resize(arc *c, value tbl);

value arc_tbl_new(arc *c, int nbits)
{
  value vht = arc_new(c, &__arc_tbl_t, sizeof(hashtbl));
  hashtbl *ht = (hashtbl *)vht;
  int size, i;
  
  ht->nbits = nbits;
  size = HASHSIZE(nbits);
  ht->k = arc_vector_new(c, size);
  ht->v = arc_vector_new(c, size);
  for (i=0; i<size; i++) {
    SVIDX(c, ht->k, i, CUNBOUND);
    SVIDX(c, ht->v, i, CUNBOUND);
  }
  return(vht);
}

value __arc_tbl_insert(arc *c, value tbl, const value k, const value v)
{
  uint64_t hashes[NHASH];
  value key;
  int i;

  /* Check for existing keys */
  for (i=0; i<NHASH; i++) {
    hashes[i] = __arc_hash(c, k, hashseeds[i]);
    if (__arc_is(c, HIDX(tbl, k, hashes[i]), k)) {
      SHIDX(c, tbl, v, hashes[i], v);
      return(v);
    }
  }

  /* Key is not there yet. Check for empty buckets. Put the key in an empty
     bucket if possible. */
  for (i=0; i<NHASH; i++) {
    key = HIDX(tbl, k, hashes[i]);
    if (key == CUNBOUND) {
      SHIDX(c, tbl, k, hashes[i], k);
      SHIDX(c, tbl, v, hashes[i], v);
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

  /* Use first hash as seed for the PRNG */
  __arc_srand(&rctx, hashes[0]);
  for (p=0; p<MAX_PUSH(tbl); p++) {
    /* Evict a key at random */
    ei = __arc_random(&rctx, NHASH);
    eh = hashes[ei];
    ek = HIDX(tbl, k, eh);
    ev = HIDX(tbl, v, eh);
    SHIDX(c, tbl, k, eh, k);
    SHIDX(c, tbl, v, eh, v);

    /* See if the evicted key hashes to an empty bucket */
    for (i=0; i<NHASH; i++) {
      hashes[i] = __arc_hash(c, ek, hashseeds[i]);
      nk = HIDX(tbl, k, hashes[i]);
      if (nk == CUNBOUND) {
	SHIDX(c, tbl, k, hashes[i], ek);
	SHIDX(c, tbl, v, hashes[i], ev);
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
  resize(c, tbl);
  __arc_tbl_insert(c, tbl, ek, ev);
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
  size = HASHSIZE(ht->nbits);
  ht->k = arc_vector_new(c, size);
  ht->v = arc_vector_new(c, size);
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
