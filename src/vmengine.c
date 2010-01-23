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

#define sp (TSP(thr))

#define XTIP(thr) ((Inst *)TIP(thr))
int vm_debug = 0;
FILE *vm_out;

#ifdef VM_DEBUG
#define NAME(_x) if (vm_debug) {fprintf(vm_out, "%p: %-20s, ", XTIP(thr)-1, _x); fprintf(vm_out,"sp=%p", sp);}
#else
#define NAME(_x)
#endif

/* do not use spTOS */
#define IF_spTOS(x)

#define IMM_ARG(access, value) (access)
#define vm_value2i(x,y) (y=x)
#define vm_Cell2i(x,y) (y=(value)x)
#define vm_Cell2target(_cell,x)	((x)=(Inst *)(_cell))
#define vm_i2value(x,y) (y=x)
#define SUPER_END

#ifdef __GNUC__

/* direct threading scheme 5: early fetching (Alpha, MIPS) */
#define NEXT_P0	({cfa=*XTIP(thr);})
#define IP		(XTIP(thr))
#define SET_IP(p)	({TIP(thr)=(value *)(p); NEXT_P0;})
#define NEXT_INST	(cfa)
#define INC_IP(const_inc)	({cfa=IP[const_inc]; TIP(thr)+=(const_inc);})
#define DEF_CA
#define NEXT_P1	(TIP(thr)++)
#define NEXT_P2	({if (--quanta <= 0 || TSTATE(thr) != Tready) goto endquantum; goto *cfa;})

#define NEXT ({DEF_CA NEXT_P1; NEXT_P2;})
#define IPTOS NEXT_INST

#define INST_ADDR(name) (Label)&&I_##name
#define LABEL(name) I_##name:
#define LABEL2(x)

#else /* !defined(__GNUC__) */

#error "FIXME: UNTHREADED INTERPRETER IS NOT YET SUPPORTED"

#endif /* !defined(__GNUC__) */

#define spTOS (sp[0])

Inst *vm_prim;

void printarg_i(value i)
{
  fprintf(vm_out, "%ld ", i);
}

void printarg_target(Inst *target)
{
  fprintf(vm_out, "%p ", target);
}

void arc_vmengine(arc *c, value thr, int quanta)
{
  static Label labels[] = {
#include "arcueid-labels.i"
  };
  register Inst *cfa;

  if (thr == CNIL) {
    vm_prim = labels;
    vm_out = stdout;
    return;
  }

  if (TSTATE(thr) != Tready)
    return;

  SET_IP(TIP(thr));

#ifdef __GNUC__
  NEXT;
#include "arcueid-vm.i"
#else

#error "FIXME: UNTHREADED INTERPRETER IS NOT YET SUPPORTED"

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
