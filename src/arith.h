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
extern value __arc_bitand2(arc *c, value arg1, value arg2);
extern value __arc_bitor2(arc *c, value arg1, value arg2);
extern value __arc_bitxor2(arc *c, value arg1, value arg2);
extern value __arc_amul_2exp(arc *c, value acc, value arg1, int n);

#endif
