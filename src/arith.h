/* 
  Copyright (C) 2009 Rafael R. Sevilla

  This file is part of CArc

  CArc is free software; you can redistribute it and/or modify it
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

extern value __carc_neg(carc *c, value arg);

extern value __carc_add2(carc *c, value arg1, value arg2);
extern value __carc_sub2(carc *c, value arg1, value arg2);
extern value __carc_mul2(carc *c, value arg1, value arg2);
extern value __carc_div2(carc *c, value arg1, value arg2);
extern value __carc_mod2(carc *c, value arg1, value arg2);
extern value __carc_bitand2(carc *c, value arg1, value arg2);
extern value __carc_bitor2(carc *c, value arg1, value arg2);
extern value __carc_bitxor2(carc *c, value arg1, value arg2);

#endif
