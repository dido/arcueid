/* Copyright (C) 2017, 2018 Rafael R. Sevilla

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

#include <setjmp.h>
#include <assert.h>

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
  icont=137,
  ienv=202,
  ienvr=203,
  iapply=76,
  iret=13,
  ijmp=78,
  ijt=79,
  ijf=80,
  ijbnd=81,
  itrue=18,
  inil=19,
  ino=20,
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
  iis=31,
  idup=34,
  icls=35,
  iconsr=36,
  imenv=101,
  idcar=38,
  idcdr=39,
  ispl=40,
  ilde0=105,
  iste0=106
};

#endif
