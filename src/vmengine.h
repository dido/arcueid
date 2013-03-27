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
  ihlt=20,
  iadd=21,
  isub=22,
  imul=23,
  idiv=24,
  icons=25,
  icar=26,
  icdr=27,
  iscar=28,
  iscdr=29,
  iis=31,
  idup=34,
  icls=35,
  iconsr=36,
  imenv=101,
  idcar=38,
  idcdr=39,
  ispl=40
};

#define CODE_CODE(c) (VINDEX((c), 0))
#define CODE_SRC(c) (VINDEX((c), 1))
#define CODE_LITERAL(c, idx) (VINDEX((c), 2+(idx)))

#define CLOS_CODE(cl) (car(cl))
#define CLOS_ENV(cl) (cdr(cl))

/* The source information is a hash table, whose keys are code offsets
   into the function and whose values are line numbers.  The following
   special negative indexes are used for metadata. */
/* File name of the compiled file */
#define SRC_FILENAME (-1)
/* Function name */
#define SRC_FUNCNAME (-2)

extern void arc_emit(arc *c, value cctx, enum vminst inst);
extern void arc_emit1(arc *c, value cctx, enum vminst inst, value arg);
extern void arc_emit2(arc *c, value cctx, enum vminst inst, value arg1,
		      value arg2);
extern void arc_emit3(arc *c, value cctx, enum vminst inst, value arg1,
		      value arg2, value arg3);
extern int arc_literal(arc *c, value cctx, value literal);
extern value arc_mkcode(arc *c, int ncodes, int nlits);
extern value arc_code_setsrc(arc *c, value code, value src);
extern value arc_cctx2code(arc *c, value cctx);
extern value arc_mkcctx(arc *c);

enum threadstate {
  Talt,				/* blocked in alt instruction */
  Tsend,			/* waiting to send */
  Trecv,			/* waiting to recv */
  Tiowait,			/* I/O wait */
  Tioready,			/* I/O ready to resume */
  Tready,			/* ready to be scheduled */
  Trelease			/* thread released */
};

struct vmthread_t {
  value funr;			/* function pointer register */
  value envr;			/* environment register */
  value valr;			/* value of most recent instruction */
  value conr;			/* continuation register */
  value *spr;			/* stack pointer */
  value stack;			/* actual stack (a vector) */
  value *stkbase;		/* base pointer of the stack */
  value *stktop;		/* top of value stack */
  value *stkfn;			/* start of stack for current function */
  union {
    value *ipptr;		/* instruction pointer */
    int aff_line;		/* line number in an AFF */
  } ip;
  int argc;			/* argument count register */

  enum threadstate state;	/* thread state */
  int tid;			/* thread ID */
  int quanta;			/* time slice */
  unsigned long long ticks;	/* time used */
  unsigned long long wakeuptime; /* wakeup time */
  value waitfd;			 /* file descriptor to wait on */

  value cmarks;			/* continuation marks */
};

#define TFUNR(t) (((struct vmthread_t *)REP(t))->funr)
#define TENVR(t) (((struct vmthread_t *)REP(t))->envr)
#define TVALR(t) (((struct vmthread_t *)REP(t))->valr)
#define TCONR(t) (((struct vmthread_t *)REP(t))->conr)
#define TSP(t) (((struct vmthread_t *)REP(t))->spr)
#define TSTACK(t) (((struct vmthread_t *)REP(t))->stack)
#define TSBASE(t) (((struct vmthread_t *)REP(t))->stkbase)
#define TSFN(t) (((struct vmthread_t *)REP(t))->stkfn)
#define TSTOP(t) (((struct vmthread_t *)REP(t))->stktop)
#define TIP(t) (((struct vmthread_t *)REP(t))->ip)
#define TIPP(t) (((struct vmthread_t *)REP(t))->ip.ipptr)
#define TARGC(t) (((struct vmthread_t *)REP(t))->argc)

#define TSTATE(t) (((struct vmthread_t *)REP(t))->state)
#define TTID(t) (((struct vmthread_t *)REP(t))->tid)
#define TQUANTA(t) (((struct vmthread_t *)REP(t))->quanta)
#define TTICKS(t) (((struct vmthread_t *)REP(t))->ticks)
#define TWAKEUP(t) (((struct vmthread_t *)REP(t))->wakeuptime)
#define TWAITFD(t) (((struct vmthread_t *)REP(t))->waitfd)
#define TCM(t) (((struct vmthread_t *)REP(t))->cmarks)

#define CPUSH(thr, val) (*(TSP(thr)--) = (val))
#define CPOP(thr) (*(++TSP(thr)))
/* Default thread stack size */
#define TSTKSIZE 65536

/* A code generation context (cctx) is a vector with the following
   items as indexes:

   0. A code pointer into the vmcode object (usually a fixnum)
   1. A vmcode object.
   2. A pointer into the literal vector (usually a fixnum)
   3. A vector of literals

   The following macros are intended to manage the data
   structure, and to generate code and literals for the
   system.
 */

#define CCTX_VCPTR(cctx) (VINDEX(cctx, 0))
#define CCTX_VCODE(cctx) (VINDEX(cctx, 1))
#define CCTX_LPTR(cctx) (VINDEX(cctx, 2))
#define CCTX_LITS(cctx) (VINDEX(cctx, 3))

/* Continuations are vectors with the following items as indexes:

   0. Offset into the code object (or internal label in an AFF)
   1. Function
   2. Environment
   3. Argument count
   4. Old value of continuation register
   5. Vector of saved stack values
 */
#define CONT_OFS(cont) (VINDEX(cont, 0))
#define CONT_FUN(cont) (VINDEX(cont, 1))
#define CONT_ENV(cont) (VINDEX(cont, 2))
#define CONT_ARGC(cont) (VINDEX(cont, 3))
#define CONT_CONT(cont) (VINDEX(cont, 4))
#define CONT_STK(cont) (VINDEX(cont, 5))

#define CONT_SIZE 6

extern void arc_jmpoffset(arc *c, value cctx, int jmpinst, int destoffset);

extern void __arc_thr_trampoline(arc *c, value thr, enum tr_states_t result);
extern int __arc_resume_aff(arc *c, value thr);
extern void arc_restorecont(arc *c, value thr, value cont);
extern int __arc_vmengine(arc *c, value thr);

extern void __arc_clos_env2heap(arc *c, value thr, value clos);

extern value __arc_env2heap(arc *c, value thr, value env);
extern void __arc_menv(arc *c, value thr, int n);

extern void __arc_update_cont_envs(arc *c, value thr, value oldenv, value nenv);
extern value __arc_cont2heap(arc *c, value thr, value cont);

#endif
