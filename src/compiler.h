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
#ifndef _COMPILER_H_

#define _COMPILER_H_

/* Special syntax handling */
extern value arc_ssyntax(arc *c, value x);
extern int arc_ssexpand(arc *c, value thr);

/* The compiler */
extern int arc_compile(arc *c, value thr);
extern int arc_eval(arc *c, value thr);
extern int arc_quasiquote(arc *c, value thr);

/* Macros */
extern int arc_macex(arc *c, value thr);
extern int arc_macex1(arc *c, value thr);
extern value arc_uniq(arc *c);

#endif
