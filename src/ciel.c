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
#include "arith.h"
#include "utf.h"
#include "../config.h"
#include "vmengine.h"

#define INIT_STACK_SIZE 1024
#define STACK_OVER_PERCENT 30
#define INIT_MEMO_SIZE 1024
#define MEMO_OVER_PERCENT 30

#define GNIL  0
#define GTRUE 1
#define GINT  2
#define GFLO  3
#define GCHAR 4
#define GSTR  5
#define GSYM  6
#define GTAB  7			/* not yet supported */
#define GBSTR 8

#define CRAT      9
#define CCOMPLEX  10
#define CTADD     11		/* not yet supported */
#define CCONS     12
#define CANNOTATE 13

#define XDUP 14
#define XMST 15
#define XMLD 16

static inline value stackpush(arc *c, value *stack, value val,
			      int *stackptr, int *stacksize)
{
  if (*stackptr > *stacksize) {
    int newsize;
    /* grow the stack */
    newsize = *stacksize + (((*stacksize) * STACK_OVER_PERCENT) / 100);
    *stack = arc_growvector(c, *stack, newsize);
  }
  VINDEX(*stack, *stackptr) = val;
  (*stackptr)++;
  return(val);
}

static inline value stackpop(arc *c, value *stack, int *stackptr,
			     int *stacksize)
{
  value val;

  (*stackptr)--;
  if ((*stackptr) < 0)
    c->signal_error(c, "CIEL stack underflow during decode");
  val = VINDEX(*stack, *stackptr);
  return(val);
}

#define PUSH(val) (stackpush(c, &stack, (val), &stackptr, &stacksize))
#define POP() (stackpop(c, &stack, &stackptr, &stacksize))

static int getsign(arc *c, value fd)
{
  return((FIX2INT(arc_readb(c, fd)) == '-') ? -1 : 1);
}

static value getint(arc *c, int sign, value fd)
{
  value acc = INT2FIX(0), parts[8];
  int i, last, pos = 0;

  /* CIEL stores integers in 64 bit chunks, little endian.  We rearrange
     the calculations here such that the only value that can possibly be
     a bignum is acc.
     XXX - we should probably make a smarter algorithm that takes
     advantage of 64-bit architectures. */
  for (;;) {
    for (i=0; i<8; i++) {
      parts[i] = arc_readb(c, fd);
      if (FIX2INT(parts[i]) < 0) {
	c->signal_error(c, "ciel-unmarshal/getint: invalid integer found in %v", fd);
      }
    }
    for (i=0; i<7; i++) {
      acc = __arc_amul_2exp(c, acc, parts[i], pos);
      pos += 8;
    }
    /* check if this is the last one */
    last = (FIX2INT(parts[7]) & 0x80) != 0;
    /* mask out the high bit if needed */
    parts[7] = INT2FIX(FIX2INT(parts[7]) & 0x7f);
    acc = __arc_amul_2exp(c, acc, parts[7], pos);
    pos += 7;
    /* stop if this is the end */
    if (last)
      break;
  }
  /* add the sign */
  acc = __arc_mul2(c, acc, INT2FIX(sign));
  return(acc);
}

/* Note: this assumes that doubles are IEEE-754.  This is true for nearly
   all modern architectures.  If you're trying to compile this code on
   a system where the C floating point types are not IEEE-754, it's your
   responsibility to port it! */
static value getflo(arc *c, value fd)
{
#if SIZEOF_DOUBLE != 8
#error "doubles are not 8 bytes in size, you probably need to do a port!"
#endif
  union { double d; char bytes[8]; } u;
  value ch;
  int i;

  for (i=0; i<8; i++) {
    ch = arc_readb(c, fd);
    if (FIX2INT(c) < 0) {
      c->signal_error(c, "ciel-unmarshal/getflo: invalid integer found in %v", fd);
    }
#ifdef WORDS_BIGENDIAN
    /* Don't know if this will work; I don't have access to a big-endian
       machine.  If you have one, confirm that this works or fix it and
       get a patch back to me. -- dido */
    u.bytes[7-i] = FIX2INT(ch);
#else
    u.bytes[i] = FIX2INT(ch);
#endif
  }
  return(arc_mkflonum(c, u.d));
}

static value getchar(arc *c, value fd)
{
  Rune r;
  value v;
  int i;

  r = 0;
  for (i=0; i<4; i++) {
    v = arc_readb(c, fd);
    if (FIX2INT(v) < 0)
      c->signal_error(c, "ciel-unmarshal/getchar: unexpected end of file from %v", fd);
    r |= (FIX2INT(v) & 0xff) << (i*8);
  }
  return(arc_mkchar(c, r));
}

static value getstr(arc *c, value fd)
{
  Rune r;
  value str, length;
  int i;

  length = getint(c, 1, fd);
  if (!FIXNUM_P(length)) {
    /* XXX - once we have neat things like ropes as strings we'll be
       able to support strings of arbitrary length limited only by
       how much memory one has. */
    c->signal_error(c, "ciel-unmarshal/getstr: only fixnum lengths are presently allowed for strings", fd);
    return(CNIL);
  }
  str = arc_mkstringlen(c, FIX2INT(length));
  for (i=0; i<FIX2INT(length); i++) {
    r = arc_readc_rune(c, fd);
    if (r == Runeerror) {
      c->signal_error(c, "ciel-unmarshal/getstr: error decoding UTF-8 characters", fd);
      return(CNIL);
    }
    arc_strsetindex(c, str, i, r);
  }
  return(str);
}

static value getbstr(arc *c, value fd)
{
  Rune r;
  value str, length;
  int i;

  length = getint(c, 1, fd);
  if (!FIXNUM_P(length)) {
    /* XXX - once we have neat things like ropes as strings we'll be
       able to support strings of arbitrary length limited only by
       how much memory one has. */
    c->signal_error(c, "ciel-unmarshal/getbstr: only fixnum lengths are presently allowed for binary strings", fd);
    return(CNIL);
  }
  str = arc_mkvmcode(c, FIX2INT(length));
  for (i=0; i<FIX2INT(length); i++) {
    r = arc_readb(c, fd);
    if (r < 0) {
      c->signal_error(c, "ciel-unmarshal/getbstr: unexpected end of file encountered", fd);
      return(CNIL);
    }
    VINDEX(str, i) = FIX2INT(r);
  }
  return(str);
}


static value fill_code(arc *c, value cctx, value bytecode, value lits)
{
  int i, j, k;
  value list, code;

  /* Read the bytecode string and generate code based on it */
  for (i=0; i<VECLEN(bytecode);) {
    int bc, nargs;
    value args[3];

    bc = VINDEX(bytecode, i++);
    nargs = (bc & 0xc0) >> 6;
    for (j=0; j<nargs; j++) {
      int sign = (VINDEX(bytecode, i++) == '-') ? -1 : 1, pos = 0;
      value acc = INT2FIX(0);

      for (k=0; k<7; k++) {
	acc |= (VINDEX(bytecode, i++) & 0xff) << pos;
	pos += 8;
      }
      acc |= VINDEX(bytecode, i++) & 0x7f << pos;
      acc *= sign;
      args[j] = acc;
    }
    switch (nargs) {
    case 0:
      arc_gcode(c, cctx, bc);
      break;
    case 1:
      arc_gcode1(c, cctx, bc, args[0]);
      break;
    case 2:
      arc_gcode2(c, cctx, bc, args[0], args[1]);
      break;
    default:
      c->signal_error(c, "invalid number of arguments for bytecode %d", bc);
      break;
    }
  }
  /* Traverse the literal list and use it to fill the literals
     list in the cctx */
  for (list = lits, i=0; !NIL_P(list); list = cdr(list))
    VINDEX(CCTX_LITS(cctx), i++) = car(list);
  /* XXX - we need to find a way to fill the function name and arguments
     here, either that or dispense with them entirely */
  code = arc_mkcode(c, CCTX_VCODE(cctx), arc_mkstringc(c, ""), CNIL,
		    VECLEN(CCTX_LITS(cctx)));
  for (i=0; i<VECLEN(CCTX_LITS(cctx)); i++)
    CODE_LITERAL(code, i) = VINDEX(CCTX_LITS(cctx), i);
  return(code);
}

/* Read a CIEL 0.0.0 file */
value arc_ciel_unmarshal(arc *c, value fd)
{
  static int header[] = { 0xc1, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  int i, flag, bc;
  value stack, memo, t;
  int memosize, stacksize, stackptr;

  /* Verify the header */
  flag = 1;
  for (i=0; i<8; i++) {
    if (FIX2INT(arc_readb(c, fd)) != header[i]) {
      flag = 0;
      break;
    }
  }
  if (!flag)
    c->signal_error(c, "ciel-unmarshal: invalid header found in file %v", fd);
  /* Initialize stack and memo */
  stacksize = INIT_STACK_SIZE;
  memosize = INIT_MEMO_SIZE;
  stack = arc_mkvector(c, stacksize);
  memo = arc_mkvector(c, memosize);
  stackptr = 0;
  /* Now, start reading the file and interpreting the CIEL bytecode */
  while ((bc = FIX2INT(arc_readb(c, fd))) >= 0) {
    switch (bc) {
    case GNIL:
      PUSH(CNIL);
      break;
    case GTRUE:
      PUSH(CTRUE);
      break;
    case GINT:
      PUSH(getint(c, getsign(c, fd), fd));
      break;
    case GFLO:
      PUSH(getflo(c, fd));
      break;
    case GCHAR:
      PUSH(getchar(c, fd));
      break;
    case GSTR:
      PUSH(getstr(c, fd));
      break;
    case GSYM: {
      value t = getstr(c, fd);

      PUSH(arc_intern(c, t));
      break;
    }
    case GBSTR:
      PUSH(getbstr(c, fd));
      break;
    case CRAT: {
      value x, y;

      x = POP();
      y = POP();
      PUSH(__arc_div2(c, x, y));
      break;
    }
    case CCOMPLEX: {
      value x, y;

      x = POP();
      y = POP();
      PUSH(arc_mkcomplex(c, arc_coerce_flonum(c, x),
			 arc_coerce_flonum(c, y)));
      break;
    }
    case CCONS: {
      value x, y;

      x = POP();
      y = POP();
      PUSH(cons(c, x, y));
      break;
    }
    case CANNOTATE: {
      /* This should have more functionality later, but for now we will
	 limit it to creating a code object from a cons of a binary string
	 (of bytecode) and a list (of literals).  It will expect at top of
	 stack a symbol 'code and below it the cons to be annotated.  Later on
	 we will provide other types of annotations. */
      value x, cc, code, lits, cctx;

      x = POP();		/* should be 'code */
      cc = POP();		/* should be a cons cell */
      if (x != arc_intern_cstr(c, "code")) {
	c->signal_error(c, "CANNOTATE only supports code annotations");
	return(CNIL);
      }

      if (!CONS_P(cc)) {
	c->signal_error(c, "Malformed code annotation, cons expected, type %d object found", TYPE(cc));
	return(CNIL);
      }

      code = cdr(cc);
      if (TYPE(code) != T_VMCODE) {
	c->signal_error(c, "Malformed code annotation, non-string (type %d) found for bytecode", TYPE(code));
	return(CNIL);
      }
      lits = car(cc);
      if (!(CONS_P(lits) || NIL_P(lits))) {
	c->signal_error(c, "Malformed code annotation, non-list (type %d) found for literals", TYPE(lits));
	return(CNIL);
      }
      cctx = arc_mkcctx(c, arc_strlen(c, code), arc_list_length(c, lits));
      PUSH(fill_code(c, cctx, code, lits));
      break;
    }
    case XDUP: {
      value val = VINDEX(stack, stackptr-1);
      PUSH(val);
      break;
    }
    case XMST: {
      value index = getint(c, 1, fd);

      VINDEX(memo, FIX2INT(index)) = POP();
      break;
    }
    case XMLD: {
      value index = getint(c, 1, fd);

      PUSH(VINDEX(memo, FIX2INT(index)));
      break;
    }
    default:
      c->signal_error(c, "Invalid CIEL opcode: %d", bc);
    }
  }
  t = POP();
  return(t);
}
