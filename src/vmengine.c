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
#include <stdlib.h>
#include "carc.h"
#include "vmengine.h"
#include "alloc.h"

/* do not use spTOS */
#define IF_spTOS(x)

#define NAME(_x)
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
#define INC_IP(const_inc)	({cfa=ip[const_inc]; ip += (const_inc);})
#define DEF_CA
#define NEXT_P1	(ip++)
#define NEXT_P2	({if (--quanta <= 0) goto endquantum; goto *cfa;})

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

Inst *vm_prim;

void carc_vmengine(carc *c, value thr, int quanta)
{
  struct vmthread *t;
  value *sp;
  Inst *ip, *ip0;
  value code;

#define spTOS (sp[0])

  t = &REP(thr)._thread;

  code = VINDEX(t->funr, 0);
  ip0 = ip = (Inst *)&VINDEX(code, t->ip);
  sp = t->sp;
  static Label labels[] = {
#include "carcvm-labels.i"
  };
  register Inst *cfa;

  if (thr == CNIL) {
    vm_prim = labels;
    return;
  }
#ifdef __GNUC__
  NEXT;
#include "carcvm-vm.i"
#else
 next_inst:
  switch (*ip++) {
#include "carcvm-vm.i"
  default:
    fprintf(stderr,"unknown instruction %d at %p\n", ip[-1], ip-1);
    exit(1);
  }
#endif
 endquantum:
  /* save values of stack pointer and instruction pointer to the
     thread after the quantum of execution finishes. */
  t->sp = sp;
  t->ip = ip - ip0;
}
