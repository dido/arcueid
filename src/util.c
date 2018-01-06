/* Copyright (C) 2017 Rafael R. Sevilla

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


/* Random number generator based on
   http://burtleburtle.net/bob/rand/smallprng.html */
#include "arcueid.h"

#define rot(x,k) (((x)<<(k))|((x)>>(64-(k))))

uint64_t __arc_rand(struct ranctx *x)
{
  uint64_t e = x->a - rot(x->b, 7);
  x->a = x->b ^ rot(x->c, 13);
  x->b = x->c + rot(x->d, 37);
  x->c = x->d + e;
  x->d = e + x->a;
  return(x->d);
}

extern void __arc_srand(struct ranctx *x, uint64_t seed)
{
  uint64_t i;
  x->a = 0xf1ea5eed, x->b = x->c = x->d = seed;
  for (i=0; i<20; ++i) {
    (void)__arc_rand(x);
  }
}
