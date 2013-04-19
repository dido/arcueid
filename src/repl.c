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

#include "io.h"

extern void __arc_print_string(arc *c, value ppstr);

#define DEFAULT_LOADFILE PKGDATA "/arc.arc"

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
  return(CNIL);
}

jmp_buf ejb;

static void errhandler(arc *c, value str)
{
  __arc_print_string(c, str);
  longjmp(ejb, 1);
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
  
int main(int argc, char **argv)
{
  value ret, cctx, code, clos;
  int i;
  char *replcode, *loadstr;
  arc *c, cc;

  banner();
  loadstr = (argc > 1) ? argv[1] : DEFAULT_LOADFILE;
  c = &cc;
  c->errhandler = errhandler;
  arc_init(c);
  if (setjmp(ejb) == 1) {
    fprintf(stderr, "failed to load arc.arc\n");
    arc_deinit(c);
    return(EXIT_FAILURE);
  }
  c->curthread = arc_mkthread(c);
  /* Load arc.arc into our system */
  arc_bindcstr(c, "initload-file", arc_mkstringc(c, loadstr));
  EXECUTE("(assign initload (infile initload-file))");
  if (NIL_P(ret))
    longjmp(ejb, 1);
  i=0;
  for (;;) {
    i++;
    EXECUTE("(assign sexpr (sread initload nil))");
    if (ret == CNIL)
      break;
    /*
    printf("%d: ", i);
    EXECUTE("(disp sexpr)");
    EXECUTE("(disp #\\u000a)");
    */
    /*
    EXECUTE("(disp (eval sexpr))");
    EXECUTE("(disp #\\u000a)");
    */
    EXECUTE("(eval sexpr)");
  }
  EXECUTE("(close initload)");
  c->gc(c);
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stdin, NULL, _IONBF, 0);
  arc_bindcstr(c, "warranty", arc_mkccode(c, 0, warranty,
					  arc_intern_cstr(c, "warranty")));
  /*  replcode = "(w/uniq eof (whiler e (do (disp \"arc> \") (read (stdin) eof)) eof (do (write (eval e))) (disp #\\u000a)))"; */
  replcode = "(whiler e (do (disp \"arc> \") (read (stdin) nil)) nil (do (write (eval e)) (disp #\\u000a)))";
  COMPILE(replcode);
  cctx = TVALR(c->curthread);
  code = arc_cctx2code(c, cctx);
  clos = arc_mkclos(c, code, CNIL);
  arc_bindcstr(c, "repl-code*", clos);
  setjmp(ejb);
  clos = arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "repl-code*"));
  if (TYPE(clos) != T_CLOS) {
    fprintf(stderr, "bad repl code\n");
    arc_deinit(c);
    return(EXIT_FAILURE);
  }
  arc_spawn(c, clos);
  arc_thread_dispatch(c);
  arc_deinit(c);
  return(EXIT_SUCCESS);
}
