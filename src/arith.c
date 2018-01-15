/* 
  Copyright (C) 2017,2018 Rafael R. Sevilla

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
#include "../config.h"

#ifdef HAVE_GMP_H
#include <gmp.h>
#endif

#define ABS(x) (((x)>=0)?(x):(-(x)))

/* Type for fixnums */
arctype __arc_fixnum_t = { NULL, NULL, __arc_immediate_hash, NULL, NULL, NULL };

static uint64_t flonum_hash(arc *c, value fl, uint64_t seed)
{
  /* XXX -- this is only really reliable as a hasher when the size of
     a double and a uint64 are the same. If not, we ought to do
     something better. */
  struct hash_ctx ctx;
  union { double fl; uint64_t h; } t;

  t.fl = *((double *)fl);
  /* first 64 bits of sha512 of 'flonum' */
  static const uint64_t magic = 0x55f11188ffce44c2ULL;
  
  __arc_hash_init(&ctx, seed);
  __arc_hash_update(&ctx, &magic, 1);
  __arc_hash_update(&ctx, &t.h, 1);
  return(__arc_hash_final(&ctx));
}

static int flonum_iso(arc *c, value v1, value v2)
{
  return(*((double *)v1) == *((double *)v2));
}

/* Type for flonums */
arctype __arc_flonum_t = { NULL, NULL, flonum_hash, flonum_iso, flonum_iso, NULL };

value arc_flonum_new(arc *c, double f)
{
  value fl = arc_new(c, &__arc_flonum_t, sizeof(double));
  double *flp = (double *)fl;
  *flp = f;
  return(fl);
}

value __arc_add2(arc *c, value arg1, value arg2)
{
  long fixnum_sum;

  if (FIXNUMP(arg1) && FIXNUMP(arg2)) {
    fixnum_sum = FIX2INT(arg1) + FIX2INT(arg2);
    if (ABS(fixnum_sum) > FIXNUM_MAX) {
#ifdef HAVE_GMP_H
    /* XXX - bignum support */
#else
      return(arc_flonum_new(c, (double)fixnum_sum));
#endif
    }
    return(INT2FIX(fixnum_sum));
  }
  /* XXX - more type handling for addition */
  return(CNIL);
}
