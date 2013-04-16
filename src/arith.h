/* 
  Copyright (C) 2013 Rafael R. Sevilla

  This file is part of Arcueid

  Arcueid is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __ARITH_H__

#define __ARITH_H__

#include <math.h>
#include <complex.h>
#include "arcueid.h"
#include "../config.h"

#define ABS(x) (((x)>=0)?(x):(-(x)))
#define SGN(x) (((x)>=0)?(1):(-(1)))

#ifdef HAVE_GMP_H
#include <gmp.h>
#define REPBNUM(n) *((mpz_t *)REP(n))
#define REPRAT(q) *((mpq_t *)REP(q))
#endif

#define REPFLO(f) *((double *)REP(f))
#define REPCPX(z) *((double complex *)REP(z))

extern value arc_mkflonum(arc *c, double val);
extern value arc_mkcomplex(arc *c, double complex z);
extern value arc_mkbignuml(arc *c, long val);
extern value arc_mkrationall(arc *c, long num, long den);

extern value __arc_add2(arc *c, value arg1, value arg2);
extern value __arc_sub2(arc *c, value arg1, value arg2);
extern value __arc_mul2(arc *c, value arg1, value arg2);
extern value __arc_div2(arc *c, value arg1, value arg2);
extern value __arc_idiv2(arc *c, value arg1, value arg2);
extern value __arc_mod2(arc *c, value arg1, value arg2);
extern value __arc_neg(arc *c, value arg);
extern value __arc_str2flo(arc *c, value obj, value b, int strptr, int limit);
extern value __arc_str2int(arc *c, value obj, value base, int strptr, int limit);
extern value (*__arc_coercefn(arc *c, value val))(arc *, value, enum arc_types);;
extern value __arc_bignum_fixnum(arc *c, value n);
extern value __arc_rational_int(arc *c, value n);


extern value arc_numcmp(arc *c, value v1, value v2);
extern value arc_exact(arc *c, value v);

extern int __arc_add(arc *c, value thr);
extern int __arc_sub(arc *c, value thr);
extern int __arc_mul(arc *c, value thr);
extern int __arc_div(arc *c, value thr);

extern value arc_expt(arc *c, value a, value b);
extern value arc_sqrt(arc *c, value v);
extern value __arc_real(arc *c, value v);
extern value __arc_imag(arc *c, value v);
extern value __arc_conj(arc *c, value v);
extern value __arc_arg(arc *c, value v);

extern value arc_srand(arc *ccc, value seed);
extern int arc_rand(arc *c, value thr);
extern value arc_trunc(arc *c, value v);
extern value arc_abs(arc *c, value v);

#endif
