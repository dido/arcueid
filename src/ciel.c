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

#define PUSH(val) (stackpush(c, &stack, (val), &stackptr, &stacksize))

static int getsign(arc *c, value fd)
{
  return((FIX2INT(arc_readb(c, fd)) == '-') ? -1 : 1);
}

static value getint(arc *c, int sign, value fd)
{
  value acc = INT2FIX(0), parts[8];
  int i, last;

  /* CIEL stores integers in 64 bit chunks, little endian.  We rearrange
     the calculations here such that the only value that can possibly be
     a bignum is acc.
     XXX - we should probably make a smarter algorithm that takes
     advantage of 64-bit architectures. */
  for (;;) {
    for (i=0; i<8; i++)
      parts[i] = arc_readb(c, fd);
    /* check if this is the last one */
    last = (FIX2INT(parts[7]) & 0x80) != 0;
    /* mask out the high bit if needed */
    parts[7] = INT2FIX(FIX2INT(parts[7]) & 0x7f);
    /* add in the highest portion first, this is 63 bits so
       we multiply by 2^7 only. */
    acc = __arc_mul2(c, acc, INT2FIX(128));
    acc = __arc_add2(c, acc, parts[7]);
    for (i=6; i>=0; i--) {
      acc = __arc_mul2(c, acc, INT2FIX(256));
      acc = __arc_add2(c, acc, parts[i]);
    }
    /* stop if this is the end */
    if (last)
      break;
  }
  /* add the sign */
  acc = __arc_mul2(c, acc, INT2FIX(sign));
  return(acc);
}

/* Read a CIEL 0.0.0 file */
value arc_ciel_unmarshal_v000(arc *c, value fd)
{
  static char header[] = { 0xc1, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  int i, flag, bc;
  value stack, memo;
  int memosize, stacksize, stackptr;

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
    default:
      c->signal_error(c, "Invalid CIEL opcode: %d", bc);
    }
  }
}
