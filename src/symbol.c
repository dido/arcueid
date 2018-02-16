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

static void mark(arc *c, value v,
		     void (*marker)(struct arc *, value, int),
		     int depth)
{
  value *v2 = (value *)v;
  marker(c, *v2, depth);
}

static uint64_t hash(arc *c, value val, uint64_t seed)
{
  struct hash_ctx ctx;
  static const uint64_t magic = 0x60a8c2b8151f19e9ULL;
  value *s = (value *)val;

  __arc_hash_init(&ctx, seed);
  __arc_hash_update(&ctx, &magic, 1);
  return(__arc_hash(c, *s, __arc_hash_final(&ctx)));
}

static void syminit(arc *c)
{
  c->obtbl = arc_tbl_new_flags(c, ARC_HASHBITS, HASH_WEAK_KEY | HASH_WEAK_VAL);
}

arctype __arc_sym_t = { NULL, mark, hash, NULL, NULL, syminit, NULL };

value arc_intern_cstr(arc *c, const char *s)
{
  value sym, str;

  sym = __arc_tbl_lookup_cstr(c, c->obtbl, s);
  if (sym != CUNBOUND)
    return(sym);

  str = arc_string_new_cstr(c, s);
  sym = arc_new(c, &__arc_sym_t, sizeof(value));
  *((value *)sym) = str;
  __arc_tbl_insert(c, c->obtbl, str, sym);
  return(sym);
}

value arc_intern(arc *c, value str)
{
  value sym;

  sym = __arc_tbl_lookup(c, c->obtbl, str);
  if (sym != CUNBOUND)
    return(sym);

  sym = arc_new(c, &__arc_sym_t, sizeof(value));
  *((value *)sym) = str;
  __arc_tbl_insert(c, c->obtbl, str, sym);
  return(sym);
}

value arc_sym2name(arc *c, value sym)
{
  return(*((value *)sym));
}
