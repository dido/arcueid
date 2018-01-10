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

/* The hash function here is a hash based on the Murmur3 hash function.
   Arcueid always hashes blocks of data in 64-bit chunks, even on 32-bit
   systems using the 32-bit hash. */

#if SIZEOF_LONG >= 8

/*! \def HASHSEED
    \brief Seed for the hash function
    Default seed is the first 32/64 bits of the Euler-Mascheroni constant.
 */
#define HASHSEED 0x93c467e37db0c7a4ULL

#define HASH64BITS

/*! \def HASHSEED
    \brief Seed for the hash function
    Default seed is the first 64 bits of the Euler-Mascheroni constant.
 */
#define HASHSEED 0x93c467e37db0c7a4ULL

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

#else

#define HASH32BITS
#define HASHSEED 0x93c467e3
#define C1 0xcc9e2d51
#define C2 0x1b873593

static inline uint32_t rotl32(uint32_t x, int8_t r)
{
  return((x << r) | (x >> (32 - r)));
}
			 
void __arc_hash_init(struct hash_ctx *ctx)
{
  ctx->h1 = HASHSEED;
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
