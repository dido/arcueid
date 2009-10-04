/* 
  Copyright (C) 2009 Rafael R. Sevilla

  This file is part of CArc

  CArc is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA
  02110-1301 USA.
*/
#include <stdlib.h>
#include "carc.h"
#include "vmengine.h"
#include "alloc.h"

#define NAME(_x)
#define vm_Cell2i(_cell, x) ((x)=(long)_cell)
#define IMM_ARG(access, value) (access)
#define SUPER_END

#ifdef __GNUC__

/* direct threading scheme 5: early fetching (Alpha, MIPS) */
#define NEXT_P0	({cfa=&code[c->pc];})
#define IP		(code[c->pc])
#define SET_IP(p)	({c->pc=(p); NEXT_P0;})
#define NEXT_INST	(cfa)
#define INC_IP(const_inc)	({cfa=&code[c->pc + const_inc]; c->pc+=(const_inc);})
#define DEF_CA
#define NEXT_P1	(c->pc++)
#define NEXT_P2	({if (--quanta <= 0) return; goto *cfa;})

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
#define IP              code[c->pc]
#define NEXT_INST	(*code[c->pc])
#define INC_IP(const_inc)	(c->pc+=(const_inc))
#define IPTOS NEXT_INST
#define INST_ADDR(name) I_##name
#define LABEL(name) case I_##name:

enum {
#include "mini-labels.i"
};

#endif /* !defined(__GNUC__) */

Inst *vm_prim;

void carc_vmengine(carc *c, Inst *code, int quanta)
{
  static Label labels[] = {
#include "carcvm-labels.i"
  };
  register Inst *cfa;

  if (code == NULL) {
    vm_prim = labels;
    return;
  }
#ifdef __GNUC__
  NEXT;
#include "carcvm-vm.i"
#else
 next_inst:
  switch (code[c->pc]) {
#include "carcvm-vm.i"
  default:
    fprintf(stderr,"unknown instruction at %d\n", c->pc);
    exit(1);
  }
#endif
}
