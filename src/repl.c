/* 
  Copyright (C) 2011 Rafael R. Sevilla

  This file is part of Arcueid

  Arcueid is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Arcueid is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this library; if not,  see <http://www.gnu.org/licenses/>.
*/
#include "arcueid.h"
#include "vmengine.h"
#include "symbols.h"
#include "utf.h"
#include "../config.h"

int main(int argc, char **argv)
{
  arc *c, cc;
  value sexpr, readfp, cctx, code, ret;
  Rune ch;

  c = &cc;
  arc_set_memmgr(c);
  cc.genv = arc_mkhash(c, 8);
  arc_init_reader(c);
  arc_init_builtins(c);
  cc.stksize = TSTKSIZE;
  cc.quantum = PQUANTA;
  readfp = arc_hash_lookup(c, c->genv, ARC_BUILTIN(c, S_STDIN));
  for (;;) {
    printf("arc> ");
    while ((ch = arc_readc_rune(c, readfp)) >= 0 && ucisspace(ch))
      ;
    if (ch < 0)
      break;
    arc_ungetc_rune(c, ch, readfp);
    sexpr = arc_read(c, readfp);
    cctx = arc_mkcctx(c, INT2FIX(1), 0);
    arc_compile(c, sexpr, cctx, CNIL, CTRUE);
    code = arc_cctx2code(c, cctx);
    ret = arc_macapply(c, code, CNIL);
    arc_print_string(c, arc_prettyprint(c, ret));
    c->rungc(c);
  }
  return(EXIT_SUCCESS);
}

