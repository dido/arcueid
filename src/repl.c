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
#include <stdio.h>
#include <getopt.h>
#include <setjmp.h>
#include <stdarg.h>
#include "arcueid.h"
#include "vmengine.h"
#include "symbols.h"
#include "utf.h"
#include "../config.h"

#define DEFAULT_LOADFILE PKGDATA "/arc.arc"

extern int debug;

static jmp_buf err_jmp_buf;

static void error_handler(struct arc *c, const char *fmt, ...)
{
  puts(fmt);
  longjmp(err_jmp_buf, 1);
}

int main(int argc, char **argv)
{
  arc *c, cc;
  value sexpr, readfp, cctx, code, ret, initload;
  char *loadstr;

  c = &cc;
  arc_set_memmgr(c);
  cc.genv = arc_mkhash(c, 8);
  arc_init_reader(c);
  arc_init_builtins(c);
  cc.stksize = TSTKSIZE;
  cc.quantum = PQUANTA;
  cc.signal_error = error_handler;

  loadstr = (argc > 1) ? argv[1] : DEFAULT_LOADFILE;

  initload = arc_infile(c, arc_mkstringc(c, loadstr));
  while ((sexpr = arc_read(c, initload)) != CNIL) {
    /* arc_print_string(c, arc_prettyprint(c, sexpr)); */
    cctx = arc_mkcctx(c, INT2FIX(1), 0);
    arc_compile(c, sexpr, cctx, CNIL, CTRUE);
    code = arc_cctx2code(c, cctx);
    ret = arc_macapply(c, code, CNIL);
    c->rungc(c);
  }
  arc_close(c, initload);

  /* arc_disasm(c, arc_rep(c, arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "or")))); */
  setjmp(err_jmp_buf);
  readfp = arc_hash_lookup(c, c->genv, ARC_BUILTIN(c, S_STDIN));
  for (;;) {
    printf("arc> ");
    sexpr = arc_read(c, readfp);
    if (sexpr == CNIL)
      break;
    cctx = arc_mkcctx(c, INT2FIX(1), 0);
    arc_compile(c, sexpr, cctx, CNIL, CTRUE);
    code = arc_cctx2code(c, cctx);
    ret = arc_macapply(c, code, CNIL);
    arc_print_string(c, arc_prettyprint(c, ret));
    c->rungc(c);
  }
  return(EXIT_SUCCESS);
}

