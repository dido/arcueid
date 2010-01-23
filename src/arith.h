/* 
  Copyright (C) 2009 Rafael R. Sevilla

  This file is part of Arcueid

  Arcueid is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA
  02110-1301 USA.
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

#endif
