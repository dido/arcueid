/* 
  Copyright (C) 2009 Rafael R. Sevilla

  This file is part of CArc

  CArc is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA
  02110-1301 USA.
*/
#include <inttypes.h>
#include "carc.h"
#include "alloc.h"
#include "../config.h"

#if SIZEOF_LONG >= 8

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

unsigned long carc_hash(carc *c, value v)
{
  unsigned long len;
  carc_hs s;

  carc_hash_init(&s, 0);
  len = hash_increment(c, v, &s);
  return(carc_hash_final(&s, len));
}
