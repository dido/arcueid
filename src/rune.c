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
#include "arcueid.h"

static uint64_t rhash(arc *c, value val, uint64_t seed)
{
  struct hash_ctx ctx;
  Rune *r = (Rune *)val;
  uint64_t rv = (uint64_t)(*r);
  static const uint64_t magic = 0x6875aecf4c191260ULL;

  __arc_hash_init(&ctx, seed);
  __arc_hash_update(&ctx, &magic, 1);
  __arc_hash_update(&ctx, &rv, 1);
  return(__arc_hash_final(&ctx));
}

static void rinit(arc *c)
{
  c->runetbl = arc_tbl_new_flags(c, 6, HASH_WEAK_VAL);
}

/* No type-specific is or iso predicates are needed for runes. Since
   rune objects are enforced to be singletons, comparing their
   references ought to be enough. */
arctype __arc_rune_t = { NULL, NULL, rhash, NULL, NULL, rinit };

value arc_rune_new(arc *c, Rune r)
{
  value rk = INT2FIX(r), rv;

  rv = __arc_tbl_lookup(c, c->runetbl, rk);
  if (rv == CUNBOUND) {
    rv = arc_new(c, &__arc_rune_t, sizeof(Rune));
    Rune *rp = (Rune *)rv;
    *rp = r;
    __arc_tbl_insert(c, c->runetbl, rk, rv);
  }
  return(rv);
}
