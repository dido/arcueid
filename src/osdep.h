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
#ifndef _OSDEP_H_
#define _OSDEP_H_

/* OS-dependent functions */
extern unsigned long long __arc_milliseconds(void);
extern value arc_seconds(arc *c);
extern value arc_msec(arc *c);
extern value arc_current_process_milliseconds(arc *c);
extern value arc_setuid(arc *c, value uid);
extern int arc_timedate(arc *c, value thr);
extern int arc_system(arc *c, value thr);
extern int arc_quit(arc *c, value thr);

#endif
