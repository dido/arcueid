/* 
  Copyright (C) 2013 Rafael R. Sevilla

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

struct cfunc_t {
  value name;
  value (*fnptr)();
  int argc;
};

static value cfunc_pprint(arc *c, value sexpr, value *ppstr, value visithash)
{
  __arc_append_cstring(c, "#<cprocedure: ", ppstr);
  arc_prettyprint(c, ((struct cfunc_t *)REP(sexpr))->name, ppstr, visithash);
  __arc_append_cstring(c, ">", ppstr);
  return(*ppstr);
}
static void cfunc_marker(arc *c, value v, int depth,
			  void (*mark)(struct arc *, value, int))
{
  mark(c, ((struct cfunc_t *)REP(v))->name, depth);
}

static unsigned long cfunc_hash(arc *c, value v, arc_hs *s, value visithash)
{
  return(arc_hash(c, ((struct cfunc_t *)REP(v))->name, visithash));
}

static value cfunc_isocmp(arc *c, value v1, value v2, value h1, value h2)
{
  struct cfunc_t *f1, *f2;

  f1 = (struct cfunc_t *)REP(v1);
  f2 = (struct cfunc_t *)REP(v2);

  if (arc_iso(c, f1->name, f2->name, h1, h2) == CNIL)
    return(CNIL);
  if (f1->argc != f2->argc)
    return(CNIL);
  return((f1->fnptr == f2->fnptr) ? CTRUE : CNIL);
}

value arc_mkccode(arc *c, int argc, value (*cfunc)(), value name)
{
  value cfn;
  struct cfunc_t *rcfn;

  cfn = arc_mkobject(c, sizeof(struct cfunc_t), T_CCODE);
  rcfn = (struct cfunc_t *)REP(cfn);
  rcfn->name = name;
  rcfn->fnptr = cfunc;
  rcfn->argc = argc;
  return(cfn);
}

static int cfunc_apply(arc *c, value thr, value cfn)
{
  int argc, i;
  struct cfunc_t *rcfn;
  value *argv;

  rcfn = (struct cfunc_t *)REP(cfn);
  argc = TARGC(thr);
  if (rcfn->argc >= 0 && rcfn->argc != argc) {
    /* XXX - error handling */
    arc_err_cstrfmt(c, "wrong number of arguments (%d for %d)", argc,
		    rcfn->argc);
    return(CNIL);
  }
  if (argc == -2) {
    /* The initial call of a ACFF.  The continuation is initially nil. */
    return(FIX2INT(rcfn->fnptr(c, thr, CNIL)));
  } else {
    argv = alloca(sizeof(value)*argc);
    for (i=argc-1; i>=0; i--)
      argv[i] = CPOP(thr);
    switch (rcfn->argc) {
    case -1:
      TVALR(thr) = rcfn->fnptr(c, argc, argv);
      break;
    case 0:
      TVALR(thr) = rcfn->fnptr(c);
      break;
    case 1:
      TVALR(thr) = rcfn->fnptr(c, argv[0]);
      break;
    case 2:
      TVALR(thr) = rcfn->fnptr(c, argv[0], argv[1]);
      break;
    case 3:
      TVALR(thr) = rcfn->fnptr(c, argv[0], argv[1], argv[2]);
      break;
    case 4:
      TVALR(thr) = rcfn->fnptr(c, argv[0], argv[1], argv[2], argv[3]);
      break;
    case 5:
      TVALR(thr) = rcfn->fnptr(c, argv[0], argv[1], argv[2], argv[3],
			       argv[4]);
      break;
    case 6:
      TVALR(thr) = rcfn->fnptr(c, argv[0], argv[1], argv[2], argv[3],
			       argv[4], argv[5]);
      break;
    case 7:
      TVALR(thr) = rcfn->fnptr(c, argv[0], argv[1], argv[2], argv[3],
			       argv[4], argv[5], argv[6]);
      break;
    case 8:
      TVALR(thr) = rcfn->fnptr(c, argv[0], argv[1], argv[2], argv[3],
			       argv[4], argv[5], argv[6], argv[7]);
      break;
    default:
      arc_err_cstrfmt(c, "too many arguments");
      return(APP_OK);
    }
  }
  return(APP_OK);
}

typefn_t __arc_cfunc_typefn__ = {
  cfunc_marker,
  __arc_null_sweeper,
  cfunc_pprint,
  cfunc_hash,
  NULL,
  cfunc_isocmp,
  cfunc_apply
};
