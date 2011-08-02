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

#ifndef _TYPEHANDLING_H_
#define _TYPEHANDLING_H_

enum {
  IDX_sym=0,
  IDX_fixnum,
  IDX_bignum,
  IDX_flonum,
  IDX_rational,
  IDX_complex,
  IDX_char,
  IDX_string,
  IDX_cons,
  IDX_table,
  IDX_input,
  IDX_output,
  IDX_exception,
  IDX_port,
  IDX_thread,
  IDX_vector,
  IDX_continuation,
  IDX_closure,
  IDX_code,
  IDX_environment,
  IDX_vmcode,
  IDX_ccode,
  IDX_custom,
  IDX_int,
  IDX_unknown,
  IDX_re,
  IDX_im,
  IDX_num,
  IDX_nil,
  IDX_t,
  IDX_sig
};

extern value __arc_typesym(arc *c, int index);

#endif
