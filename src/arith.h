/* 
  Copyright (C) 2010 Rafael R. Sevilla

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
#ifndef _ARITH_H_
#define _ARITH_H_

extern value __arc_neg(arc *c, value arg);

extern value __arc_add2(arc *c, value arg1, value arg2);
extern value __arc_sub2(arc *c, value arg1, value arg2);
extern value __arc_mul2(arc *c, value arg1, value arg2);
extern value __arc_div2(arc *c, value arg1, value arg2);
extern value __arc_mod2(arc *c, value arg1, value arg2);
extern value __arc_idiv2(arc *c, value arg1, value arg2);
extern value __arc_bitand2(arc *c, value arg1, value arg2);
extern value __arc_bitor2(arc *c, value arg1, value arg2);
extern value __arc_bitxor2(arc *c, value arg1, value arg2);
extern value __arc_amul_2exp(arc *c, value acc, value arg1, int n);

extern value __arc_itoa(arc *c, value num, value base, int uc, int sign);
extern value __arc_abs(arc *c, value v);

extern value __arc_add(arc *c, int argc, value *argv);
extern value __arc_sub(arc *c, int argc, value *argv);
extern value __arc_mul(arc *c, int argc, value *argv);
extern value __arc_div(arc *c, int argc, value *argv);

extern value __arc_real(arc *c, value v);
extern value __arc_imag(arc *c, value v);
extern value __arc_conj(arc *c, value v);
extern value __arc_arg(arc *c, value v);

extern value __arc_acos(arc *c, value v);
extern value __arc_acosh(arc *c, value v);
extern value __arc_asin(arc *c, value v);
extern value __arc_asinh(arc *c, value v);
extern value __arc_atan(arc *c, value v);
extern value __arc_atan2(arc *c, value v1, value v2);
extern value __arc_atanh(arc *c, value v);
extern value __arc_cbrt(arc *c, value v);
extern value __arc_ceil(arc *c, value v);
extern value __arc_cos(arc *c, value v);
extern value __arc_cosh(arc *c, value v);
extern value __arc_erf(arc *c, value v);
extern value __arc_erfc(arc *c, value v);
extern value __arc_exp(arc *c, value v);
extern value __arc_expm1(arc *c, value v);
extern value __arc_floor(arc *c, value x);
extern value __arc_fmod(arc *c, value x, value y);
extern value __arc_frexp(arc *c, value v);
extern value __arc_hypot(arc *c, value x, value y);
extern value __arc_ldexp(arc *c, value x, value y);
extern value __arc_lgamma(arc *c, value v);
extern value __arc_log(arc *c, value v);
extern value __arc_log10(arc *c, value v);
extern value __arc_log2(arc *c, value v);
extern value __arc_logb(arc *c, value v);
extern value __arc_modf(arc *c, value v);
extern value __arc_nan(arc *c, int argc, value *argv);
extern value __arc_nearbyint(arc *c, value v);
extern value __arc_sin(arc *c, value v);
extern value __arc_sinh(arc *c, value v);
extern value __arc_tan(arc *c, value v);
extern value __arc_tanh(arc *c, value v);
extern value __arc_tgamma(arc *c, value v);
extern value __arc_trunc(arc *c, value v);

/* Mathematical functions */
extern value arc_expt(arc *c, value a, value b);
extern value arc_rand(arc *c, int argc, value *argv);
extern value arc_srand(arc *ccc, value seed);
extern value arc_trunc(arc *c, value v);
extern value arc_sqrt(arc *c, value v);

#endif
