/* 
  Copyright (C) 2012 Rafael R. Sevilla

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
  along with this library; if not, see <http://www.gnu.org/licenses/>.

  Note that unlike most of the other code in src this particular one
  is licensed under GPLv3 not LGPLv3.  It contains code that links to
  readline, which is GPL.
*/
#include <stdio.h>
#include <getopt.h>
#include <setjmp.h>
#include <stdarg.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>
#include "arcueid.h"
#include "vmengine.h"
#include "symbols.h"
#include "builtin.h"
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
			    void (*mark)(arc *, value, int, value))
{
  mark(c, PORTS(v).str, level+1, CNIL);
}

static char *line_read = (char *)NULL;

static void readline_sweeper(arc *c, value v)
{
  if (line_read != NULL)
    free(line_read);
  line_read = NULL;
}

static int readline_getb(arc *c, struct arc_port *p)
{
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
  arc_err_cstrfmt(c, "cannot write to a readline port");
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
  if (line_read != NULL)
    free(line_read);
  line_read = NULL;
  return(0);
}

static int readline_ready(arc *c, struct arc_port *p)
{
  fd_set rfds;
  struct timeval tv;
  int retval, len;

  /* if we have data from readline, obviously we're ready */
  len = (p->u.strfile.str == CNIL) ? 0 : arc_strlen(c, p->u.strfile.str);
  if (p->u.strfile.idx < len)
    return(1);

  /* otherwise, see if stdin is readable */
  FD_ZERO(&rfds);
  FD_SET(fileno(stdin), &rfds);
  tv.tv_usec = tv.tv_sec = 0;
  retval = select(fileno(stdin)+1, &rfds, NULL, NULL, &tv);

  if (retval == -1) {
    int en = errno;

    arc_err_cstrfmt(c, "error selecting for readline (%s; errno=%d)", strerror(en), en);
    return(0);
  }

  if (retval == 0)
    return(0);
  /* yes, we are ready! */
  if (FD_ISSET(fileno(stdin), &rfds))
    return(1);
  return(0);
}

static int readline_fd(arc *c, struct arc_port *p)
{
  return(fileno(stdin));
}

static arc *rlcc;

static int __arc_rlgetc(FILE *fp)
{
  return(FIX2INT(arc_readb(rlcc, arc_stdin(rlcc))));
}

value arc_readlineport(arc *c)
{
  void *cellptr;
  value fd;

  cellptr = c->get_block(c, sizeof(struct cell) + sizeof(struct arc_port));
  if (cellptr == NULL)
    arc_err_cstrfmt(c, "openstring: cannot allocate memory");
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
  PORT(fd)->ready = readline_ready;
  PORT(fd)->fd = readline_fd;
  PORT(fd)->ungetrune = -1;	/* no rune available */
  PORT(fd)->closed = 0;
  rl_variable_bind("blink-matching-paren", "on");
  rl_basic_quote_characters = "\"";
  rl_basic_word_break_characters = "[]()!:~\"";
  rlcc = c;
  rl_getc_function = __arc_rlgetc;
  return(fd);
}

#endif

#define DEFAULT_LOADFILE PKGDATA "/arc.arc"

static jmp_buf err_jmp_buf;

static void error_handler(struct arc *c, value err)
{
  printf("Error: ");
  arc_print_string(c, err);
  printf("\n");
  longjmp(err_jmp_buf, 1);
}

static void error_handler2(struct arc *c, value err)
{
  printf("Error: ");
  arc_print_string(c, err);
  printf("\n");
}

static void banner(void)
{
  printf("%s REPL Copyright (c) 2012 Rafael R. Sevilla\n", PACKAGE_STRING);
  printf("This is free software; see the source code for copying conditions.\n");
  printf("There is ABSOLUTELY NO WARRANTY; for details type (warranty)\n");
}

static value warranty(arc *c, int argc, value *argv)
{
  printf("\n%s REPL\nCopyright (c) 2012 Rafael R. Sevilla\n", PACKAGE_STRING);
  printf("\nThis program is free software; you can redistribute it and/or modify\nit under the terms of the GNU General Public License as published by\nthe Free Software Foundation; either version 3 of the License, or\n(at your option) any later version.\n\nThis program is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for more details.\n\nYou should have received a copy of the GNU General Public License\nalong with this program.  If not, see <http://www.gnu.org/licenses/>.\n\n");
  return(CNIL);
}

int main(int argc, char **argv)
{
  arc *c, cc;
  value sexpr, cctx, code, initload;
  char *loadstr, *replcode;

  c = &cc;
  arc_init(c);

  banner();
  loadstr = (argc > 1) ? argv[1] : DEFAULT_LOADFILE;

  c->signal_error = error_handler;

  if (setjmp(err_jmp_buf) == 1)
    return(EXIT_FAILURE);
  initload = arc_infile(c, arc_mkstringc(c, loadstr));
  arc_bindsym(c, arc_intern_cstr(c, "initload"), initload);
  arc_bindsym(c, arc_intern_cstr(c, "warranty"), arc_mkccode(c, -1, warranty, arc_intern_cstr(c, "warranty")));
  while ((sexpr = arc_read(c, initload, CNIL)) != CNIL) {
    /* arc_print_string(c, arc_prettyprint(c, sexpr)); 
       printf("\n"); */
    cctx = arc_mkcctx(c, INT2FIX(1), 0);
    arc_compile(c, sexpr, cctx, CNIL, CTRUE);
    code = arc_cctx2code(c, cctx);
    arc_macapply(c, code, CNIL);
    c->rungc(c);
  }
  arc_close(c, initload);

#ifdef HAVE_LIBREADLINE
  {
    value readfp = arc_readlineport(c);
    arc_bindsym(c, arc_intern_cstr(c, "repl-readline"), readfp);
  }
#endif

  c->signal_error = error_handler2;

  /* read-eval-print in Arcueid! */
#ifdef HAVE_LIBREADLINE
  replcode = "(w/uniq eof (whiler e (read repl-readline eof) eof (do (write (eval e)) (prn))))";
#else
  replcode = "(w/uniq eof (whiler e (do (disp \"arc> \") (read (stdin) eof)) eof (do (write (eval e))) (prn)))";
#endif
  sexpr = arc_read(c, arc_instring(c, arc_mkstringc(c, replcode), CNIL), CNIL);
  cctx = arc_mkcctx(c, INT2FIX(1), 0);
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);
  code = arc_cctx2code(c, cctx);
  code = arc_mkclosure(c, code, CNIL);
  arc_spawn(c, code);
  arc_thread_dispatch(c);
  return(EXIT_SUCCESS);
}

