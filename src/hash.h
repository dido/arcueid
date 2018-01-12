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
#ifndef _HASH_H_
#define _HASH_H_

/*! \def NHASH
    \brief The number of hashes used
 */
#define NHASH 3

typedef struct {
  int nbits;		/*!< number of hash bits  */
  int usage;	       	/*!< number of mappings in the table */
  int stashsize;	/*!< size of stash  */
  int threshold;	/*!< number of mappings beyond which we have to
			  resize */
  value (*getkey)(arc *, value, uint64_t);
  value (*getval)(arc *, value, uint64_t);
  void (*setkey)(arc *, value, uint64_t, value);
  void (*setval)(arc *, value, uint64_t, value);
  value k;		/*!< Table of keys */
  value v;		/*!< Table of values */
} hashtbl;

#define HASHSIZE(n) ((unsigned long)1 << (n))
#define HASHBITS(tbl) (((hashtbl *)(tbl))->nbits)
#define HASHMASK(tbl) (HASHSIZE(HASHBITS(tbl))-1)
#define HIDX(tbl, kv, hash) (VIDX(((hashtbl *)tbl)->kv, (hash) & (HASHMASK(tbl))))
#define SHIDX(c, tbl, kv, hash, x) (SVIDX((c), ((hashtbl *)tbl)->kv, (hash) & (HASHMASK(tbl)), (x)))
#define USAGE(tbl, op) (((hashtbl *)(tbl))->usage op)
#define THRESHOLD(tbl) (((hashtbl *)(tbl))->threshold)

/*! \def MAXLOAD
    \brief Maximum load factor as a percentage
 */
#define MAXLOAD 80

#endif
