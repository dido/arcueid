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
#include "arcueid.h"
#include "utf.h"
#include "arith.h"
#include "hash.h"

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

void __arc_append_buffer_close(arc *c, Rune *buf, int *idx, value *str)
{
  value nstr;

  nstr = arc_mkstring(c, buf, *idx);
  *str = (*str == CNIL) ? nstr : arc_strcat(c, *str, nstr);
  *idx = 0;
}

void __arc_append_buffer(arc *c, Rune *buf, int *idx, int bufmax,
			 Rune ch, value *str)
{
  if (*idx >= bufmax)
    __arc_append_buffer_close(c, buf, idx, str);
  buf[(*idx)++] = ch;
}

void __arc_append_cstring(arc *c, char *buf, value *ppstr)
{
  value nstr = arc_mkstringc(c, buf);

  *ppstr = (*ppstr == CNIL) ? nstr : arc_strcat(c, *ppstr, nstr);
}

value __arc_visitkey(value v)
{
  unsigned long myhash;
  arc_hs s;

  arc_hash_init(&s, 0);
  arc_hash_update(&s, (unsigned long)v);
  myhash = arc_hash_final(&s, 1);
  return(INT2FIX((long)myhash));
}

/* This function is used for recursive visits in data structures which
   may be cyclic.  It uses v as an actual unsigned long, which is then
   converted into a fixnum after hashing, and that is used as the key for
   the hash table.  If the key is not already present, it returns CNIL
   and adds the key.  If the key is present, it returns whatever
   other value has been set for it in the hash. */
value __arc_visit2(arc *c, value v, value hash, value mykeyval)
{
  value keyval, val;

  keyval = __arc_visitkey(v);
  if (mykeyval == CNIL)
    mykeyval = keyval;

  if ((val = arc_hash_lookup(c, hash, keyval)) == CUNBOUND) {
    arc_hash_insert(c, hash, keyval, mykeyval);
    return(CNIL);
  }
  return(val);
}

value __arc_visit(arc *c, value v, value hash)
{
  return(__arc_visit2(c, v, hash, CNIL));
}

value __arc_visitp(arc *c, value v, value hash)
{
  value keyval;

  keyval = __arc_visitkey(v);
  return((arc_hash_lookup(c, hash, keyval) == CUNBOUND) ? CNIL : CTRUE);
}

void __arc_unvisit(arc *c, value v, value hash)
{
  arc_hash_delete(c, hash, __arc_visitkey(v));
}


void __arc_print_string(arc *c, value ppstr)
{
  char *str;

  str = (char *)alloca(FIX2INT(arc_strutflen(c, ppstr))*sizeof(char));
  arc_str2cstr(c, ppstr, str);
  printf("%s\n", str);
}

Rune __arc_strgetc(arc *c, value str, int *index)
{
  if (*index < arc_strlen(c, str))
    return(arc_strindex(c, str, (*index)++));
  return(Runeerror);
}

void __arc_strungetc(arc *c, int *index)
{
  if (*index <= 0)
    return;
  (*index)--;
}

/* Utility functions for queues.
*/
void __arc_enqueue(arc *c, value thr, value *head, value *tail)
{
  value cell;

  cell = cons(c, thr, CNIL);
  if (*head == CNIL && *tail == CNIL) {
    __arc_wb(*head, cell);
    *head = cell;
    __arc_wb(*tail, cell);
    *tail = cell;
    return;
  }
  scdr(*tail, cell);
  *tail = cell;
}

value __arc_dequeue(arc *c, value *head, value *tail)
{
  value thr;

  /* empty queue */
  if (*head == CNIL && *tail == CNIL)
    return(CNIL);
  thr = car(*head);
  __arc_wb(*head, cdr(*head));
  *head = cdr(*head);
  if (NIL_P(*head)) {
    __arc_wb(*tail, *head);
    *tail = *head;
  }
  return(thr);
}

value __arc_ull2val(arc *c, unsigned long long ms)
{
  if (ms < FIXNUM_MAX) {
    return(INT2FIX(ms));
  } else {
#ifdef HAVE_GMP_H
    value msbn;

#if SIZEOF_UNSIGNED_LONG_LONG == 8
    /* feed value into the bignum 32 bits at a time */
    msbn = arc_mkbignuml(c, (ms >> 32)&0xffffffff);
    mpz_mul_2exp(REPBNUM(msbn), REPBNUM(msbn), 32);
    mpz_add_ui(REPBNUM(msbn), REPBNUM(msbn), ms & 0xffffffff);
#else
    int i;

    msbn = arc_mkbignuml(c, 0);
    for (i=SIZEOF_UNSIGNED_LONG_LONG-1; i>=0; i--) {
      mpz_mul_2exp(REP(msbn)._bignum, REP(msbn)._bignum, 8);
      mpz_add_ui(REP(msbn)._bignum, REP(msbn)._bignum, (ms >> (i*8)) & 0xff);
    }
#endif
    return(msbn);
#else
    /* floating point */
    return(arc_mkflonum(c, (double)ms));
#endif
  }
  return(CNIL);
}
