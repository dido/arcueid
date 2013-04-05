/* 
  Copyright (C) 2013 Rafael R. Sevilla

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

  NOTE: Non-atom keys will not work with the current set of hash table
  support functions. There will be another set of hash table functions
  that will not have this limitation.
*/
#include "arcueid.h"
#include "alloc.h"
#include "arith.h"
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

#ifdef BAD_TESTING_HASH
/* This causes everything to hash to zero. This has the result of
   turning the open addressing hash table into a table where one
   has to do a linear search of all items.  Used for test purposes
   only! */

unsigned long arc_hash_final(arc_hs *s, unsigned long len)
{
  return(0L);
}

#else

unsigned long arc_hash_final(arc_hs *s, unsigned long len)
{
  s->s[2] += len << 3;
  FINAL(s->s[0], s->s[1], s->s[2]);
  return(s->s[2]);
}

#endif

unsigned long arc_hash_increment(arc *c, value v, arc_hs *s)
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
    arc_hash_update(s, (unsigned long)SYM2ID(v));
    break;
  default:
    tfn = __arc_typefn(c, v);
    if (tfn->hash == NULL) {
      arc_err_cstrfmt(c, "no type-specific hasher found for type %d", TYPE(v));
      return(length);
    }
    length = tfn->hash(c, v, s);
    break;
  }
  return(length);
}

unsigned long arc_hash_level(arc *c, value v, unsigned long level)
{
  unsigned long len;
  arc_hs s;

  arc_hash_init(&s, level);
  len = arc_hash_increment(c, v, &s);
  return(arc_hash_final(&s, len));
}

unsigned long arc_hash(arc *c, value v)
{
  return(arc_hash_level(c, v, 0));
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
   4 - The original hash value computed for this element
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

#define BUCKET_SIZE (5)
#define BINDEX(t) (FIX2INT(REP(t)[0]))
#define SBINDEX(t, idx) (REP(t)[0] = INT2FIX(idx))
#define BKEY(t) (REP(t)[1])
#define BVALUE(t) (REP(t)[2])
#define BTABLE(t) (REP(t)[3])
#define BHASHVAL(t) (REP(t)[4])

#define HASHSIZE(n) ((unsigned long)1 << (n))
#define HASHMASK(n) (HASHSIZE(n)-1)
#define MAX_LOAD_FACTOR 70	/* percentage */
/* linear probing */
#define PROBE(i) (i)

#define TABLESIZE(t) (HASHSIZE(HASH_BITS(t)))
#define TABLEMASK(t) (HASHMASK(HASH_BITS(t)))

/* An empty slot is either CUNBOUND or CUNDEF.  CUNDEF is used as a
   'tombstone' value for deleted elements.  If this is found, one may have
   to keep probing until either the actual element is found or one runs into
   a CUNBOUND, meaning the element is definitely not in the table.
   Since we enforce load factor, there will definitely be some table
   elements which remain unused. */
#define EMPTYP(x) (((x) == CUNBOUND) || ((x) == CUNDEF))

#if 0
static value hash_pprint(arc *c, value sexpr, value *ppstr, value visithash)
{
  /* XXX - fill this in */
  return(CNIL);
}
#endif

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

static AFFDEF(hash_isocmp)
{
  AARG(v1, v2, vh1, vh2);
  AVAR(iso2, tbl, e, v2val, i);
  value vhh1, vhh2;		/* not required after calls */
  AFBEGIN;

  if ((vhh1 = __arc_visit(c, AV(v1), AV(vh1))) != CNIL) {
    /* If we find a visited object, see if v2 is also visited in vh2.
       If not, they are not the same. */
    vhh2 = __arc_visit(c, AV(v2), AV(vh2));
    /* We see if the same value was produced on visiting. */
    ARETURN((vhh2 == vhh1) ? CTRUE : CNIL);
  }

  /* Get value assigned by __arc_visit to v1. */
  vhh1 = __arc_visit(c, AV(v1), AV(vh1));
  /* If we somehow already visited v2 when v1 was not visited in the
     same way, they cannot be the same. */
  if (__arc_visit2(c, AV(v2), AV(vh2), AV(vhh1)) != CNIL)
    ARETURN(CNIL);

  /* Two hash tables must have identical numbers of entries to be isomorphic */
  if (HASH_NENTRIES(AV(v1)) != HASH_NENTRIES(AV(v2)))
    ARETURN(CNIL);
  AV(tbl) = HASH_TABLE(AV(v1));
  AV(iso2) = arc_mkaff(c, arc_iso2, CNIL);
  for (AV(i) = INT2FIX(0); FIX2INT(AV(i))<VECLEN(AV(tbl));
       AV(i) = INT2FIX(FIX2INT(AV(i)) + 1)) {
    AV(e) = VINDEX(AV(tbl), FIX2INT(AV(i)));
    if (EMPTYP(AV(e)))
      continue;
    AV(v2val) = arc_hash_lookup(c, AV(v2), BKEY(AV(e)));
    AFCALL(AV(iso2), BVALUE(AV(e)), AV(v2val), AV(vh1), AV(vh2));
    if (NIL_P(AFCRV))
      ARETURN(CNIL);
  }
  ARETURN(CTRUE);
  AFEND;
}
AFFEND

/* A hash can be applied with an index and an optional default value */
static int hash_apply(arc *c, value thr, value tbl)
{
  value key, dflt = CNIL, val;

  if (arc_thr_argc(c, thr) == 2) {
    dflt = arc_thr_pop(c, thr);
  } else if (arc_thr_argc(c, thr) != 1) {
    arc_err_cstrfmt(c, "application of a table expects 1 or 2 arguments, given %d",
		    arc_thr_argc(c, thr));
    return(TR_RC);
  }
  key = arc_thr_pop(c, thr);
  val = arc_hash_lookup(c, tbl, key);
  val = (NIL_P(val)) ? dflt : val;
  arc_thr_set_valr(c, thr, val);
  return(TR_RC);
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

value arc_newtable(arc *c)
{
  return(arc_mkhash(c, ARC_HASHBITS));
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
    hv = (unsigned int)FIX2INT(BHASHVAL(e));
    index = hv & HASHMASK(nhashbits);
    for (j=0; !EMPTYP(VINDEX(newtbl, index)); j++)
      index = (index + PROBE(j)) & HASHMASK(nhashbits);
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
static value mkhashbucket(arc *c, value key, value val, int index, value table,
			  value hashcode)
{
  value bucket;

  bucket = arc_mkobject(c, BUCKET_SIZE*sizeof(value), T_TBUCKET);
  BKEY(bucket) = key;
  BVALUE(bucket) = val;
  SBINDEX(bucket, index);
  BTABLE(bucket) = table;
  BHASHVAL(bucket) = hashcode;
  return(bucket);
}

static value hash_lookup(arc *c, value hash, value key, unsigned int *index)
{
  unsigned int hv, i;
  value e;

  hv = arc_hash(c, key);
  *index = hv & TABLEMASK(hash);
  for (i=0;; i++) {
    *index = (*index + PROBE(i)) & TABLEMASK(hash);
    e = HASH_INDEX(hash, *index);
    /* CUNBOUND means there was never any element at that index, so we
       can stop. */
    if (e == CUNBOUND)
      return(CUNBOUND);
    /* CUNDEF means that there was an element at that index, but it was
       deleted at some point, so we may need to continue probing. */
    if (e == CUNDEF)
      continue;
    if (arc_is2(c, BKEY(e), key) == CTRUE)
      return(BVALUE(e));
  }
  return(CUNBOUND);
}

/* These functions will only work for simple keys for which a basic hash
   is available.  They are a convenience because most hash tables are
   indexed by strings, symbols, numbers, and other simple objects.  In
   particular, symbol tables are indexed in that way. */

value arc_hash_insert(arc *c, value hash, value key, value val)
{
  unsigned int hv, index, i;
  value e;

  /* First of all, look for the key if a binding already exists for it */
  e = hash_lookup(c, hash, key, &index);
  if (BOUND_P(e)) {
    /* if we are already bound, overwrite the old value */
    e = VINDEX(HASH_TABLE(hash), index);
    BVALUE(e) = val;
    return(val);
  }
  /* Not yet bound.  Look for a slot where we can put it */
  if (HASH_NENTRIES(hash)+1 > HASH_LLIMIT(hash))
    hashtable_expand(c, hash);
  SET_NENTRIES(hash, HASH_NENTRIES(hash)+1);
  hv = arc_hash(c, key);
  index = hv & TABLEMASK(hash);
  for (i=0;; i++) {
    e = VINDEX(HASH_TABLE(hash), index);
    /* If we see an empty bucket in our search, or if we see a bucket
       whose key is the same as the key specified, we have found the
       place where the element should go. This second case should never
       happen, based on what we did above, but hey, belt and suspenders. */
    if (EMPTYP(e) || arc_is2(c, BKEY(e), key) == CTRUE)
      break;
    /* We found a bucket, but it is occupied by some other key. Continue
       probing. */
    index = (index + PROBE(i)) & TABLEMASK(hash);
  }

  if (EMPTYP(e)) {
    /* No such key in the hash table yet.  Create a bucket and
       assign it to the table. */
    e = mkhashbucket(c, key, val, index, hash, INT2FIX(hv));
    VINDEX(HASH_TABLE(hash), index) = e;
  } else {
    /* The key already exists.  Use the current bucket but change the
       value to the value specified. */
    BVALUE(e) = val;
  }
  return(val);
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

/* The following functions are more general, and will work for any kind
   of key for which an xhash function exists.
 */

/* An arc_hs is encoded into a vector of fixnums in the following way:

   HS[0] = low 16 or 32 bits of s[0];
   HS[1] = high 16 or 32 bits of s[0];
   HS[2] = low 16 or 32 bits of s[1];
   HS[3] = high 16 or 32 bits of s[1];
   HS[4] = low 16 or 32 bits of s[2];
   HS[5] = high 16 or 32 bits of s[2];
   HS[6] = hash state
*/

#ifdef HASH64BITS
#define BITSHIFT 32
#define MASK 0xffffffff
#else
#define BITSHIFT 16
#define MASK 0xffff
#endif

#define ENC_VEC_LEN 7

static value encode_hs(arc *c, value enc, arc_hs *hs)
{
  int i, j, shift;

  for (i=0; i<6; i++) {
    j = i/2;
    shift = (i%2 == 0) ? 0 : BITSHIFT;
    VINDEX(enc, i) = INT2FIX((hs->s[j] >> shift) & MASK);
  }
  VINDEX(enc, 6) = INT2FIX(hs->state);
  return(enc);
}

static arc_hs *decode_hs(arc *c, arc_hs *hs, value enc)
{
  int i;

  for (i=0; i<3; i++) {
    hs->s[i] = ((unsigned long)(FIX2INT(VINDEX(enc, i*2)) & MASK)) |
      (((unsigned long)((FIX2INT(VINDEX(enc, i*2+1))) & MASK)) << BITSHIFT);
  }
  hs->state = FIX2INT(VINDEX(enc, 6));
  return(hs);
}

AFFDEF(arc_xhash_increment)
{
  AARG(v, ehs);
  AOARG(visithash);
  AVAR(length);
  arc_hs hs;
  typefn_t *tfn;
  AFBEGIN;

  decode_hs(c, &hs, AV(ehs));
  AV(length) = INT2FIX(1);
  arc_hash_update(&hs, TYPE(AV(v)));
  if (TYPE(AV(v)) == T_FIXNUM)
    arc_hash_update(&hs, (unsigned long)FIX2INT(AV(v)));
  else if (TYPE(AV(v)) == T_SYMBOL)
    arc_hash_update(&hs, (unsigned long)SYM2ID(AV(v)));
  else if (AV(v) == CTRUE || NIL_P(AV(v))) {
    /* nothing to do here */
    ;
  } else {
    tfn = __arc_typefn(c, AV(v));
    if (tfn->hash != NULL)
      AV(length) = INT2FIX(tfn->hash(c, AV(v), &hs));
    else if (tfn->xhash != NULL) {
      encode_hs(c, AV(ehs), &hs);
      AFTCALL(arc_mkaff(c, tfn->xhash, CNIL), AV(v), AV(ehs), AV(length), AV(visithash));
    } else {
      arc_err_cstrfmt(c, "no type-specific hasher found for type %d", TYPE(v));
    }
  }
  encode_hs(c, AV(ehs), &hs);
  ARETURN(AV(length));
  AFEND;
}
AFFEND

AFFDEF(arc_xhash)
{
  AARG(v);
  AOARG(level);
  AVAR(ehs);
  arc_hs s;
  value len;
  unsigned long final;
  AFBEGIN;
  AV(ehs) = arc_mkvector(c, ENC_VEC_LEN);
  if (!BOUND_P(AV(level)))
    AV(level) = INT2FIX(0);
  arc_hash_init(&s, FIX2INT(AV(level)));
  encode_hs(c, AV(ehs), &s);
  AFCALL(arc_mkaff(c, arc_xhash_increment, CNIL), AV(v), AV(ehs));
  len = AFCRV;
  decode_hs(c, &s, AV(ehs));
  final = arc_hash_final(&s, FIX2INT(len));
  ARETURN(INT2FIX(final));
  AFEND;
}
AFFEND

/* Returns unbound or the hash bucket */
AFFDEF(xhash_lookup)
{
  AARG(hash, key);
  AVAR(e, index, i);
  unsigned int hv;
  AFBEGIN;
  AFCALL(arc_mkaff(c, arc_xhash, CNIL), AV(key));
  hv = FIX2INT(AFCRV);
  AV(index) = INT2FIX(hv & TABLEMASK(AV(hash)));
  for (AV(i)=INT2FIX(0);; AV(i) = INT2FIX(FIX2INT(AV(i)) + 1)) {
    AV(index) = INT2FIX((FIX2INT(AV(index)) + PROBE(FIX2INT(AV(i)))) & TABLEMASK(AV(hash)));
    AV(e) = HASH_INDEX(AV(hash), FIX2INT(AV(index)));
    /* CUNBOUND means there was never any element at that index, so we
       can stop. */
    if (AV(e) == CUNBOUND)
      ARETURN(CUNBOUND);
    /* CUNDEF means that there was an element at that index, but it was
       deleted at some point, so we may need to continue probing. */
    if (e == CUNDEF)
      continue;
    AFCALL(arc_mkaff(c, arc_iso, CNIL), BKEY(AV(e)), AV(key));
    if (AFCRV == CTRUE)
      ARETURN(cons(c, AV(index), AV(e)));
  }
  ARETURN(CUNBOUND);
  AFEND;
}
AFFEND

AFFDEF(arc_xhash_lookup2)
{
  AARG(tbl, key);
  AFBEGIN;
  AFCALL(arc_mkaff(c, xhash_lookup, CNIL), AV(tbl), AV(key));
  if (BOUND_P(AFCRV))
    ARETURN(cdr(AFCRV));
  ARETURN(CUNBOUND);
  AFEND;
}
AFFEND

AFFDEF(arc_xhash_insert)
{
  AARG(hash, key, val);
  AVAR(hv, index, i, e);
  AFBEGIN;

  /* First, look for the key if a binding already exists for it */
  AFCALL(arc_mkaff(c, arc_xhash_lookup2, CNIL), AV(hash), AV(key));
  if (BOUND_P(AFCRV)) {
    AV(e) = AFCRV;
    BVALUE(AV(e)) = AV(val);
    ARETURN(AV(val));
  }

  /* Not already bound, so we need to create a new binding */
  if (HASH_NENTRIES(AV(hash))+1 > HASH_LLIMIT(AV(hash)))
    hashtable_expand(c, hash);
  SET_NENTRIES(AV(hash), HASH_NENTRIES(AV(hash))+1);
  AFCALL(arc_mkaff(c, arc_xhash, CNIL), AV(key));
  AV(hv) = AFCRV;
  AV(index) = INT2FIX(FIX2INT(AV(hv)) & TABLEMASK(AV(hash)));
  for (AV(i)=INT2FIX(0);; AV(i) = INT2FIX(FIX2INT(AV(i)) + 1)) {
    AV(e) = VINDEX(HASH_TABLE(AV(hash)), FIX2INT(AV(index)));
    /* If we see an empty bucket in our search, or if we see a bucket
       whose key is the same as the key specified, we have found the
       place where the element should go. */
    if (EMPTYP(AV(e)))
      break;
    AFCALL(arc_mkaff(c, arc_iso, CNIL), BKEY(AV(e)), AV(key));
    if (AFCRV == CTRUE)
      break;
    /* We found a bucket, but it is occupied by some other key. Continue
       probing. */
    AV(index) = INT2FIX((FIX2INT(AV(index)) + PROBE(FIX2INT(AV(i)))) & TABLEMASK(AV(hash)));
  }

  if (EMPTYP(AV(e))) {
    /* No such key in the hash table yet.  Create a bucket and
       assign it to the table. */
    AV(e) = mkhashbucket(c, AV(key), AV(val), FIX2INT(AV(index)), AV(hash),
			 AV(hv));
    VINDEX(HASH_TABLE(AV(hash)), FIX2INT(AV(index))) = AV(e);
  } else {
    /* The key already exists.  Use the current bucket but change the
       value to the value specified. */
    BVALUE(AV(e)) = val;
  }
  ARETURN(AV(val));
  AFEND;
}
AFFEND

AFFDEF(arc_xhash_lookup)
{
  AARG(tbl, key);
  AFBEGIN;
  AFCALL(arc_mkaff(c, arc_xhash_lookup2, CNIL), AV(tbl), AV(key));
  if (BOUND_P(AFCRV))
    ARETURN(BVALUE(AFCRV));
  ARETURN(CUNBOUND);
  AFEND;
}
AFFEND

AFFDEF(arc_xhash_delete)
{
  AARG(tbl, key);
  value index, e;
  AFBEGIN;
  AFCALL(arc_mkaff(c, arc_xhash_lookup2, CNIL), AV(tbl), AV(key));
  if (!BOUND_P(AFCRV))
    ARETURN(CUNBOUND);

  index = car(AFCRV);
  e = VINDEX(HASH_TABLE(AV(tbl)), FIX2INT(index));
  BTABLE(e) = CNIL;
  VINDEX(HASH_TABLE(AV(tbl)), FIX2INT(index)) = CUNDEF;
  SET_NENTRIES(AV(tbl), HASH_NENTRIES(AV(tbl))-1);
  ARETURN(BVALUE(e));
  AFEND;
}
AFFEND

AFFDEF(arc_xhash_iter)
{
  AARG(hash, state);
  value index, keyval, e;
  AFBEGIN;
  if (NIL_P(AV(state)))
    AV(state) = cons(c, cons(c, CNIL, CNIL), INT2FIX(0));
  keyval = car(AV(state));
  index = cdr(AV(state));
  while (FIX2INT(index) < VECLEN(HASH_TABLE(AV(hash)))) {
    e = VINDEX(HASH_TABLE(AV(hash)), FIX2INT(index));
    index = __arc_add2(c, index, INT2FIX(1));
    if (EMPTYP(e))
      continue;
    scdr(AV(state), index);
    scar(keyval, BKEY(e));
    scdr(keyval, BVALUE(e));
    ARETURN(AV(state));
  }
  ARETURN(CNIL);
  AFEND;
}
AFFEND

AFFDEF(arc_xhash_map)
{
  AARG(proc, table);
  AVAR(state);
  AFBEGIN;

  AV(state) = CNIL;
  for (;;) {
    AFCALL(arc_mkaff(c, arc_xhash_iter, CNIL), AV(table), AV(state));
    AV(state) = AFCRV;
    if (NIL_P(AV(state)))
      ARETURN(AV(table));
    AFCALL(AV(proc), car(car(AV(state))), cdr(car(AV(state))));
  }
  ARETURN(AV(table));		/* never get here? */
  AFEND;
}
AFFEND

static AFFDEF(hash_xcoerce)
{
  AARG(obj, stype, arg);
  AVAR(state, list);
  AFBEGIN;
  (void)arg;
  if (FIX2INT(AV(stype)) == T_TABLE)
    ARETURN(AV(obj));

  if (FIX2INT(AV(stype)) == T_CONS) {
    AV(list) = AV(state) = CNIL;
    for (;;) {
      AFCALL(arc_mkaff(c, arc_xhash_iter, CNIL), AV(obj), AV(state));
      AV(state) = AFCRV;
      if (NIL_P(AV(state))) {
	ARETURN(AV(list));
      }
      AV(list) = cons(c, cons(c, car(car(AV(state))), cdr(car(AV(state)))),
		      AV(list));
    }
    /* never get here? */
    ARETURN(AV(list));
  }
  arc_err_cstrfmt(c, "cannot coerce");
  ARETURN(CNIL);
  AFEND;
}
AFFEND

AFFDEF(hash_xhash)
{
  AARG(obj, ehs, length, visithash);
  AVAR(state);
  AFBEGIN;

  if (!BOUND_P(AV(visithash)))
    AV(visithash) = arc_mkhash(c, ARC_HASHBITS);

  /* Already visited at some point.  Do not recurse further. */
  if (__arc_visit(c, AV(obj), AV(visithash)) != CNIL)
    ARETURN(AV(length));

  /* Iterate over all keys and values */
  AV(state) = CNIL;
  for (;;) {
    AFCALL(arc_mkaff(c, arc_xhash_iter, CNIL), AV(obj), AV(state));
    AV(state) = AFCRV;
    if (NIL_P(AV(state))) {
      ARETURN(AV(length));
    }
    /* increment the hash over the key */
    AFCALL(arc_mkaff(c, arc_xhash_increment, CNIL), car(car(AV(state))),
	   AV(ehs), AV(visithash));
    AV(length) = __arc_add2(c, AV(length), AFCRV);
    /* increment the hash over the value */
    AFCALL(arc_mkaff(c, arc_xhash_increment, CNIL), cdr(car(AV(state))),
	   AV(ehs), AV(visithash));
    AV(length) = __arc_add2(c, AV(length), AFCRV);
  }
  /* never get here? */
  ARETURN(AV(length));
  AFEND;
}
AFFEND

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
  NULL,
  NULL,
  NULL,
  hash_isocmp,
  hash_apply,
  hash_xcoerce,
  hash_xhash
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
  NULL,
  NULL,
  NULL,
  hash_isocmp,
  hash_apply,
  hash_xcoerce,
  hash_xhash
};
