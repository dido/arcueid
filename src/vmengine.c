/* 
  Copyright (C) 2009 Rafael R. Sevilla

  This file is part of CArc

  CArc is free software; you can redistribute it and/or modify it
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
#include "carc.h"
#include "vmengine.h"
#include "alloc.h"
#include "arith.h"

int vm_debug = 0;
FILE *vm_out;

#undef VM_DEBUG

#ifdef VM_DEBUG
#define NAME(_x) if (vm_debug) {fprintf(vm_out, "%p: %-20s, ", ip-1, _x); fprintf(vm_out,"sp=%p", sp);}
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
#define NEXT_P0	({cfa=*ip;})
#define IP		(ip)
#define SET_IP(p)	({ip=(p); NEXT_P0;})
#define NEXT_INST	(cfa)
#define INC_IP(const_inc)	({cfa=IP[const_inc]; ip+=(const_inc);})
#define DEF_CA
#define NEXT_P1	(ip++)
#define NEXT_P2	({if (--quanta <= 0 || TSTATE(thr) != Tready) goto endquantum; goto *cfa;})

#define NEXT ({DEF_CA NEXT_P1; NEXT_P2;})
#define IPTOS NEXT_INST

#define INST_ADDR(name) (Label)&&I_##name
#define LABEL(name) I_##name:
#define LABEL2(x)

#else /* !defined(__GNUC__) */

/* use switch dispatch */
#define DEF_CA
#define NEXT_P0
#define NEXT_P1
#define NEXT_P2 goto next_inst;
#define IP              code[t->ip]
#define NEXT_INST	(*code[t->ip])
#define INC_IP(const_inc)	(t->ip+=(const_inc))
#define IPTOS NEXT_INST
#define INST_ADDR(name) I_##name
#define LABEL(name) case I_##name:

enum {
#include "mini-labels.i"
};

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

void carc_vmengine(carc *c, value thr, int quanta)
{
  value *sp;
  Inst *ip, *ip0;
  value code;
  static Label labels[] = {
#include "carcvm-labels.i"
  };
  register Inst *cfa;

  if (thr == CNIL) {
    vm_prim = labels;
    vm_out = stdout;
    return;
  }

 restart:

  if (TSTATE(thr) != Tready)
    return;

  code = VINDEX(TFUNR(thr), 0);
  ip0 = (Inst *)&VINDEX(code, TIP(thr));
  sp = TSP(thr);

  SET_IP(ip0);

#ifdef __GNUC__
  NEXT;
#include "carcvm-vm.i"
#else
 next_inst:
  switch (*ip++) {
#include "carcvm-vm.i"
  default:
    fprintof(stderr,"unknown instruction %d at %p\n", ip[-1], ip-1);
    exit(1);
  }
#endif
 endquantum:
  /* save values of stack pointer and instruction pointer to the
     thread after the quantum of execution finishes. */
  TSP(thr) = sp;
  TIP(thr) = ip - ip0;
}

value carc_mkthread(carc *c, value funptr, int stksize, int ip)
{
  value thr;

  thr = c->get_cell(c);
  BTYPE(thr) = T_THREAD;
  TSTACK(thr) = carc_mkvector(c, stksize);
  TSBASE(thr) = &VINDEX(TSTACK(thr), 0);
  TSP(thr) = TSTOP(thr) = &VINDEX(TSTACK(thr), stksize-1);
  TIP(thr) = ip;
  TSTATE(thr) = Tready;  
  TFUNR(thr) = funptr;
  TENVR(thr) = TVALR(thr) = CNIL;
  return(thr);
}

void carc_apply(carc *c, value thr, value fun)
{
  value argv;
  int argc;

  switch (TYPE(TVALR(thr))) {
  case T_CODE:
    TFUNR(thr) = TVALR(thr);
    TIP(thr) = 0;
    break;
  case T_CCODE:
    argv = CNIL;
    argc = TARGC(thr);
    while (--TARGC(thr))
      argv = cons(c, *TSP(thr)--, argv);
    TVALR(thr) = REP(fun).cfunc(argc, argv);
    carc_return(c, thr);
    break;
  case T_CONS:
    if (TARGC(thr) != 1) {
      c->signal_error(c, "list-ref expects 1 argument");
      return;
    }
    argv = *TSP(thr)--;
    if (TYPE(argv) != T_FIXNUM || TYPE(argv) != T_BIGNUM) {
      c->signal_error(c, "list-ref expects non-negative exact integer as argument");
      return;
    }

    if (carc_numcmp(c, argv, INT2FIX(0)) < 0) {
      c->signal_error(c, "list-ref expects non-negative exact integer as argument");
      return;
    }

    while (carc_numcmp(c, argv, INT2FIX(0)) < 0 && TVALR(thr) != CNIL)
      WB(&TVALR(thr), cdr(TVALR(thr)));
    if (TVALR(thr) != CNIL)
      WB(&TVALR(thr), car(TVALR(thr)));
    break;
  default:
    c->signal_error(c, "invalid function application");
  }
}

/* Restore a continuation */
void carc_restorecont(carc *c, value thr, value cont)
{
  int stklen;

  TIP(thr) = VINDEX(cont, 0);
  WB(&TFUNR(thr), VINDEX(cont, 1));
  WB(&TENVR(thr), VINDEX(cont, 2));
  stklen = VECLEN(VINDEX(cont, 3));
  TSP(thr) = TSTOP(thr) + stklen;
  memcpy(TSP(thr), &VINDEX(VINDEX(cont, 3), 0), stklen*sizeof(value));
}

/* Restore the continuation at the head of the continuation register */
void carc_return(carc *c, value thr)
{
  value cont;

  cont = car(TCONR(thr));
  WB(&TCONR(thr), cdr(TCONR(thr)));
  carc_restorecont(c, thr, cont);
}

value carc_mkcont(carc *c, value offset, value thr)
{
  value cont = carc_mkvector(c, 4);
  value savedstk;
  int stklen;

  BTYPE(cont) = T_CONT;
  WB(&VINDEX(cont, 0), offset);
  WB(&VINDEX(cont, 1), TFUNR(thr));
  WB(&VINDEX(cont, 2), TENVR(thr));
  /* Save the used portion of the stack */
  stklen = TSP(thr) - TSTOP(thr);
  savedstk = carc_mkvector(c, stklen);
  memcpy(&VINDEX(savedstk, 0), TSP(thr), stklen*sizeof(value));
  WB(&VINDEX(cont, 3), savedstk);
  return(cont);
}

value carc_mkenv(carc *c, value parent, int size)
{
  value env;

  env = cons(c, carc_mkvector(c, size), parent);
  BTYPE(env) = T_ENV;
  return(env);
}
