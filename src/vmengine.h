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
#include "arcueid.h"

/*! \enum vminst
    \brief Virtual machine instructions
    This is parsed by scripts to generate some include files. Be
    careful with extraneous text here!

    Virtual machine instructions have the convention that the top two
    bits give the number of operands for the instruction, from 0 to 3,
    thus the env instruction with three operands, is 11001010 in
    binary, with the top two bits 0b11 = 3.
*/
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

/*! \enum threadstate
    \brief Thread states
 */
enum threadstate {
  Talt,				/*!< blocked in alt instruction */
  Tsend,			/*!< waiting to send */
  Trecv,			/*!< waiting to recv */
  Tiowait,			/*!< I/O wait */
  Tsleep,			/*!< sleep */
  Tready,			/*!< ready to be scheduled */
  Tcritical,			/*!< critical section */
  Trelease,			/*!< thread released */
  Tbroken,			/*!< error break */
};

typedef struct {
  struct arc *c;		/*!< Arc context  */
  value func;			/*!< Function register */
  value env;			/*!< Environment register */
  value acc;			/*!< Accumulator */
  value cont;			/*!< Continuation register */
  value *sp;			/*!< Stack pointer */
  value stack;			/*!< Actual stack */
  value *stkbase;		/*!< Base pointer of stack */
  value *stktop;		/*!< Top of value stack */
  int ip;			/*!< Instruction pointer */
  int argc;			/*!< Argument count */
  enum threadstate state;	/*!< Thread state */
  int tid;			/*!< Thread ID */
  long quanta;			/*!< Time slice */
  unsigned long long ticks;	/*!< time used */
  unsigned long long wuptime;	/*!< wakeup time */
  int waitfd;			/*!< file descriptor waited on */
  int waitrw;			/*!< wait on read/write */
  value exh;			/*!< current exception handler */
  jmp_buf errjmp;		/*!< error jump point  */
  value cmarks;			/*!< continuation marks */
  value rvch;			/*!< return value channel */
} arc_thread;

/* \fn value __arc_thread_new(arc *c, int tid)
   \brief Create a new thread
 */
extern value __arc_thread_new(arc *c, int tid);

/* \fn void __arc_thr_trampoline(arc *c, value thr, enum arc_trstate state)
   \brief Thread trampoline
   Resume execution of _thr_ until some condition occurs such that the
   thread is no longer able to continue executing.
 */
extern void __arc_thr_trampoline(arc *c, value thr, enum arc_trstate state);

#endif
