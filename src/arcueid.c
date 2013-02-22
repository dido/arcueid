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
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <float.h>
#include <inttypes.h>
#include "arcueid.h"

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
#ifndef alloca
# define alloca __builtin_alloca
#endif
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# include <stddef.h>
void *alloca (size_t);
#endif

void __arc_null_marker(arc *c, value v, int depth,
			void (*markfn)(arc *, value, int))
{
  /* Does nothing */
}

void __arc_null_sweeper(arc *c, value v)
{
  /* Does nothing */
}

value arc_prettyprint(arc *c, value sexpr, value *ppstr, value visithash)
{
  typefn_t *tfn;

  switch (TYPE(sexpr)) {
  case T_NIL:
    __arc_append_cstring(c, "nil", ppstr);
    break;
  case T_TRUE:
    __arc_append_cstring(c, "t", ppstr);
    break;
  case T_FIXNUM:
    {
      long val = FIX2INT(sexpr);
      int len;
      char *outstr;

      len = snprintf(NULL, 0, "%ld", val) + 1;
      outstr = (char *)alloca(sizeof(char)*(len+2));
      snprintf(outstr, len+1, "%ld", val);
      __arc_append_cstring(c, outstr, ppstr);
    }
    break;
  case T_SYMBOL:
    /* XXX - handle this case */
    break;
  case T_NONE:
    /* XXX - this is an error case that needs handling */
    break;
  default:
    /* non-immediate type */
    tfn = __arc_typefn(c, sexpr);
    tfn->pprint(c, sexpr, ppstr, visithash);
    break;
  }
  return(*ppstr);
}

value arc_mkobject(arc *c, size_t size, int type)
{
  struct cell *cc;

  cc = (struct cell *)c->alloc(c, sizeof(struct cell) + size - sizeof(value));
  cc->_type = type;
  return((value)cc);
}

static value vector_pprint(arc *c, value sexpr, value *ppstr, value visithash)
{
  /* XXX fill this in */
  return(CNIL);
}

static void vector_marker(arc *c, value v, int depth,
			  void (*markfn)(arc *, value, int))
{
  int i;

  for (i=0; i<VECLEN(v); i++)
    markfn(c, VINDEX(v, i), depth);
}

static value vector_hash(arc *c, value v, arc_hs *s, value visithash)
{
  unsigned long len;
  int i;

  if (visithash == CNIL)
    visithash = arc_mkhash(c, ARC_HASHBITS);
  if (__arc_visit(c, v, visithash) != CNIL) {
    /* Already visited at some point.  Do not recurse further.  An already
       visited node will still contribute 1 to the length though. */
    return(1);
  }
  len = 0;
  for (i=0; i<VECLEN(v); i++)
    len += arc_hash(c, VINDEX(v, i), visithash);
  return(len);
}

static value vector_isocmp(arc *c, value v1, value v2, value vh1, value vh2)
{
  value vhh1, vhh2;
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
  /* Vectors must be identical in length to be the same */
  if (VECLEN(v1) != VECLEN(v2))
    return(CNIL);
  /* Recursive comparisons */
  for (i=0; i<VECLEN(v1); i++) {
    if (arc_iso(c, VINDEX(v1, i), VINDEX(v2, i), vh1, vh2) != CTRUE)
      return(CNIL);
  }
  return(CTRUE);
}

value arc_mkvector(arc *c, int length)
{
  value vec;
  int i;

  vec = arc_mkobject(c, (length+1)*sizeof(value), T_VECTOR);
  VINDEX(vec, -1) = INT2FIX(length);
  for (i=0; i<length; i++)
    VINDEX(vec, i) = CNIL;
  return(vec);
}

value arc_is(arc *c, value v1, value v2)
{
  typefn_t *tfn;

  /* An object is definitely the same as itself */
  if (v1 == v2)
    return(CTRUE);
  /* Two objects with different types are definitely not the same */
  if (TYPE(v1) != TYPE(v2))
    return(CNIL);

  /* v1 == v2 check should have covered this, but just in case */
  if (IMMEDIATE_P(v1))
    return(CNIL);

  tfn = __arc_typefn(c, v1);
  /* If no iscmp is defined, two objects of that type can be equal if and
     only if their references are equal (and they weren't from above). */
  if (tfn->iscmp == NULL)
    return(CNIL);
  return(tfn->iscmp(c, v1, v2));
}

value arc_iso(arc *c, value v1, value v2, value vh1, value vh2)
{
  typefn_t *tfn;

  /* An object is definitely the same as itself */
  if (v1 == v2)
    return(CTRUE);
  /* Two objects with different types are definitely not the same */
  if (TYPE(v1) != TYPE(v2))
    return(CNIL);
  /* v1 == v2 check should have covered this, but just in case */
  if (IMMEDIATE_P(v1))
    return(CNIL);

  tfn = __arc_typefn(c, v1);
  if (tfn->isocmp == NULL)
    return(CNIL);
  return(tfn->isocmp(c, v1, v2, vh1, vh2));
}

typefn_t __arc_vector_typefn__ = {
  vector_marker,
  __arc_null_sweeper,
  vector_pprint,
  vector_hash,
  NULL,
  vector_isocmp
};

typefn_t *__arc_typefn(arc *c, value v)
{
  value typedesc;

  if (TYPE(v) != T_TAGGED)
    return(c->typefns[TYPE(v)]);
  /* For tagged types (custom types), the type descriptor hash should*/
  typedesc = arc_hash_lookup(c, c->typedesc, car(v));
  if (typedesc == CNIL)
    return(NULL);		/* XXX should this be an error? */
  return((typefn_t *)REP(typedesc));
}

extern typefn_t __arc_flonum_typefn__;
extern typefn_t __arc_complex_typefn__;
extern typefn_t __arc_string_typefn__;
extern typefn_t __arc_char_typefn__;

#ifdef HAVE_GMP_H
extern typefn_t __arc_bignum_typefn__;
#endif

extern typefn_t __arc_cons_typefn__;
extern typefn_t __arc_table_typefn__;
extern typefn_t __arc_hb_typefn__;

void arc_init_datatypes(arc *c)
{
  c->typefns[T_FLONUM] = &__arc_flonum_typefn__;
  c->typefns[T_COMPLEX] = &__arc_complex_typefn__;
  c->typefns[T_STRING] = &__arc_string_typefn__;
  c->typefns[T_CHAR] = &__arc_char_typefn__;

#ifdef HAVE_GMP_H
  c->typefns[T_BIGNUM] = &__arc_bignum_typefn__;
#endif

  c->typefns[T_CONS] = &__arc_cons_typefn__;
  c->typefns[T_VECTOR] = &__arc_vector_typefn__;
  c->typefns[T_TABLE] = &__arc_table_typefn__;
  c->typefns[T_TBUCKET] = &__arc_hb_typefn__;
}
