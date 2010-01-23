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
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA
  02110-1301 USA.
*/

#ifndef _SYMBOLS_H_

#define _SYMBOLS_H_

enum builtin_syms {
  S_FN,				/* fn syntax */
  S_US,				/* underscore */
  S_QUOTE,			/* quote */
  S_QQUOTE,			/* quasiquote */
  S_UNQUOTE,			/* unquote */
  S_UNQUOTESP,			/* unquote-splicing */
  S_COMPOSE,			/* compose */
  S_COMPLEMENT,			/* complement */
  S_T,				/* true */
  S_NIL,			/* nil */
  S_NO,				/* no */
  S_ANDF,			/* andf */
  S_GET,			/* get */
  S_THE_END			/* end of the line */
};

#define ARC_BUILTIN(c, sym) (VINDEX((c)->builtin, sym))

#endif
