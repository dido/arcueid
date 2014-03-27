/* 
  Copyright (C) 2014 Rafael R. Sevilla

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
#include "arcueid.h"
#include "vmengine.h"

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

#include "disasm.h"

static const char *fmt[] = { "%08x\t%s", /* no args */
			     "%08x\t%s\t%02x", /* 1 arg */
			     "%08x\t%s\t%02x,%02x", /* 2 arg */
			     "%08x\t%s\t%02x,%02x,%02x", /* 3 arg */
			     "%08x\t%s\t%08x"		   /* jump */
};

value __arc_disasm_inst(arc *c, value *base, value *inst, int *instlen)
{
  int offset, nops, fmtstr, slen=0, jumpofs;
  char *dastr=NULL;
  const char *insttxt;
  value vstr;

  offset = inst - base;
  nops = (FIX2INT(*inst) >> 6) & 0x03;
  insttxt = disasm_opcodes[*inst];
  fmtstr = (insttxt[0] == 'j') ? 4 : nops;
  for (;;) {
    switch (fmtstr) {
    case 0:
      slen = snprintf(dastr, slen, fmt[fmtstr], offset, insttxt);
      break;
    case 1:
      slen = snprintf(dastr, slen, fmt[fmtstr], offset, insttxt,
		      FIX2INT(*(inst + 1)));
      break;
    case 2:
      slen = snprintf(dastr, slen, fmt[fmtstr], offset, insttxt,
		      FIX2INT(*(inst + 1)),
		      FIX2INT(*(inst + 2)));
      break;
    case 3:
      slen = snprintf(dastr, slen, fmt[fmtstr], offset, insttxt,
		      FIX2INT(*(inst + 1)),
		      FIX2INT(*(inst + 2)),
		      FIX2INT(*(inst + 3)));
      break;
    case 4:
      jumpofs = offset + FIX2INT(*(inst + 1));
      slen = snprintf(dastr, slen, fmt[fmtstr], offset, insttxt, jumpofs);
      break;
    }
    if (dastr == NULL) {
      dastr = alloca(sizeof(char) * ++slen);
    } else {
      break;
    }
  }
  vstr = arc_mkstringc(c, dastr);
  if (instlen != NULL)
    *instlen = nops + 1;
  return(vstr);
}
