/* 
  Copyright (C) 2013 Rafael R. Sevilla

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
#include <stdlib.h>
#include <getopt.h>
#include <setjmp.h>
#include <stdarg.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>
#include "arcueid.h"
#include "vmengine.h"
#include "builtins.h"
#include "utf.h"
#include "io.h"
#include "compiler.h"
#include "../config.h"
#include "gopt.h"

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

#ifdef HAVE_LIBREADLINE
#if defined(HAVE_READLINE_READLINE_H)
#include <readline/readline.h>
#include <readline/history.h>
#elif defined(HAVE_READLINE_H)
#include <readline.h>
#include <history.h>
#else
#error Readline library found but no headers!
#endif

/* Readline I/O module

   XXX - Once we have a loadable module interface this code needs to
   go there.

   For now we put this here, to prevent the main lib from becoming
   fully GPLed.
*/

struct rlio_t {
  value str;
  int idx;
  int closed;
};

#define RLDATA(rlio) (IODATA(rlio, struct rlio_t *))

static void rl_marker(arc *c, value v, int depth,
		      void (*markfn)(arc *, value, int))
{
  markfn(c, RLDATA(v)->str, depth+1);
}

static void rl_sweeper(arc *c, value v)
{
  RLDATA(v)->closed = 1;
  rl_callback_handler_remove();
}

static typefn_t rlio_tfn = {
  rl_marker,
  rl_sweeper,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

static char *line_read;
static int read_eof;

void rl_lthandler(char *str)
{
  line_read = str;
  if (str == NULL)
    read_eof = 1;
}

static AFFDEF(rl_closed_p)
{
  AARG(rlio);
  AFBEGIN;
  ARETURN((RLDATA(AV(rlio))->closed) ? CTRUE : CNIL);
  AFEND;
}
AFFEND

static int stdin_ready(arc *c)
{
  fd_set rfds;
  struct timeval tv;
  int retval, check;

  /* XXX - NOT PORTABLE! */
#ifdef _IO_fpos_t
  check = (stdin->_IO_read_ptr != stdin->_IO_read_end);
#else
  check = (stdin->_gptr < (stdin)->_egptr);
#endif
  if (check)
    return(1);

  FD_ZERO(&rfds);
  FD_SET(fileno(stdin), &rfds);
  tv.tv_usec = tv.tv_sec = 0;
  retval = select(fileno(stdin)+1, &rfds, NULL, NULL, &tv);
  if (retval == -1) {
    int en = errno;

    arc_err_cstrfmt(c, "error selecting for readline (%s; errno=%d)", strerror(en), en);
    return(0);
  }

  if (retval == 0) {
    return(0);
  }
  if (FD_ISSET(fileno(stdin), &rfds))
    return(1);
  return(0);
}

static AFFDEF(rl_ready)
{
  AARG(rlio);
  AFBEGIN;
  (void)rlio;
  ARETURN(CTRUE);
  AFEND;
}
AFFEND

static AFFDEF(rl_wready)
{
  AARG(rlio);
  AFBEGIN;
  (void)rlio;
  /* We cannot write to a readline port */
  ARETURN(CNIL);
  AFEND;
}
AFFEND

static AFFDEF(rl_getb)
{
  AARG(rlio);
  AVAR(len);
  value rlstr;
  AFBEGIN;
  WV(len, INT2FIX((NIL_P(RLDATA(AV(rlio))->str)) ? 0 : arc_strlen(c, RLDATA(AV(rlio))->str)));
  while (RLDATA(AV(rlio))->idx >= FIX2INT(AV(len))) {
    if (!stdin_ready(c)) {
      AIOWAITR(fileno(stdin));
      continue;
    }
    rl_callback_read_char();

    if (read_eof)
      ARETURN(CNIL);

    if (line_read == NULL)
      continue;

    if (line_read && *line_read) {
      add_history(line_read);
      next_history();		/* not sure why this is needed... */
      rlstr = arc_mkstringc(c, line_read);
      RLDATA(AV(rlio))->str = arc_strcatc(c, rlstr, '\n');
      WV(len, arc_strlen(c, RLDATA(AV(rlio))->str));
      RLDATA(AV(rlio))->idx = 0;
    }
    free(line_read);		/* not sure if this should be done... */
    line_read = NULL;
  }
  ARETURN(INT2FIX(arc_strindex(c, RLDATA(AV(rlio))->str, RLDATA(AV(rlio))->idx++)));
  AFEND;
}
AFFEND

static AFFDEF(rl_putb)
{
  AARG(rlio, byte);
  AFBEGIN;
  (void)rlio;
  (void)byte;
  arc_err_cstrfmt(c, "cannot write to a readline port");
  ARETURN(AV(byte));
  AFEND;
}
AFFEND

static AFFDEF(rl_seek)
{
  AARG(rlio, offset, whence);
  AFBEGIN;
  (void)rlio;
  (void)offset;
  (void)whence;
  arc_err_cstrfmt(c, "cannot seek a readline port");
  ARETURN(CNIL);
  AFEND;
}
AFFEND

static AFFDEF(rl_tell)
{
  AARG(rlio);
  AFBEGIN;
  (void)rlio;
  arc_err_cstrfmt(c, "cannot tell a readline port");
  ARETURN(CNIL);
  AFEND;
}
AFFEND

static AFFDEF(rl_close)
{
  AARG(rlio);
  AFBEGIN;
  RLDATA(AV(rlio))->closed = 1;
  line_read = NULL;
  rl_callback_handler_remove();
  ARETURN(CNIL);
  AFEND;
}
AFFEND

static value arc_rl_on_new_line_with_prompt(arc *c)
{
  return(INT2FIX(rl_on_new_line_with_prompt()));
}

value arc_readlineport(arc *c)
{
  value rlio;
  value io_ops;

  rlio = __arc_allocio(c, T_INPORT, &rlio_tfn, sizeof(struct rlio_t));
  IO(rlio)->flags = IO_FLAG_GETB_IS_GETC;
  io_ops = arc_mkvector(c, IO_last+1);
  SVINDEX(io_ops, IO_closed_p, arc_mkaff(c, rl_closed_p, CNIL));
  SVINDEX(io_ops, IO_ready, arc_mkaff(c, rl_ready, CNIL));
  SVINDEX(io_ops, IO_wready, arc_mkaff(c, rl_wready, CNIL));
  SVINDEX(io_ops, IO_getb, arc_mkaff(c, rl_getb, CNIL));
  SVINDEX(io_ops, IO_putb, arc_mkaff(c, rl_putb, CNIL));
  SVINDEX(io_ops, IO_seek, arc_mkaff(c, rl_seek, CNIL));
  SVINDEX(io_ops, IO_tell, arc_mkaff(c, rl_tell, CNIL));
  SVINDEX(io_ops, IO_close, arc_mkaff(c, rl_close, CNIL));
  IO(rlio)->io_ops = io_ops;
  IO(rlio)->name = arc_mkstringc(c, "(repl)");
  RLDATA(rlio)->closed = 0;
  RLDATA(rlio)->idx = 0;
  RLDATA(rlio)->str = CNIL;
  using_history();
  rl_variable_bind("blink-matching-paren", "on");
  rl_basic_quote_characters = "\"";
  rl_basic_word_break_characters = "[]()!:~\"";
  rl_already_prompted = 1;
  rl_callback_handler_install("arc> ", rl_lthandler);
  line_read = NULL;
  read_eof = 0;
  return(rlio);
}

/* End Readline I/O module */

#endif

extern void __arc_print_string(arc *c, value ppstr);

#define DEFAULT_LOADFILE PKGDATA "/arc.arc"
#define DEFAULT_HISTORY_FILE ".arcueid_history"

static void banner(void)
{
  printf("%s REPL Copyright (c) 2013 Rafael R. Sevilla\n", PACKAGE_STRING);
  printf("This is free software; see the source code for copying conditions.\n");
  printf("There is ABSOLUTELY NO WARRANTY; for details type (warranty)\n");
}

static value warranty(arc *c)
{
  printf("\n%s REPL\nCopyright (c) 2013 Rafael R. Sevilla\n", PACKAGE_STRING);
  printf("\nThis program is free software; you can redistribute it and/or modify\nit under the terms of the GNU General Public License as published by\nthe Free Software Foundation; either version 3 of the License, or\n(at your option) any later version.\n\nThis program is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for more details.\n\nYou should have received a copy of the GNU General Public License\nalong with this program.  If not, see <http://www.gnu.org/licenses/>.\n\n");
#ifdef HAVE_LIBREADLINE
  printf("Readline Support Enabled.\n\n");
#endif
  return(CNIL);
}

static void help(void)
{
  printf("Usage: arcueid [OPTION]... [FILE]...\n");
  printf("Evaluate Arc code, interactively or from a script.\n\n");
  printf("  -I, --include=PATH    add PATH to the load path (may be used\n");
  printf("                        more than once)\n");
  printf("  --init-load           init load file (defaults to %s)\n",
	 DEFAULT_LOADFILE);
  printf("  -l, --load=FILE       load FILE before dropping into the REPL\n");
  printf("                        (may be used more than once)\n");
  printf("  -q, --quiet           do not display banner on startup\n");
  printf("  -h, --help            display this help and exit\n");
  printf("  -v, --version         output version information and exit\n");
}

static jmp_buf ejb;
static value replthread=CNIL;

static void errhandler2(arc *c, value thr, value str)
{
  __arc_print_string(c, str);
  exit(EXIT_FAILURE);
}

static void errhandler(arc *c, value thr, value str)
{
  __arc_print_string(c, str);
#ifdef HAVE_LIBREADLINE
  printf("arc> ");
  fflush(NULL);
  rl_redisplay();
#endif
  if (thr == replthread)
    longjmp(ejb, 1);
  /* else just return */
}

#define QUANTA ULONG_MAX

#define CPUSH_(val) CPUSH(c->curthread, val)

#define XCALL0(clos) do {				\
    TQUANTA(c->curthread) = QUANTA;			\
    SVALR(c->curthread, clos);				\
    TARGC(c->curthread) = 0;				\
    __arc_thr_trampoline(c, c->curthread, TR_FNAPP);	\
  } while (0)

#define XCALL(fname, ...) do {				\
    SVALR(c->curthread, arc_mkaff(c, fname, CNIL));	\
    TARGC(c->curthread) = NARGS(__VA_ARGS__);		\
    FOR_EACH(CPUSH_, __VA_ARGS__);			\
    __arc_thr_trampoline(c, c->curthread, TR_FNAPP);	\
  } while (0)

AFFDEF(compile_something)
{
  AARG(something);
  value sexpr;
  AVAR(sio);
  AFBEGIN;
  TQUANTA(thr) = QUANTA;	/* needed so macros can execute */
  WV(sio, arc_instring(c, AV(something), CNIL));
  AFCALL(arc_mkaff(c, arc_sread, CNIL), AV(sio), CNIL);
  sexpr = AFCRV;
  AFTCALL(arc_mkaff(c, arc_compile, CNIL), sexpr, arc_mkcctx(c), CNIL, CTRUE);
  AFEND;
}
AFFEND

#define COMPILE(str) XCALL(compile_something, arc_mkstringc(c, str))

#define EXECUTE(sexpr)				\
  COMPILE(sexpr);				\
  cctx = TVALR(c->curthread);			\
  code = arc_cctx2code(c, cctx);		\
  clos = arc_mkclos(c, code, CNIL);		\
  XCALL0(clos);					\
  ret = TVALR(c->curthread)

static arc *c, cc;

void cleanup(void)
{
#ifdef HAVE_LIBREADLINE
  {
    char *homedir;

    homedir = getenv("HOME");
    if (homedir != NULL) {
      char *histfile = alloca(sizeof(char)
			      * (strlen(homedir)
				 + strlen(DEFAULT_HISTORY_FILE) + 2));
      sprintf(histfile, "%s/%s", homedir, DEFAULT_HISTORY_FILE);
      write_history(histfile);
      clear_history();
      rl_callback_handler_remove();
    }
  }
#endif

  arc_deinit(c);
}
  
int main(int argc, const char **argv)
{
  value ret, cctx, code, clos;
  int i;
  char *replcode;
  const char *loadstr, *ls;
  void *options =
    gopt_sort(&argc, argv,
	      gopt_start(gopt_option('h', 0, gopt_shorts('h', '?'),
				     gopt_longs("help")),
			 gopt_option('v', 0, gopt_shorts('v'),
				     gopt_longs("version")),
			 gopt_option('q', 0, gopt_shorts('q'),
				     gopt_longs("quiet")),
			 gopt_option('L', GOPT_ARG, gopt_shorts(0),
				     gopt_longs("init-load")),
			 gopt_option('I', GOPT_ARG|GOPT_REPEAT,
				     gopt_shorts('I'),
				     gopt_longs("include")),
			 gopt_option('l', GOPT_ARG|GOPT_REPEAT,
				     gopt_shorts('l'),
				     gopt_longs("load"))));

  (void)ret;
  if (gopt(options, 'h')) {
    help();
    exit(EXIT_SUCCESS);
  }

  if (gopt(options, 'v')) {
    printf("%s (Arc 3.1 compatible)\n", PACKAGE_STRING);
    exit(EXIT_SUCCESS);
  }

  if (!gopt(options, 'q'))
    banner();

  loadstr = DEFAULT_LOADFILE;
  if (gopt_arg(options, 'L', &ls))
    loadstr = ls;

  c = &cc;
  c->errhandler = errhandler2;
  arc_init(c);
  atexit(cleanup);

  c->curthread = arc_mkthread(c);
  /* Load arc.arc into our system */
  arc_bindcstr(c, "initload-file", arc_mkstringc(c, loadstr));
  EXECUTE("(load initload-file)");
  c->errhandler = errhandler;
  c->gc(c);

  /* add load path dirs */
  arc_loadpath_add(c, arc_mkstringc(c, PKGDATA));
  for (i=0;; i++) {
    const char *path;
    path = gopt_arg_i(options, 'I', i);
    if (path == NULL)
      break;
    arc_loadpath_add(c, arc_mkstringc(c, path));
  }

  /* load extra files */
  for (i=0;; i++) {
    const char *loadfile;
    loadfile = gopt_arg_i(options, 'l', i);
    if (loadfile == NULL)
      break;
    XCALL(arc_load, arc_mkstringc(c, loadfile));
  }
#if 0
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stdin, NULL, _IONBF, 0);
#endif
  arc_bindcstr(c, "warranty", arc_mkccode(c, 0, warranty,
					  arc_intern_cstr(c, "warranty")));
#ifdef HAVE_LIBREADLINE
  {
    value rlfp;
    char *homedir;

    homedir = getenv("HOME");
    if (homedir != NULL) {
      char *histfile = alloca(sizeof(char)
			      * (strlen(homedir)
				 + strlen(DEFAULT_HISTORY_FILE) + 2));
      sprintf(histfile, "%s/%s", homedir, DEFAULT_HISTORY_FILE);
      read_history(histfile);
    }
    printf("arc> ");
    fflush(NULL);
    rlfp = arc_readlineport(c);
    arc_bindcstr(c, "repl-readline", rlfp);
    arc_bindcstr(c, "rl-on-new-line-with-prompt", arc_mkccode(c, 0, arc_rl_on_new_line_with_prompt, CNIL));
    /* Load history */
  }
#endif

#ifdef HAVE_LIBREADLINE
  replcode = "(w/uniq eof (whiler e (read repl-readline eof) eof (do (write (eval e)) (prn) (disp \"arc> \") (flushout) (rl-on-new-line-with-prompt))))";
#else
  replcode = "(whiler e (do (disp \"arc> \") (flushout) (read (stdin) nil)) nil (do (write (eval e)) (disp #\\u000a)))";
#endif
  COMPILE(replcode);
  cctx = TVALR(c->curthread);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  arc_bindcstr(c, "repl-code*", clos);
  setjmp(ejb);
  clos = arc_gbind_cstr(c, "repl-code*");
  if (TYPE(clos) != T_CLOS) {
    fprintf(stderr, "bad repl code\n");
    arc_deinit(c);
    return(EXIT_FAILURE);
  }
  replthread = arc_spawn(c, clos);
  arc_thread_dispatch(c);
  return(EXIT_SUCCESS);
}
