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
#include "arcueid.h"
#include "coroutine.h"

/* Read a CIEL 0.0.0 file */
value arc_ciel_unmarshal_v000(arc *c, value fd)
{
  static char header[] = { 0xc1, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  int i, flag;

  /* Verify the header */
  flag = 1;
  for (i=0; i<8; i++) {
    if (FIX2INT(arc_readb(c, fd) != header[i])) {
      flag = 0;
      break;
    }
  }
  if (!flag)
    c->signal_error(c, "ciel-unmarshal-v000: invalid header found in file %v", fd);
  return(CNIL);
}
