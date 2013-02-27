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

unsigned long arc_hash_increment(arc *c, value v, arc_hs *s, value visithash)
{
  unsigned long length=1;
  typefn_t *tfn;

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
    tfn = __arc_typefn(c, v);
    length = tfn->hash(c, v, s, visithash);
    break;
  }
  return(length);
}

unsigned long arc_hash_level(arc *c, value v, value visithash,
			     unsigned long level)
{
  unsigned long len;
  arc_hs s;

  arc_hash_init(&s, level);
  len = arc_hash_increment(c, v, &s, visithash);
  return(arc_hash_final(&s, len));
}

unsigned long arc_hash(arc *c, value v, value visithash)
{
  return(arc_hash_level(c, v, visithash, 0));
}

/* Arcueid's hash table data type.  A hash table is simply a four-tuple,
   with the elements as follows:

   0 - The actual table itself (a vector)
   1 - Number of hash bits (a fixnum)
   2 - Number of entries total (a fixnum)
   3 - Load limit (a fixnum)

   Hash buckets are triples with the elements as follows:

   0 - The index of the element in the current hash table (fixnum)
   1 - The key of this element
   2 - The value of this element
   3 - The table to which this element belongs
*/

#define HASH_SIZE (4)
#define HASH_TABLE(t) (REP(t)[0])
#define HASH_INDEX(t, i) (VINDEX(HASH_TABLE(t), (i)))
#define HASH_BITS(t) (FIX2INT(REP(t)[1]))
#define HASH_NENTRIES(t) (FIX2INT(REP(t)[2]))
#define HASH_LLIMIT(t) (FIX2INT(REP(t)[3]))
#define SET_HASHBITS(t, n) (REP(t)[1] = INT2FIX(n))
#define SET_NENTRIES(t, n) (REP(t)[2] = INT2FIX(n))
#define SET_LLIMIT(t, n) (REP(t)[3] = INT2FIX(n))

#define BUCKET_SIZE (4)
#define BINDEX(t) (FIX2INT(REP(t)[0]))
#define SBINDEX(t, idx) (REP(t)[0] = INT2FIX(idx))
#define BKEY(t) (REP(t)[1])
#define BVALUE(t) (REP(t)[2])
#define BTABLE(t) (REP(t)[3])

#define HASHSIZE(n) ((unsigned long)1 << (n))
#define HASHMASK(n) (HASHSIZE(n)-1)
#define MAX_LOAD_FACTOR 70	/* percentage */
/* linear probing */
#define PROBE(i, k) (i)

#define TABLESIZE(t) (HASHSIZE(HASH_BITS(t)))
#define TABLEMASK(t) (HASHMASK(HASH_BITS(t)))

/* An empty slot is either CUNBOUND or CUNDEF.  CUNDEF is used as a
   'tombstone' value for deleted elements.  If this is found, one may have
   to keep probing until either the actual element is found or one runs into
   a CUNBOUND, meaning the element is definitely not in the table.
   Since we enforce load factor, there will definitely be some table
   elements which remain unused. */
#define EMPTYP(x) (((x) == CUNBOUND) || ((x) == CUNDEF))

static value hash_pprint(arc *c, value sexpr, value *ppstr, value visithash)
{
  /* XXX - fill this in */
  return(CNIL);
}

static void hash_marker(arc *c, value v, int depth,
			void (*markfn)(arc *, value, int))
{
  /* Just mark the table--that should be enough. It's just a vector. */
  markfn(c, HASH_TABLE(v), depth);
}

static void hash_sweeper(arc *c, value v)
{
  int i;
  value tbl = HASH_TABLE(v);

  /* Clear the BTABLE links for any active buckets within this hash
     before fully sweeping it away. */
  for (i=0; i<VECLEN(tbl); i++) {
    if (EMPTYP(VINDEX(tbl, i)))
      continue;
    BTABLE(VINDEX(tbl, i)) = CNIL;
  }
}

static unsigned long hash_hasher(arc *c, value v, arc_hs *s, value visithash)
{
  unsigned long len;
  value tbl, e;
  int i;

  if (visithash == CNIL)
    visithash = arc_mkhash(c, ARC_HASHBITS);
  if (__arc_visit(c, v, visithash) != CNIL) {
    /* Already visited at some point.  Do not recurse further.  An already
       visited node will still contribute 1 to the length though. */
    return(1);
  }
  tbl = HASH_TABLE(v);
  len = 0;
  for (i=0; i<VECLEN(tbl); i++) {
    e = VINDEX(tbl, i);
    if (EMPTYP(e))
      continue;
    len += arc_hash(c, BKEY(e), visithash);
    len += arc_hash(c, BVALUE(e), visithash);
  }
  return(len);
}

static value hash_isocmp(arc *c, value v1, value v2, value vh1, value vh2)
{
  value vhh1, vhh2;
  value v2val, tbl, e;
  int i;

  if (vh1 == CNIL) {
    vh1 = arc_mkhash(c, ARC_HASHBITS);
    vh2 = arc_mkhash(c, ARC_HASHBITS);
  }

  if ((vhh1 = __arc_visit(c, v1, vh1)) != CNIL) {
    /* If we find a visited object, see if v2 is also visited in vh2.
       If not, they are not the same. */
    vhh2 = __arc_visit(c, v2, vh2);
    /* We see if the same value was produced on visiting. */
    return((vhh2 == vhh1) ? CTRUE : CNIL);
  }

  /* Get value assigned by __arc_visit to v1. */
  vhh1 = __arc_visit(c, v1, vh1);
  /* If we somehow already visited v2 when v1 was not visited in the
     same way, they cannot be the same. */
  if (__arc_visit2(c, v2, vh2, vhh1) != CNIL)
    return(CNIL);

  /* Two hash tables must have identical numbers of entries to be isomorphic */
  if (HASH_NENTRIES(v1) != HASH_NENTRIES(v2))
    return(CNIL);
  /* Two hash tables must have identical key-value pair mappings to be
     isomorphic */
  tbl = HASH_TABLE(v1);
  for (i=0; i<VECLEN(tbl); i++) {
    e = VINDEX(tbl, i);
    if (EMPTYP(e))
      continue;
    v2val = arc_hash_lookup(c, v2, BKEY(e));
    if (arc_iso(c, BVALUE(e), v2val, vh1, vh2) == CNIL)
      return(CNIL);
  }
  return(CTRUE);
}

/* A hash can be applied with an index and an optional default value */
static int hash_apply(arc *c, value thr, value tbl)
{
  value key, dflt = CNIL, val;

  if (arc_thr_argc(c, thr) == 2) {
    dflt = arc_thr_pop(c, thr);
  } else if (arc_thr_argc(c, thr) != 1) {
    arc_err_cstrfmt(c, "application of a table expects 1 or 2 arguments, given %d",
		    arc_thr_argc(c, thr));
    return(APP_OK);
  }
  key = arc_thr_pop(c, thr);
  val = arc_hash_lookup(c, tbl, key);
  val = (NIL_P(val)) ? dflt : val;
  arc_thr_set_valr(c, thr, val);
  return(APP_OK);
}

value arc_mkhash(arc *c, int hashbits)
{
  value hash;
  int i;

  hash = arc_mkobject(c, sizeof(value)*HASH_SIZE, T_TABLE);
  SET_HASHBITS(hash, hashbits);
  SET_NENTRIES(hash, 0);
  SET_LLIMIT(hash, (HASHSIZE(hashbits)*MAX_LOAD_FACTOR) / 100);
  HASH_TABLE(hash) = arc_mkvector(c, HASHSIZE(hashbits));
  for (i=0; i<HASHSIZE(hashbits); i++)
    VINDEX(HASH_TABLE(hash), i) = CUNBOUND;
  return(hash);
}

static void hashtable_expand(arc *c, value hash)
{
  unsigned int hv, index, i, j, nhashbits;
  value oldtbl, newtbl, e;

  nhashbits = HASH_BITS(hash) + 1;
  newtbl = arc_mkvector(c, HASHSIZE(nhashbits));
  for (i=0; i<HASHSIZE(nhashbits); i++)
    VINDEX(newtbl, i) = CUNBOUND;
  oldtbl = HASH_TABLE(hash);
  /* Search for active keys and copy them into the new table */
  for (i=0; i<VECLEN(oldtbl); i++) {
    e = VINDEX(oldtbl, i);
    if (EMPTYP(e))
      continue;
    /* insert the old key into the new table */
    hv = arc_hash(c, BKEY(e), CNIL);
    index = hv & HASHMASK(nhashbits);
    for (j=0; !EMPTYP(VINDEX(newtbl, index)); j++)
      index = (index + PROBE(j, k)) & HASHMASK(nhashbits);
    VINDEX(newtbl, index) = e;
    SBINDEX(e, index);		/* change index */
    VINDEX(oldtbl, i) = CUNBOUND;
  }
  SET_HASHBITS(hash, nhashbits);
  SET_LLIMIT(hash, (HASHSIZE(nhashbits)*MAX_LOAD_FACTOR) / 100);
  HASH_TABLE(hash) = newtbl;
}

static void hb_marker(arc *c, value v, int depth,
		      void (*markfn)(arc *, value, int))
{
  /* Mark the key/value pair */
  markfn(c, BKEY(v), depth);
  markfn(c, BVALUE(v), depth);
}

static void hb_sweeper(arc *c, value v)
{
  /* If the table has not been collected yet, clear
     its entry in the parent table, setting it to undef. */
  if (BTABLE(v) != CNIL) {
    value t = BTABLE(v);

    VINDEX(HASH_TABLE(t), BINDEX(v)) = CUNDEF;
    SET_NENTRIES(t, HASH_NENTRIES(t)-1);
  }
}

/* Create a hash bucket.  Hash buckets are objects that should never be
   directly visble, just as the tombstone values CUNDEF and CUNBOUND should
   never be seen directly by the interpreter. */
static value mkhashbucket(arc *c, value key, value val, int index, value table)
{
  value bucket;

  bucket = arc_mkobject(c, BUCKET_SIZE*sizeof(value), T_TBUCKET);
  BKEY(bucket) = key;
  BVALUE(bucket) = val;
  SBINDEX(bucket, index);
  BTABLE(bucket) = table;
  return(bucket);
}

value arc_hash_insert(arc *c, value hash, value key, value val)
{
  unsigned int hv, index, i;
  value e;

  if (HASH_NENTRIES(hash)+1 > HASH_LLIMIT(hash))
    hashtable_expand(c, hash);
  SET_NENTRIES(hash, HASH_NENTRIES(hash)+1);
  hv = arc_hash(c, key, CNIL);
  index = hv & TABLEMASK(hash);
  /* Collision resolution with open addressing */
  for (i=0;; i++) {
    e = VINDEX(HASH_TABLE(hash), index);
    /* If we see an empty bucket in our search, or if we see a bucket
       whose key is the same as the key specified, we have found the
       place where the element should go. */
    if (EMPTYP(e) || arc_is(c, BKEY(e), key) == CTRUE)
      break;
    /* We found a bucket, but it is occupied by some other key. Continue
       probing. */
    index = (index + PROBE(i, k)) & TABLEMASK(hash);
  }

  if (EMPTYP(e)) {
    /* No such key in the hash table yet.  Create a bucket and
       assign it to the table. */
    e = mkhashbucket(c, key, val, index, hash);
    VINDEX(HASH_TABLE(hash), index) = e;
  } else {
    /* The key already exists.  Use the current bucket but change the
       value to the value specified. */
    BVALUE(e) = val;
  }
  return(val);
}

static value hash_lookup(arc *c, value hash, value key, unsigned int *index)
{
  unsigned int hv, i;
  value e;

  hv = arc_hash(c, key, CNIL);
  *index = hv & TABLEMASK(hash);
  for (i=0;; i++) {
    *index = (*index + PROBE(i, key)) & TABLEMASK(hash);
    e = HASH_INDEX(hash, *index);
    /* CUNBOUND means there was never any element at that index, so we
       can stop. */
    if (e == CUNBOUND)
      return(CUNBOUND);
    /* CUNDEF means that there was an element at that index, but it was
       deleted at some point, so we may need to continue probing. */
    if (e == CUNDEF)
      continue;
    if (arc_iso(c, BKEY(e), key, CNIL, CNIL) == CTRUE)
      return(BVALUE(e));
  }
}

value arc_hash_lookup(arc *c, value tbl, value key)
{
  unsigned int index;

  return(hash_lookup(c, tbl, key, &index));
}

/* Slightly different version which returns the actual hash bucket
   with the key and value if a binding is available. */
value arc_hash_lookup2(arc *c, value hash, value key)
{
  unsigned int index;
  value val;

  val = hash_lookup(c, hash, key, &index);
  if (val == CUNBOUND)
    return(CUNBOUND);
  return(HASH_INDEX(hash, index));
}

value arc_hash_delete(arc *c, value hash, value key)
{
  unsigned int index;
  value v, e;

  v = hash_lookup(c, hash, key, &index);
  if (v != CUNBOUND) {
    e = VINDEX(HASH_TABLE(hash), index);
    BTABLE(e) = CNIL;
    VINDEX(HASH_TABLE(hash), index) = CUNDEF;
    SET_NENTRIES(hash, HASH_NENTRIES(hash)-1);
  }
  return(v);
}

/* This is the only difference between a weak table and a normal
   hash table.  A weak table will only mark the table vector itself,
   and will NOT propagate the mark to any of the table buckets. */
static void wtable_marker(arc *c, value v, int depth,
			  void (*markfn)(arc *, value, int))
{
  value tbl = HASH_TABLE(v);

  MARK(tbl);
}

/* Make a weak table */
value arc_mkwtable(arc *c, int hashbits)
{
  value tbl = arc_mkhash(c, hashbits);

  ((struct cell *)tbl)->_type = T_WTABLE;
  return(tbl);
}

/* Type function tables */

typefn_t __arc_table_typefn__ = {
  hash_marker,
  hash_sweeper,
  hash_pprint,
  hash_hasher,
  NULL,
  hash_isocmp,
  hash_apply
};

typefn_t __arc_hb_typefn__ = {
  hb_marker,
  hb_sweeper,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

typefn_t __arc_wtable_typefn__ = {
  wtable_marker,
  hash_sweeper,
  hash_pprint,
  hash_hasher,
  NULL,
  hash_isocmp,
  hash_apply
};
