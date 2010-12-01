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
#define GTAB  7			/* not supported */

#define CRAT      8
#define CCOMPLEX  9
#define CTADD     10
#define CCONS     11
#define CANNOTATE 12

#define XDUP 13
#define XMST 14
#define XMLD 15

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

/* Read a CIEL 0.0.0 file */
value arc_ciel_unmarshal(arc *c, value fd)
{
  static int header[] = { 0xc1, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  int i, flag, bc;
  value stack, memo;
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
    case CRAT:
      PUSH(__arc_div2(c, POP(), POP()));
      break;
    default:
      c->signal_error(c, "Invalid CIEL opcode: %d", bc);
    }
  }
  return(POP());
}
