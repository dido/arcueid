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

#if 0
static uint64_t hashseeds[NHASH] = {
  0x93c467e37db0c7a4ULL,	/* Euler-Mascheroni constant */
  0xb7e151628aed2a6aULL,	/* fractional digits of e */
  0x9e3779b9f4a7c15fULL		/* Golden ratio */
  /* more seeds need to be defined if more hashes are wanted. */
};
#endif

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

typedef struct {
  int nbits;			/*!< number of hash bits  */
  value tables[NHASH];		/*!< hash tables */
} hashtbl;

static void tmark(arc *c, value v,
		  void (*marker)(struct arc *, value, int),
		  int depth)
{
  hashtbl *ht = (hashtbl *)v;
  int i;

  for (i=0; i<NHASH; i++)
    marker(c, ht->tables[i], depth);
}

arctype __arc_tbl_t = { NULL, tmark, NULL, sizeof(hashtbl) };

#define HASHSIZE(n) ((unsigned long)1 << (n))
#define HASHMASK(n) (HASHSIZE(n)-1)
#define HASHIDX(tbl, tnum, hs) (VIDX(((hashtbl *)(tbl))->tables[tnum], (((hs)[(tnum)]) & HASHMASK(((hashtbl *)(tbl))->nbits))))
#define SHASHIDX(c, tbl, tnum, hs, x) (SVIDX((c), ((hashtbl *)(tbl))->tables[tnum], (((hs)[(tnum)]) & HASHMASK(((hashtbl *)tbl)->nbits)), (x)))

value arc_tbl_new(arc *c, int nbits)
{
  value vht = arc_new(c, &__arc_tbl_t, sizeof(hashtbl));
  hashtbl *ht = (hashtbl *)vht;
  int i, size;

  ht->nbits = nbits;
  size = HASHSIZE(nbits);
  for (i=0; i<NHASH; i++)
    ht->tables[i] = arc_vector_new(c, size);
  return(vht);
}

typedef struct {
  uint64_t h[NHASH];		/*!< Hashes of the key  */
  value k;			/*!< The key itself  */
  value v;			/*!< The associated value  */
} hashbucket;

static void bmark(arc *c, value v,
		  void (*marker)(struct arc *, value, int),
		  int depth)
{
  hashbucket *hb = (hashbucket *)v;
  marker(c, hb->k, depth);
  marker(c, hb->v, depth);
}

arctype __arc_hb_t = { NULL, bmark, NULL, sizeof(hashbucket) };

static value bucket_new(arc *c, value key, value val, const uint64_t *hashes)
{
  value bv = arc_new(c, &__arc_hb_t, sizeof(hashbucket));
  hashbucket *hb = (hashbucket *)bv;
  int i;

  for (i=0; i<NHASH; i++)
    hb->h[i] = hashes[i];
  hb->k = key;
  hb->v = val;
  return(bv);
}

/* Compare two sets of hashes */
static int hcomp(const uint64_t *h1, const uint64_t *h2)
{
  int i;

  for (i=0; i<NHASH; i++) {
    if (h1[i] != h2[i])
      return(0);
  }
  return(1);
}

/* Look up the key in tbl. Returns -1 if the key was not found.
   Otherwise, returns the subtable number where it was found. The
   value mapping may be returned by using the subtable number in
   the HASHIDX macro. Uses the hashes to test if keys are equal. */
int __arc_hash_primitive_lookup(arc *c, const value tbl, const value key,
				const uint64_t *hashes)
{
  hashbucket *hb;
  value b;
  int i;

  /* Check if the value exists in any of the tables */
  for (i=0; i<NHASH; i++)  {
    b = HASHIDX(tbl, i, hashes);
    /* Empty bucket, look in another table. */
    if (NILP(b))
      continue;
    /* see if it is our value */
    hb = (hashbucket *)b;
    /* Check all the hashes. It is the same object if and only if ALL
       hashes match */
    if (hcomp(hashes, hb->h))
      return(i);
  }
  return(-1);
}

/* Try to move bucket hb to one of its NHASH alternate locations in
   ht. Return 1 on success, 0 on failure. */
static int try_move_bucket(arc *c, value ht, const hashbucket *hb)
{
  int i;
  value b;

  for (i=0; i<NHASH; i++) {
    b = HASHIDX(ht, i, hb->h);
    if (NILP(b)) {
      SHASHIDX(c, ht, i, hb->h, (value)hb);
      return(1);
    }
  }
  return(0);
}

/* Try to insert key-val mapping into tbl. Returns 0 if it failed to
   insert the mapping. Creates a bucket if necessary. */
static int try_hash_insert(arc *c, value tbl, const value key,
			   const value val,
			   const uint64_t *hashes)
{
  int i, r;
  value b;
  hashbucket *hb;
  
  r = __arc_hash_primitive_lookup(c, tbl, key, hashes);
  if (r > 0) {
    /* Object is already present in the table. Update the value in the
       bucket with val */
    b = HASHIDX(tbl, r, hashes);
    hb = (hashbucket *)b;
    arc_wb(c, hb->v, val);
    hb->v = val;
    return(1);
  }

  /* Object is not already present in any table. Try to find a place
     to put it. */
  for (i=0; i<NHASH; i++) {
    b = HASHIDX(tbl, i, hashes);
    if (NILP(b)) {
      r = i;
      break;
    }
  }

  /* We have found or created a subtable that has an empty slot
     corresponding to one of the hashes. Put the value there. */
  do {
    if (r > 0) {
      b = bucket_new(c, key, val, hashes);
      SHASHIDX(c, tbl, r, hashes, b);
      return(1);
    }

    /* No obvious place to put it. See if we can move one of the buckets
       already in place to one of the other alternative places where it
       can go. */
    for (i=0; i<NHASH; i++) {
      /* Get a bucket already occupying one of the subtables and try
	 to move it to an alternate location. */
      b = HASHIDX(tbl, i, hashes);
      hb = (hashbucket *)b;
      if (try_move_bucket(c, tbl, hb)) {
	r = i;
	break;
      }
    }
  } while (r > 0);
  /* None of our attempts to insert the new object were
     successful. This probably means that we ought to resize
     the tables. */
  return(0);
}

/* Resize the hash tables */
static void hash_resize(arc *c, value tbl)
{
  value newsubtables[NHASH];
  hashtbl *ht = (hashtbl *)tbl;
  int newsize, i, j, k;
  value b;
  hashbucket *hb;

  newsize = HASHSIZE(ht->nbits+1);
  for (i=0; i<NHASH; i++)
    newsubtables[i] = arc_vector_new(c, newsize);
  /* Look for all of the old buckets in the current hash table and put
     them into the new subtables */
  for (i=0; i<NHASH; i++) {
    for (j=0; j<HASHSIZE(ht->nbits); j++) {
      b = VIDX(ht->tables[i], j);
      if (NILP(b))
	continue;
      hb = (hashbucket *)b;
      /* Find a new place for this bucket in the new subtables */
      for (k=0; k<NHASH; k++) {
	if (NILP(VIDX(newsubtables[k], hb->h[k] & HASHMASK(ht->nbits+1))))
	  SVIDX(c, newsubtables[k], hb->h[k] & HASHMASK(ht->nbits+1), b);
      }
    }
  }
  ht->nbits++;
  for (i=0; i<NHASH;i++) {
    arc_wb(c, ht->tables[i], newsubtables[i]);
    ht->tables[i] = newsubtables[i];
  }
}

value __arc_primitive_hash_insert(arc *c, value tbl, const value key,
				  const value val, const uint64_t *hashes)
{
  while (!try_hash_insert(c, tbl, key, val, hashes))
    hash_resize(c, tbl);
  return(val);
}
