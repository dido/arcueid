/* 
  Copyright (C) 2010 Rafael R. Sevilla

  This file is part of Arcueid

  Arcueid is free software: you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library. If not, see <http://www.gnu.org/licenses/>
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../config.h"
#include "arcueid.h"
#include "vmengine.h"
#include "alloc.h"
#include "arith.h"

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
#ifndef alloca
# define alloca __builtin_alloca
#endif
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# include <stddef.h>
void *alloca (size_t);
#endif

/* instruction decoding macros */
#ifdef HAVE_THREADED_INTERPRETER
/* threaded interpreter */
#define INST(name) lbl_##name
#define JTBASE ((void *)&&lbl_inop)
#define NEXT {							\
    if (--quanta <= 0 || TSTATE(thr) != Tready)			\
      goto endquantum;						\
    goto *(JTBASE + jumptbl[*TIP(thr)++]); }
#else
/* switch interpreter */
#define INST(name) case name
#define NEXT break
#endif

#define CPUSH(thr, val) (*(TSP(thr)--) = (val))
#define CPOP(thr) (*(++TSP(thr)))

void arc_vmengine(arc *c, value thr, int quanta)
{
#ifdef HAVE_THREADED_INTERPRETER
  static const int jumptbl[] = {
#include "jumptbl.h"
  };
#else
  value curr_instr;
#endif
  if (TSTATE(thr) != Tready)
    return;
  c->curthread = thr;

#ifdef HAVE_THREADED_INTERPRETER
  goto *(void *)(JTBASE + jumptbl[*TIP(thr)++]);
#else
  for (;;) {
    if (--quanta <= 0 || TSTATE(thr) != Tready)
      goto endquantum;
    curr_instr = *TIP(thr)++;
    switch (curr_instr) {
#endif
    INST(inop):
      NEXT;
    INST(ipush):
      CPUSH(thr, TVALR(thr));
      NEXT;
    INST(ipop):
      TVALR(thr) = CPOP(thr);
      NEXT;
    INST(ildi):
      TVALR(thr) = *TIP(thr)++;
      NEXT;
    INST(ildl):
      TVALR(thr) = CODE_LITERAL(TFUNR(thr), *TIP(thr)++);
      NEXT;
    INST(ildg):
      {
	value tmp;
	tmp = CODE_LITERAL(TFUNR(thr), *TIP(thr)++);
	if ((TVALR(thr) = arc_hash_lookup(c, c->genv, tmp)) == CUNBOUND) {
	  c->signal_error(c, "Unbound symbol %S\n", tmp);
	  TVALR(thr) = CNIL;
	}
      }
      NEXT;
    INST(istg):
      arc_hash_insert(c, c->genv, CODE_LITERAL(TFUNR(thr), *TIP(thr)++),
		      TVALR(thr));
      NEXT;
    INST(ilde):
      {
	value iindx, tmp;
	int ienv;

	ienv = (int)*TIP(thr)++;
	iindx = *TIP(thr)++;
	tmp = TENVR(thr);
	while (--ienv >= 0)
	  tmp = cdr(tmp);
	TVALR(thr) = ENV_VALUE(car(tmp), iindx);
      }
      NEXT;
    INST(iste):
      {
	value iindx, tmp;
	int ienv;

	ienv = (int)*TIP(thr)++;
	iindx = *TIP(thr)++;
	tmp = TENVR(thr);
	while (--ienv >= 0)
	  tmp = cdr(tmp);
	WB(&ENV_VALUE(car(tmp), iindx), TVALR(thr));
      }
      NEXT;
    INST(imvarg):
      {
	int iindx = (int)*TIP(thr)++;
	if (TSP(thr) == TSTOP(thr))
	  c->signal_error(c, "too few arguments");
	WB(&ENV_VALUE(car(TENVR(thr)), iindx), CPOP(thr));
      }
      NEXT;
    INST(imvoarg):
      {
	int iindx = (int)*TIP(thr)++;
	value arg = (TSP(thr) == TSTOP(thr)) ? CNIL : CPOP(thr);

	WB(&ENV_VALUE(car(TENVR(thr)), iindx), arg);
      }
      NEXT;
    INST(imvrarg):
      {
	int iindx = (int)*TIP(thr)++;
	value list = CNIL, i;

	while (TSP(thr) != TSTOP(thr)) {
	  i = CPOP(thr);
	  list = cons(c, i, list);
	}
	WB(&ENV_VALUE(car(TENVR(thr)), iindx), list);
      }
      NEXT;
    INST(icont):
      {
	int icofs = (int)*TIP(thr)++;
	WB(&TCONR(thr), cons(c, arc_mkcont(c, INT2FIX(icofs), thr),
			     TCONR(thr)));
      }
      NEXT;
    INST(ienv):
      {
	value ienvsize = *TIP(thr)++;

	WB(&TENVR(thr), arc_mkenv(c, TENVR(thr), ienvsize));
	ENV_NAMES(car(TENVR(thr))) = CODE_NAME(TFUNR(thr));
	WB(&VINDEX(car(TENVR(thr)), 0), VINDEX(TFUNR(thr), 2));
      }
      NEXT;
    INST(iapply):
      {
	int argc = *TIP(thr)++;
	TARGC(thr) = argc;
	arc_apply(c, thr, TVALR(thr));
      }
      NEXT;
    INST(iret):
      arc_return(c, thr);
      NEXT;
    INST(ijmp):
      {
	int itarget = *TIP(thr)++;
	TIP(thr) += itarget-2;
      }
      NEXT;
    INST(ijt):
      {
	int itarget = *TIP(thr)++;
	if (TVALR(thr) != CNIL)
	  TIP(thr) += itarget-2;
      }
      NEXT;
    INST(ijf):
      {
	int itarget = *TIP(thr)++;
	if (TVALR(thr) == CNIL)
	  TIP(thr) += itarget-2;
      }
      NEXT;
    INST(itrue):
      TVALR(thr) = CTRUE;
      NEXT;
    INST(inil):
      TVALR(thr) = CNIL;
      NEXT;
    INST(ihlt):
      TSTATE(thr) = Trelease;
      goto endquantum;
      NEXT;
    INST(iadd):
      TVALR(thr) = __arc_add2(c, TVALR(thr), CPOP(thr));
      NEXT;
    INST(isub):
      TVALR(thr) = __arc_sub2(c, TVALR(thr), CPOP(thr));
      NEXT;
    INST(imul):
      TVALR(thr) = __arc_mul2(c, TVALR(thr), CPOP(thr));
      NEXT;
    INST(idiv):
      TVALR(thr) = __arc_div2(c, TVALR(thr), CPOP(thr));
      NEXT;
    INST(icons):
      TVALR(thr) = cons(c, TVALR(thr), CPOP(thr));
      NEXT;
    INST(icar):
      TVALR(thr) = car(TVALR(thr));
      NEXT;
    INST(icdr):
      TVALR(thr) = cdr(TVALR(thr));
      NEXT;
    INST(iscar):
      scar(TVALR(thr), CPOP(thr));
      NEXT;
    INST(iscdr):
      scdr(TVALR(thr), CPOP(thr));
      NEXT;
    INST(ispl):
      {
	value list = TVALR(thr), nlist = CPOP(thr);
	/* Find the first cons in list whose cdr is not itself a cons.
	   Join the list from the stack to it. */
	for (;;) {
	  if (!CONS_P(cdr(list))) {
	    if (cdr(list) == CNIL)
	      scdr(list, nlist);
	    else
	      c->signal_error(c, "splicing improper list");
	    break;
	  }
	  list = cdr(list);
	}
      }
      NEXT;
    INST(iis):
      TVALR(thr) = arc_is(c, TVALR(thr), CPOP(thr));
      NEXT;
    INST(iiso):
      TVALR(thr) = arc_iso(c, TVALR(thr), CPOP(thr));
      NEXT;
    INST(igt):
      TVALR(thr) = (FIX2INT(arc_cmp(c, TVALR(thr), CPOP(thr))) > 0) ?
	CTRUE : CNIL;
      NEXT;
    INST(ilt):
      TVALR(thr) = (FIX2INT(arc_cmp(c, TVALR(thr), CPOP(thr))) < 0) ?
	CTRUE : CNIL;
      NEXT;
    INST(idup):
      TVALR(thr) = *(TSP(thr)+1);
      NEXT;
    INST(icls):
      TVALR(thr) = arc_mkclosure(c, TVALR(thr), TENVR(thr));
      NEXT;
    INST(iconsr):
      TVALR(thr) = cons(c, CPOP(thr), TVALR(thr));
      NEXT;
#ifndef HAVE_THREADED_INTERPRETER
    default:
#else
    INST(invalid):
#endif
      c->signal_error(c, "invalid opcode %02x", *TIP(thr));
#ifdef HAVE_THREADED_INTERPRETER
#else
    }
  }
#endif
 endquantum:
  return;
}

value arc_mkthread(arc *c, value funptr, int stksize, int ip)
{
  value thr;
  value code;

  thr = c->get_cell(c);
  BTYPE(thr) = T_THREAD;
  TSTACK(thr) = arc_mkvector(c, stksize);
  TSBASE(thr) = &VINDEX(TSTACK(thr), 0);
  TSP(thr) = TSTOP(thr) = &VINDEX(TSTACK(thr), stksize-1);
  TSTATE(thr) = Tready;
  TFUNR(thr) = funptr;
  code = VINDEX(TFUNR(thr), 0);
  TIP(thr) = &VINDEX(code, ip);
  TENVR(thr) = TVALR(thr) = CNIL;
  return(thr);
}

void arc_apply(arc *c, value thr, value fun)
{
  value *argv, cl, cfn, avec;
  int argc, i;

  switch (TYPE(TVALR(thr))) {
  case T_CLOS:
    cl = TVALR(thr);
    WB(&TFUNR(thr), car(cl));
    WB(&TENVR(thr), cdr(cl));
    TIP(thr) = &VINDEX(VINDEX(TFUNR(thr), 0), 0);
    break;
  case T_CCODE:
    cfn = TVALR(thr);
    argc = TARGC(thr);
    if (REP(cfn)._cfunc.argc >= 0 && REP(cfn)._cfunc.argc != argc) {
      c->signal_error(c, "wrong number of arguments (%d for %d)\n", argc,
		      REP(cfn)._cfunc.argc);
      return;
    }
    argv = alloca(sizeof(value)*argc);
    for (i=0; i<argc; i++)
      argv[i] = CPOP(thr);
    switch (REP(cfn)._cfunc.argc) {
    case -2:
      avec = arc_mkvector(c, argc);
      memcpy(&VINDEX(avec, 0), argv, sizeof(value)*argc);
      TVALR(thr) = REP(cfn)._cfunc.fnptr(c, avec);
      break;
    case -1:
      TVALR(thr) = REP(cfn)._cfunc.fnptr(c, argc, argv);
      break;
    case 0:
      TVALR(thr) = REP(cfn)._cfunc.fnptr(c);
      break;
    case 1:
      TVALR(thr) = REP(cfn)._cfunc.fnptr(c, argv[0]);
      break;
    case 2:
      TVALR(thr) = REP(cfn)._cfunc.fnptr(c, argv[0], argv[1]);
      break;
    case 3:
      TVALR(thr) = REP(cfn)._cfunc.fnptr(c, argv[0], argv[1], argv[2]);
      break;
    case 4:
      TVALR(thr) = REP(cfn)._cfunc.fnptr(c, argv[0], argv[1], argv[2],
					 argv[3]);
      break;
    case 5:
      TVALR(thr) = REP(cfn)._cfunc.fnptr(c, argv[0], argv[1], argv[2],
					 argv[3], argv[4]);
      break;
    case 6:
      TVALR(thr) = REP(cfn)._cfunc.fnptr(c, argv[0], argv[1], argv[2],
					 argv[3], argv[4], argv[5]);
      break;
    case 7:
      TVALR(thr) = REP(cfn)._cfunc.fnptr(c, argv[0], argv[1], argv[2],
					 argv[3], argv[4], argv[5],
					 argv[6]);
      break;
    case 8:
      TVALR(thr) = REP(cfn)._cfunc.fnptr(c, argv[0], argv[1], argv[2],
					 argv[3], argv[4], argv[5],
					 argv[6], argv[7]);
      break;
    default:
      c->signal_error(c, "too many arguments");
      return;
    }
    arc_return(c, thr);
    break;
  case T_CONS:
    if (TARGC(thr) != 1) {
      c->signal_error(c, "list application expects 1 argument, given %d",
		      INT2FIX(TARGC(thr)));
      TVALR(thr) = CNIL;
    } else {
      value count = CPOP(thr), ocount, cval;

      ocount = count;
      if ((FIXNUM_P(count) || TYPE(count) == T_BIGNUM) &&
	  (cval = arc_cmp(c, count, INT2FIX(0))) != INT2FIX(-1)) {
	value list=TVALR(thr), res;
	/* We now have a non-negative exact integer for the count */
	do {
	  if (!CONS_P(list)) {
	    c->signal_error(c, "index %d too large for list", ocount);
	    res = CNIL;
	    break;
	  }
	  res = car(list);
	  list = cdr(list);
	  count = __arc_sub2(c, count, INT2FIX(1));
	} while ((cval = arc_cmp(c, count, INT2FIX(0))) != INT2FIX(-1));
	TVALR(thr) = res;
      } else {
	c->signal_error(c, "list application expects non-negative exact integer argument, given object of type %d", INT2FIX(TYPE(count)));
	TVALR(thr) = CNIL;
      }
    }
    arc_return(c, thr);
    break;
  case T_VECTOR:
    if (TARGC(thr) != 1) {
      c->signal_error(c, "vector application expects 1 argument, given %d",
		      INT2FIX(TARGC(thr)));
      TVALR(thr) = CNIL;
    } else {
      value count = CPOP(thr), ocount, cval;

      ocount = count;
      if (FIXNUM_P(count) &&
	  (cval = arc_cmp(c, count, INT2FIX(0))) != INT2FIX(-1)) {
	value vec=TVALR(thr), res;
	/* We now have a non-negative exact integer for the count/index */
	if (FIX2INT(count) >= VECLEN(vec)) {
	  c->signal_error(c, "index %d too large for vector", ocount);
	  res = CNIL;
	} else {
	  res = VINDEX(vec, FIX2INT(count));
	}
	TVALR(thr) = res;
      } else {
	/* XXX - Permit negative indices?  Could be useful. */
	c->signal_error(c, "vector application expects non-negative fixnum argument, given object of type %d", INT2FIX(TYPE(count)));
	TVALR(thr) = CNIL;
      }
    }
    arc_return(c, thr);
    break;
  case T_TABLE:
    if (TARGC(thr) != 1 && TARGC(thr) != 2) {
      c->signal_error(c, "table application expects 1 or 2 arguments, given %d",
		      INT2FIX(TARGC(thr)));
      TVALR(thr) = CNIL;
    } else {
      value tbl, key, dflt, bind;

      tbl = TVALR(thr);
      key = CPOP(thr);
      dflt = (TARGC(thr) == 1) ? CNIL : CPOP(thr);
      bind = arc_hash_lookup(c, tbl, key);
      TVALR(thr) = (bind == CUNBOUND) ? dflt : bind;
    }
    arc_return(c, thr);
    break;
  case T_STRING:
    if (TARGC(thr) != 1) {
      c->signal_error(c, "string application expects 1 argument, given %d",
		      INT2FIX(TARGC(thr)));
      TVALR(thr) = CNIL;
    } else {
      value count = CPOP(thr), ocount, cval;

      ocount = count;
      if (FIXNUM_P(count) &&
	  (cval = arc_cmp(c, count, INT2FIX(0))) != INT2FIX(-1)) {
	value str=TVALR(thr), res;
	/* We now have a non-negative exact integer for the count/index.
	   XXX - string length is always a fixnum? */
	if (FIX2INT(count) >= arc_strlen(c, str)) {
	  c->signal_error(c, "index %d too large for string", ocount);
	  res = CNIL;
	} else {
	  res = arc_mkchar(c, arc_strindex(c, str, FIX2INT(count)));
	}
	TVALR(thr) = res;
      } else {
	/* XXX - permit negative indices? Not allowed by reference Arc. */
	c->signal_error(c, "string application expects non-negative fixnum argument, given object of type %d", INT2FIX(TYPE(count)));
	TVALR(thr) = CNIL;
      }
    }
    arc_return(c, thr);
    break;
  default:
    c->signal_error(c, "invalid function application");
  }
}

/* Restore a continuation */
void arc_restorecont(arc *c, value thr, value cont)
{
  int stklen, offset;

  WB(&TFUNR(thr), VINDEX(cont, 1));
  offset = FIX2INT(VINDEX(cont, 0));
  TIP(thr) = &VINDEX(VINDEX(TFUNR(thr), 0), offset);
  WB(&TENVR(thr), VINDEX(cont, 2));
  stklen = VECLEN(VINDEX(cont, 3));
  TSP(thr) = TSTOP(thr) - stklen;
  memcpy(TSP(thr), &VINDEX(VINDEX(cont, 3), 0), stklen*sizeof(value));
}

/* Restore the continuation at the head of the continuation register */
void arc_return(arc *c, value thr)
{
  value cont;

  cont = car(TCONR(thr));
  WB(&TCONR(thr), cdr(TCONR(thr)));
  arc_restorecont(c, thr, cont);
}

value arc_mkcont(arc *c, value offset, value thr)
{
  value cont = arc_mkvector(c, 4);
  value savedstk;
  int stklen;

  BTYPE(cont) = T_CONT;
  WB(&VINDEX(cont, 0), offset);
  WB(&VINDEX(cont, 1), TFUNR(thr));
  WB(&VINDEX(cont, 2), TENVR(thr));
  /* Save the used portion of the stack */
  stklen = TSTOP(thr) - (TSP(thr));
  savedstk = arc_mkvector(c, stklen);
  memcpy(&VINDEX(savedstk, 0), TSP(thr), stklen*sizeof(value));
  WB(&VINDEX(cont, 3), savedstk);
  return(cont);
}

value arc_mkenv(arc *c, value parent, int size)
{
  value env;

  env = cons(c, arc_mkvector(c, size+1), parent);
  BTYPE(env) = T_ENV;
  return(env);
}

value arc_mkclosure(arc *c, value code, value env)
{
  value clos;

  clos = cons(c, code, env);
  BTYPE(clos) = T_CLOS;
  return(clos);
}

/* Main dispatcher.  Will run each thread in the thread list for
   at most quanta cycles or until the thread leaves ready state.
   Also runs garbage collector threads periodically.  This should
   be called with at least one thread already in the run queue.
   Terminates when no more threads can run. */
void arc_thread_dispatch(arc *c, int quanta)
{
  value prev = CNIL, thr;
  c->vmqueue = CNIL;
  int nthreads = 0, blockedthreads = 0, ncycles = 0;

  for (;;) {
    /* loop back to the beginning if we hit the nil at the end */
    if (c->vmqueue == CNIL) {
      /* Simple deadlock detection.  We declare deadlock if after two
	 cycles through the run queue we find that all threads are in
	 a blocked state. */
      if (nthreads != 0 && nthreads == blockedthreads && ++ncycles > 2) {
	c->signal_error(c, "fatal: deadlock detected");
	return;
      } else {
	/* reset cycle counter if we get a state where there are
	   some threads which are not blocked */
	ncycles = 0;
      }
      c->vmqueue = c->vmthreads;
      prev = CNIL;
      /* No more available threads, exit */
      if (c->vmqueue == CNIL)
	return;
      /* reset thread counts */
      nthreads = 0;
      blockedthreads = 0;
    }
    ++nthreads;
    thr = car(c->vmqueue);
    arc_vmengine(c, thr, quanta);
    switch (TSTATE(thr)) {
      /* see if the thread has changed state to Trelease, Texiting, or
       Tbroken.  Remove the thread from the queue if so. */
    case Trelease:
    case Texiting:
    case Tbroken:
      /* dequeue the terminated thread */
      if (prev == CNIL) {
	WB(&c->vmthreads, cdr(c->vmqueue));
      } else {
	scdr(prev, cdr(c->vmqueue));
      }
      break;
    case Talt:
    case Tsend:
    case Trecv:
      /* increment count of threads in blocked states */
      ++blockedthreads;
      break;
    default:
      break;
    }

    /* garbage collect */
    c->rungc(c);

    /* queue next thread */
    prev = c->vmqueue;
    c->vmqueue = cdr(c->vmqueue);
  }
}
