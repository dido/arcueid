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

static void markroots(arc *c, void (*marker)(struct arc *, value))
{
  /* XXX fill in later */
}

void arc_init(arc *c)
{
  c->mm_ctx = __arc_new_mm_ctx();
  c->gc_ctx = __arc_new_gc_ctx(c);
  c->markroots = markroots;
}

arctype __arc_nil_t = { NULL, NULL, __arc_immediate_hash, 0 };

arctype *arc_type(value val)
{
  struct GChdr *gh;

  if (NILP(val))
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
