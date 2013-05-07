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
#ifndef _HASH_H_

#define _HASH_H_

extern void arc_hash_init(arc_hs *s, unsigned long level);
extern void arc_hash_update(arc_hs *s, unsigned long val);
extern unsigned long arc_hash_final(arc_hs *s, unsigned long len);
extern unsigned long arc_hash_increment(arc *c, value v, arc_hs *s);
extern unsigned long arc_hash(arc *c, value v);
extern value arc_mkhash(arc *c, int hashbits);
extern value arc_mkwtable(arc *c, int hashbits);
extern int arc_newtable(arc *c, value thr);
extern value arc_hash_lookup(arc *c, value tbl, value key);
extern value arc_hash_lookup2(arc *c, value tbl, value key);
extern value arc_hash_insert(arc *c, value hash, value key, value val);
extern value arc_hash_delete(arc *c, value hash, value key);
extern int arc_hash_length(arc *c, value hash);
extern int arc_xhash_lookup(arc *c, value thr);
extern int arc_xhash_lookup2(arc *c, value thr);
extern int arc_xhash_delete(arc *c, value thr);
extern int arc_xhash_insert(arc *c, value thr);
extern int arc_xhash_increment(arc *c, value thr);
extern int arc_xhash_iter(arc *c, value thr);
extern int arc_xhash_map(arc *c, value thr);

#endif
