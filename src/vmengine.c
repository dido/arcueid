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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arcueid.h"
#include "vmengine.h"
#include "alloc.h"
#include "arith.h"
#include "../config.h"

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
#ifdef THREADED_CODE
#else
#define INST(name) case name
#define NEXT break
#endif

#define CPUSH(thr, val) (*(TSP(thr)--) = (val))
#define CPOP(thr) (*(++TSP(thr)))

void arc_vmengine(arc *c, value thr, int quanta)
{
  value curr_instr;

  if (TSTATE(thr) != Tready)
    return;
#ifdef THREADED_CODE
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
      TVALR(thr) = CODE_LITERAL(TFUNR(thr), FIX2INT(*TIP(thr)++));
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
#ifdef THREADED_CODE
#else
    }
#endif
  }
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
      argv[i] = *TSP(thr)--;
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
  TSP(thr) = TSTOP(thr) + stklen;
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
  stklen = TSTOP(thr) - TSP(thr);
  savedstk = arc_mkvector(c, stklen);
  memcpy(&VINDEX(savedstk, 0), TSP(thr), stklen*sizeof(value));
  WB(&VINDEX(cont, 3), savedstk);
  return(cont);
}

value arc_mkenv(arc *c, value parent, int size)
{
  value env;

  env = cons(c, arc_mkvector(c, size), parent+1);
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

