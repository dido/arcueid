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
