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
#ifndef _VMENGINE_H_

#define _VMENGINE_H_

enum vminst {
  inop=0,
  ipush=1,
  ipop=2,
  ildl=67,
  ildi=68,
  ildg=69,
  istg=70,
  ilde=135,
  iste=136,
  imvarg=73,
  imvoarg=74,
  imvrarg=75,
  icont=140,
  ienv=141,
  iapply=78,
  iret=15,
  ijmp=80,
  ijt=81,
  ijf=82,
  itrue=19,
  inil=20,
  ihlt=21,
  iadd=22,
  isub=23,
  imul=24,
  idiv=25,
  icons=26,
  icar=27,
  icdr=28,
  iscar=29,
  iscdr=30,
  ispl=31,
  iis=32,
  iiso=33,
  igt=34,
  ilt=35,
  idup=36,
  icls=37,
  iconsr=38
};

#define CODE_CODE(c) (VINDEX((c), 0))
#define CODE_SRC(c) (VINDEX((c), 1))
#define CODE_LITERAL(c, idx) (VINDEX((c), 2+(idx)))

/* The source information is a hash table, whose keys are code offsets
   into the function and whose values are line numbers.  The following
   special negative indexes are used for metadata. */
/* File name of the compiled file */
#define SRC_FILENAME (-1)
/* Function name */
#define SRC_FUNCNAME (-2)

#endif
