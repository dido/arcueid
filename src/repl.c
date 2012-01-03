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

  Note that unlike most of the other code in src this particular one
  is licensed under GPLv3 not LGPLv3.  It contains code that links to
  readline, which is GPL.
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

#ifdef HAVE_LIBREADLINE
#ifdef HAVE_READLINE_H
#include <readline.h>
#elif HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#endif
#ifdef HAVE_HISTORY_H
#include <history.h>
#elif HAVE_READLINE_HISTORY
#include <readline/history.h>
#endif

#include "io.h"

/* Readline stuff -- NOTE: since libreadline is GPLed, we put this here,
   so as not to prevent the main libarcueid from being LGPLed instead. */

static value readline_pp(arc *c, value v)
{
  return(arc_mkstringc(c, "#<readline-port>"));
}

static void readline_marker(arc *c, value v, int level,
			void (*mark)(arc *, value, int))
{
  mark(c, PORTS(v).str, level+1);
}

static void readline_sweeper(arc *c, value v)
{
  c->free_block(c, (void *)v);
}

static int readline_getb(arc *c, struct arc_port *p)
{
  static char *line_read = (char *)NULL;
  int len;
  value rlstr;

  len = (p->u.strfile.str == CNIL) ? 0 : arc_strlen(c, p->u.strfile.str);
  while (p->u.strfile.idx >= len) {
    /* try to read a new line */
    if (line_read) {
      free(line_read);
      line_read = (char *)NULL;
    }
    line_read = readline("arc> ");
    if (line_read && *line_read) {
      add_history(line_read);
      rlstr = arc_mkstringc(c, line_read);
      p->u.strfile.str = arc_strcatc(c, rlstr, '\n');
      len = arc_strlen(c, p->u.strfile.str);
      p->u.strfile.idx = 0;
    }
    if (line_read == NULL)
      return(EOF);
  }
  return(arc_strindex(c, p->u.strfile.str, p->u.strfile.idx++));
}

static int readline_putb(arc *c, struct arc_port *p, int byte)
{
  c->signal_error(c, "cannot write to a readline port");
  return(byte);
}

/* works just like stringio */
static int readline_seek(arc *c, struct arc_port *p, int64_t offset, int whence)
{
  int64_t len;

  len = (int64_t)arc_strlen(c, p->u.strfile.str);
  switch (whence) {
  case SEEK_SET:
    break;
  case SEEK_CUR:
    offset += p->u.strfile.idx;
    break;
  case SEEK_END:
    offset = len - offset;
    break;
  default:
    return(-1);
  }
  if (offset >= len || offset < 0)
    return(-1);
  p->u.strfile.idx = offset;
  return(0);
}

static int64_t readline_tell(arc *c, struct arc_port *p)
{
  return(p->u.strfile.idx);
}

static int readline_close(arc *c, struct arc_port *p)
{
  return(0);
}

value arc_readlineport(arc *c)
{
  void *cellptr;
  value fd;

  cellptr = c->get_block(c, sizeof(struct cell) + sizeof(struct arc_port));
  if (cellptr == NULL)
    c->signal_error(c, "openstring: cannot allocate memory");
  fd = (value)cellptr;
  BTYPE(fd) = T_PORT;
  REP(fd)._custom.pprint = readline_pp;
  REP(fd)._custom.marker = readline_marker;
  REP(fd)._custom.sweeper = readline_sweeper;
  PORT(fd)->type = FT_STRING;
  PORTS(fd).idx = 0;
  PORTS(fd).str = CNIL;
  PORT(fd)->getb = readline_getb;
  PORT(fd)->putb = readline_putb;
  PORT(fd)->seek = readline_seek;
  PORT(fd)->tell = readline_tell;
  PORT(fd)->close = readline_close;
  PORT(fd)->ungetrune = -1;	/* no rune available */
  rl_variable_bind("blink-matching-paren", "on");
  rl_basic_quote_characters = "\"";
  rl_basic_word_break_characters = "[]()!:~\"";
  return(fd);
}

#endif

#define DEFAULT_LOADFILE PKGDATA "/arc.arc"

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
  arc_init(c);
  c->signal_error = error_handler;

  loadstr = (argc > 1) ? argv[1] : DEFAULT_LOADFILE;

  initload = arc_infile(c, arc_mkstringc(c, loadstr));
  while ((sexpr = arc_read(c, initload)) != CNIL) {
    /*    arc_print_string(c, arc_prettyprint(c, sexpr));
	  printf("\n"); */
    cctx = arc_mkcctx(c, INT2FIX(1), 0);
    arc_compile(c, sexpr, cctx, CNIL, CTRUE);
    code = arc_cctx2code(c, cctx);
    ret = arc_macapply(c, code, CNIL);
    c->rungc(c);
  }
  arc_close(c, initload);

  setjmp(err_jmp_buf);
#ifdef HAVE_LIBREADLINE
  readfp = arc_readlineport(c);
  arc_bindsym(c, arc_intern_cstr(c, "repl-readline"), readfp);
#else
  readfp = arc_hash_lookup(c, c->genv, ARC_BUILTIN(c, S_STDIN));
#endif
  for (;;) {
#ifndef HAVE_LIBREADLINE
    printf("arc> ");
#endif
    sexpr = arc_read(c, readfp);
    if (sexpr == CNIL)
      break;
    cctx = arc_mkcctx(c, INT2FIX(1), 0);
    arc_compile(c, sexpr, cctx, CNIL, CTRUE);
    code = arc_cctx2code(c, cctx);
    ret = arc_macapply(c, code, CNIL);
    arc_print_string(c, arc_prettyprint(c, ret));
    printf("\n");
    c->rungc(c);
  }
  return(EXIT_SUCCESS);
}

