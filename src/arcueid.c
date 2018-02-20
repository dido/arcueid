/*  Copyright (C) 2017, 2018 Rafael R. Sevilla

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
#include "arcueid.h"
#include "alloc.h"
#include "gc.h"
#include "../config.h"

struct {
  char *name;
  arctype *t;
} __arc_builtin_types[] = {
  { "nil", &__arc_nil_t },
  { "fixnum", &__arc_fixnum_t },
  { "flonum", &__arc_flonum_t },
  { "cons", &__arc_cons_t },
  { "hashtbl", &__arc_tbl_t },
  { "vector", &__arc_vector_t },
  { "wref", &__arc_wref_t },
  { "rune", &__arc_rune_t },
  { "sym", &__arc_sym_t },
  { "string", &__arc_string_t },
  { "io", &__arc_io_t },
  { "sio", &__arc_sio_t },
  { NULL, NULL}
};

static void markroots(arc *c, void (*marker)(struct arc *, value))
{
  marker(c, c->vmthreads);
  marker(c, c->runetbl);
  marker(c, c->obtbl);
  marker(c, c->fftbl);
  marker(c, c->genv);
}

void arc_init(arc *c)
{
  c->mm_ctx = __arc_new_mm_ctx();
  c->gc_ctx = __arc_new_gc_ctx(c);
  c->markroots = markroots;
  c->stksize = THREAD_STACK_SIZE;
}

void arc_types_init(arc *c)
{
  int i;

  c->genv = arc_tbl_new(c, ARC_HASHBITS);
  for (i=0; __arc_builtin_types[i].name; i++) {
    if (__arc_builtin_types[i].t->init != NULL)
      __arc_builtin_types[i].t->init(c);
  }
}

arctype __arc_nil_t = { NULL, NULL, __arc_immediate_hash, NULL, NULL, NULL, NULL };

arctype *arc_type(value val)
{
  struct GChdr *gh;

  if (NILP(val) || val == CUNBOUND || val == CLASTARG)
    return(&__arc_nil_t);

  if (FIXNUMP(val))
    return(&__arc_fixnum_t);

  V2GCH(gh, val);
  return(gh->t);
}

uint64_t __arc_immediate_hash(arc *c, value val, uint64_t seed)
{
  struct hash_ctx ctx;
  uint64_t tv = (uint64_t)val;

  __arc_hash_init(&ctx, seed);
  __arc_hash_update(&ctx, &tv, 1);
  return(__arc_hash_final(&ctx));
}

int __arc_is(arc *c, value v1, value v2)
{
  arctype *t;

  /* If they are the same object, it is obviously the same as itself.
     This comparison should suffice for immediate objects that are not
     pointers. */
  if (v1 == v2)
    return(1);

  /* If it is an immediate value, no further checks are required, they
     are evidently not the same */
  if (IMMEDIATEP(v1))
    return(0);

  /* If v1 and v2 have different types, they cannot be the same */
  t = arc_type(v1);
  if (t != arc_type(v2))
    return(0);

  /* If there is no type-specific function, 'is' cannot be used to
     compare two objects of this type, thus they cannot be the same
     under this definition unless they are the same object (as above) */
  if (t->is == NULL)
    return(0);

  /* Use type-specific function to compare */
  return(t->is(c, v1, v2));
}

value arc_bind(arc *c, value sym, value binding)
{
  return(__arc_tbl_insert(c, c->genv, sym, binding));
}

value arc_bindc(arc *c, const char *csym, value binding)
{
  return(arc_bind(c, arc_intern_cstr(c, csym), binding));
}

value arc_gbind(arc *c, value sym)
{
  return(__arc_tbl_lookup(c, c->genv, sym));
}

value arc_gbindc(arc *c, const char *csym)
{
  return(arc_gbind(c, arc_intern_cstr(c, csym)));
}

