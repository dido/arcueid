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

unsigned long arc_hash_increment(arc *c, value v, arc_hs *s)
{
  unsigned long length=1;

  arc_hash_update(s, TYPE(v));
  switch (TYPE(v)) {
  case T_NIL:
    break;
  case T_TRUE:
    break;
  case T_FIXNUM:
    arc_hash_update(s, (unsigned long)FIX2INT(v));
    break;
  case T_NONE:
    /* XXX - this is an error! */
    break;
  case T_SYMBOL:
    /* XXX - to be implemented */
    /*    arc_hash_update(s, (unsigned long)SYM2ID(v)); */
  default:
    length = ((struct cell *)v)->hash(c, v, s);
    break;
  }
  return(length);
}

unsigned long arc_hash(arc *c, value v)
{
  unsigned long len;
  arc_hs s;

  arc_hash_init(&s, 0);
  len = arc_hash_increment(c, v, &s);
  return(arc_hash_final(&s, len));
}
