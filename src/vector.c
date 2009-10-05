/* 
  Copyright (C) 2009 Rafael R. Sevilla

  This file is part of CArc

  CArc is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; either version 3 of the
  License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA
  02110-1301 USA.
*/
#include <string.h>
#include "carc.h"

value carc_mkvector(carc *c, int length)
{
  void *cellptr;
  value vect;

  cellptr = c->get_block(c, sizeof(struct cell) + sizeof(value)*length);
  if (cellptr == NULL)
    return(CNIL);
  vect = (value)cellptr;
  REP(vect)._vector.length = length;
  memset(REP(vect)._vector.data, 0, sizeof(value)*length);
  return(vect);
}
