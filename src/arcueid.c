/* 
  Copyright (C) 2012 Rafael R. Sevilla

  This file is part of Arcueid

  Arcueid is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; either version 3 of the
  License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
*/
/* miscellaneous procedures and initialization */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <float.h>
#include <inttypes.h>
#include "arcueid.h"
#include "alloc.h"
#include "arith.h"
#include "utf.h"
#include "symbols.h"
#include "builtin.h"
#include "vmengine.h"
#include "../config.h"

value arc_is(arc *c, value v1, value v2)
{
  int i;

  /* An object is definitely the same as itself */
  if (v1 == v2)
    return(CTRUE);
  /* Two objects with different types are definitely not the same */
  if (TYPE(v1) != TYPE(v2))
    return(CNIL);

  switch (TYPE(v1)) {
#ifdef HAVE_GMP_H
  case T_BIGNUM:
    return((mpz_cmp(REP(v1)._bignum, REP(v2)._bignum) == 0) ? CTRUE : CNIL);
  case T_RATIONAL:
    return((mpq_equal(REP(v1)._rational, REP(v2)._rational)) ? CTRUE : CNIL);
#endif
  case T_FLONUM:
    return((REP(v1)._flonum == REP(v2)._flonum) ? CTRUE : CNIL);
  case T_COMPLEX:
    return((REP(v1)._complex.re == REP(v2)._complex.re)
	   && (REP(v1)._complex.im == REP(v2)._complex.im) ? CTRUE : CNIL);
  case T_CHAR:
    return((REP(v1)._char == REP(v2)._char) ? CTRUE : CNIL);
  case T_STRING:
    if (REP(v1)._str.length != REP(v2)._str.length)
      return(CNIL);
    for (i=0; i<REP(v1)._str.length; i++) {
      if (arc_strindex(c, v1, i) != arc_strindex(c, v2, i))
	return(CNIL);
    }
    return(CTRUE);
  default:
    /* objects of other types cannot be equal to each other unless they
       have the same reference */
    return(CNIL);
  }
  return(CNIL);
}

value arc_is_multi(arc *c, int argc, value *argv)
{
  value ret=CTRUE, first;
  int i;

  if (argc < 2)
    return(ret);
  first = argv[0];
  for (i=1; i<argc; i++) {
    ret = arc_is(c, first, argv[i]);
    if (ret == CNIL)
      break;
  }
  return(ret);
}

extern value __arc_hash_iso(arc *c, value v1, value v2);

value arc_iso(arc *c, value v1, value v2)
{
  value elem1, elem2;
  int i;

  /* An object is definitely the same as itself */
  if (v1 == v2)
    return(CTRUE);
  /* Two objects with different types are definitely not the same */
  if (TYPE(v1) != TYPE(v2))
    return(CNIL);
  switch (TYPE(v1)) {
  case T_NIL:
  case T_TRUE:
  case T_FIXNUM:
  case T_SYMBOL:
    return(v1 == v2);
#ifdef HAVE_GMP_H
  case T_BIGNUM:
    return((mpz_cmp(REP(v1)._bignum, REP(v2)._bignum) == 0) ? CTRUE : CNIL);
  case T_RATIONAL:
    return((mpq_equal(REP(v1)._rational, REP(v2)._rational)) ? CTRUE : CNIL);
#endif
  case T_FLONUM:
    return((REP(v1)._flonum == REP(v2)._flonum) ? CTRUE : CNIL);
  case T_COMPLEX:
    return((REP(v1)._complex.re == REP(v2)._complex.re)
	   && (REP(v1)._complex.im == REP(v2)._complex.im) ? CTRUE : CNIL);
  case T_CHAR:
    return((REP(v1)._char == REP(v2)._char) ? CTRUE : CNIL);
  case T_STRING:
    if (REP(v1)._str.length != REP(v2)._str.length)
      return(CNIL);
    for (i=0; i<REP(v1)._str.length; i++) {
      if (arc_strindex(c, v1, i) != arc_strindex(c, v2, i))
	return(CNIL);
    }
    return(CTRUE);
  case T_CONS:
    /* XXX: this naive traversal will loop forever if there is a loop
       in the cons cells! We need something more sophisticated to do
       this properly.  Well, the reference Arc implementation by Paul
       Graham is no better... */
    for (; TYPE(v1) == T_CONS && TYPE(v2) == T_CONS;
	 v1 = cdr(v1), v2 = cdr(v2)) {
      elem1 = car(v1);
      elem2 = car(v2);
      if (!arc_iso(c, elem1, elem2))
	return(CNIL);
    }
    /* compare last cdr */
    return(arc_iso(c, v1, v2));
  case T_TABLE:
    return(__arc_hash_iso(c, v1, v2));
  case T_TBUCKET:
    if (arc_iso(c, REP(v1)._hashbucket.key, REP(v2)._hashbucket.key) == CNIL)
      return(CNIL);
    if (arc_iso(c, REP(v1)._hashbucket.val, REP(v2)._hashbucket.val) == CNIL)
      return(CNIL);
    return(CTRUE);
  case T_TAGGED:
  case T_CLOS:
  case T_ENV:
    if (arc_iso(c, car(v1), car(v2)) == CNIL)
      return(CNIL);
    if (arc_iso(c, cdr(v1), cdr(v2)) == CNIL)
      return(CNIL);
    return(CTRUE);
  case T_EXCEPTION:
  case T_THREAD:
  case T_VECTOR:
  case T_CONT:
  case T_CODE:
  case T_CHAN:
  case T_XCONT:
    /* XXX - handle vectors */
    if (VECLEN(v1) != VECLEN(v2))
      return(CNIL);
  case T_INPUT:
  case T_OUTPUT:
  case T_PORT:
  case T_CUSTOM:
    /* XXX - handle ports and custom */
  case T_VMCODE:
    /* XXX - handle vmcode */
  case T_CCODE:
  case T_NONE:
    return(CNIL);
  }
  return(CNIL);
}

value arc_iso_multi(arc *c, int argc, value *argv)
{
  value ret=CTRUE, first;
  int i;

  if (argc < 2)
    return(ret);
  first = argv[0];
  for (i=1; i<argc; i++) {
    ret = arc_iso(c, first, argv[i]);
    if (ret == CNIL)
      break;
  }
  return(ret);
}

value scar(value x, value y)
{
  WB(&car(x), y);
  return(y);
}

value scdr(value x, value y)
{
  WB(&cdr(x), y);
  return(y);
}

value arc_mkvector(arc *c, int length)
{
  void *cellptr;
  value vect;

  cellptr = c->get_block(c, sizeof(struct cell) + sizeof(value)*length);
  if (cellptr == NULL)
    return(CNIL);
  vect = (value)cellptr;
  BTYPE(vect) = T_VECTOR;
  REP(vect)._vector.length = length;
  memset(REP(vect)._vector.data, 0, sizeof(value)*length);
  return(vect);
}

value arc_rungc(arc *c)
{
  while (!c->rungc(c))
    ;
  return(CNIL);
}

/* Grow a vector, leaving nils where no objects are.  This will automatically
   release the space of the old vector.  If the vector is smaller than the
   original */
value arc_growvector(arc *c, value vect, int newlength)
{
  value newvect;
  int len;

  newvect = arc_mkvector(c, newlength);
  len = REP(vect)._vector.length;
  if (len > newlength)
    len = newlength;
  memcpy(REP(newvect)._vector.data, REP(vect)._vector.data, sizeof(value)*len);
  c->free_block(c, (void *)vect);
  return(newvect);
}

value arc_list_append(value list1, value val)
{
  value list;

  if (val == CNIL)
    return(list1);
  if (list1 == CNIL)
    return(val);
  list = list1;
  while (cdr(list) != CNIL)
    list = cdr(list);
  scdr(list, val);
  return(list1);
}

value arc_list_assoc(arc *c, value key, value a_list)
{
  value cc;

  while (a_list != CNIL) {
    cc = car(a_list);
    if (arc_is(c, car(cc), key) == CTRUE)
      return(cc);
    else
      a_list = cdr(a_list);
  }
  return(CNIL);
}

value arc_list_length(arc *c, value list)
{
  value n;

  n = INT2FIX(0);
  while (list != CNIL) {
    list = cdr(list);
    n = __arc_add2(c, n, INT2FIX(1));
  }
  return(n);
}

value arc_cmp(arc *c, value v1, value v2)
{
  if (!(NUMERIC_P(v1) && NUMERIC_P(v2)) && TYPE(v1) != TYPE(v2)) {
    arc_err_cstrfmt(c, "Invalid types for comparison");
    return(CNIL);
  }
  switch (TYPE(v1)) {
  case T_CHAR:
    /* XXX - should this change to honor LC_COLLATE someday? */
    return(arc_numcmp(c, INT2FIX(REP(v1)._char), INT2FIX(REP(v2)._char)));
  case T_FIXNUM:
  case T_FLONUM:
#ifdef HAVE_GMP_H
  case T_BIGNUM:
  case T_RATIONAL:
#endif
    return(arc_numcmp(c, v1, v2));
    break;
  case T_STRING:
    return(arc_strcmp(c, v1, v2));
    break;
  default:
    break;
  }
  arc_err_cstrfmt(c, "Invalid types for comparison");
  return(CNIL);
}

static value arc_gt2(arc *c, value v1, value v2)
{
  return((FIX2INT(arc_cmp(c, v1, v2)) > 0) ? CTRUE : CNIL);
}

static value arc_lt2(arc *c, value v1, value v2)
{
  return((FIX2INT(arc_cmp(c, v1, v2)) < 0) ? CTRUE : CNIL);
}

value arc_gt(arc *c, int argc, value *argv)
{
  value prev;
  int i;

  if (argc < 2)
    return(CTRUE);
  prev = argv[0];
  for (i=1; i<argc; i++) {
    if (arc_gt2(c, prev, argv[i]) == CNIL)
      return(CNIL);
    prev = argv[i];
  }
  return(CTRUE);
}

/* utility function */
value arc_list_reverse(arc *c, value xs)
{
  value acc = CNIL;

  while (xs != CNIL) {
    acc = cons(c, car(xs), acc);
    xs = cdr(xs);
  }
  return(acc);
}

value arc_lt(arc *c, int argc, value *argv)
{
  value prev;
  int i;

  if (argc < 2)
    return(CTRUE);
  prev = argv[0];
  for (i=1; i<argc; i++) {
    if (arc_lt2(c, prev, argv[i]) == CNIL)
      return(CNIL);
    prev = argv[i];
  }
  return(CTRUE);
}

value arc_bound(arc *c, value sym)
{
  TYPECHECK(sym, T_SYMBOL, 1);
  return((arc_hash_lookup(c, c->genv, sym) == CUNBOUND) ? CNIL: CTRUE);
}

value arc_sref(arc *c, value com, value val, value ind)
{
  switch (TYPE(com)) {
  case T_TABLE:
    if (val == CNIL)
      arc_hash_delete(c, com, ind);
    else
      arc_hash_insert(c, com, ind, val);
    break;
  case T_STRING:
    if (TYPE(val) != T_CHAR) {
      arc_err_cstrfmt(c, "cannot set string index to non-character");
      return(CNIL);
    }
    if (TYPE(ind) != T_FIXNUM || FIX2INT(ind) < 0) {
      arc_err_cstrfmt(c, "string index must be non-negative exact integer");
      return(CNIL);
    }
    arc_strsetindex(c, com, FIX2INT(ind), REP(val)._char);
    break;
  case T_CONS:
    if (TYPE(ind) != T_FIXNUM || FIX2INT(ind) < 0) {
      arc_err_cstrfmt(c, "list index must be non-negative exact integer");
      return(CNIL);
    } else {
      int idx = FIX2INT(ind), notfound = 1;
      value obj;

      for (obj=com; obj != CNIL; obj = cdr(obj), --idx) {
	if (idx == 0) {
	  scar(obj, val);
	  notfound = 0;
	  break;
	}
      }
      if (notfound) {
	arc_err_cstrfmt(c, "index %d too large for list", FIX2INT(ind));
	return(CNIL);
      }
    }
    break;
  case T_VECTOR:
    if (FIX2INT(ind) >= VECLEN(com)) {
      arc_err_cstrfmt(c, "index %d too large for vector", FIX2INT(ind));
      return(CNIL);
    }
    VINDEX(com, FIX2INT(ind)) = val;
    break;
  default:
    arc_err_cstrfmt(c, "can't set reference to object of type %d", TYPE(com));
  }
  return(val);
}

value arc_len(arc *c, value obj)
{
  switch (TYPE(obj)) {
  case T_NIL:
    /* Note that, oddly, PG-Arc returns 1 for this! */
    return(INT2FIX(0));
  case T_STRING:
    return(INT2FIX(arc_strlen(c, obj)));
  case T_CONS:
    return(arc_list_length(c, obj));
  case T_VECTOR:
    return(INT2FIX(VECLEN(obj)));
  case T_TABLE:
    return(INT2FIX(arc_hash_length(c, obj)));
  default:
    /* Note that PG-Arc also allows the length of symbols to be taken,
       but why this should be remains inexplicable to me. */
    arc_err_cstrfmt(c, "can't get length of object of type %d", TYPE(obj));
  }
  return(CNIL);
}

#define UNIQ_START_VAL 2874
#define UNIQ_PREFIX 'g'

value arc_uniq(arc *c)
{
  static unsigned long long uniqnum = UNIQ_START_VAL;
  char buffer[1024];

  snprintf(buffer, sizeof(buffer)/sizeof(char), "g%llu", uniqnum++);
  return(arc_intern_cstr(c, buffer));
}

value arc_car(arc *c, value v)
{
  if (NIL_P(v))
    return(CNIL);
  TYPECHECK(v, T_CONS, 1);
  return(car(v));
}

value arc_cdr(arc *c, value v)
{
  if (NIL_P(v))
    return(CNIL);
  TYPECHECK(v, T_CONS, 1);
  return(cdr(v));
}

value arc_cons(arc *c, value ca, value cd)
{
  return(cons(c, ca, cd));
}

value arc_scar(arc *c, value x, value y)
{
  TYPECHECK(x, T_CONS, 1);
  return(scar(x, y));
}

value arc_scdr(arc *c, value x, value y)
{
  TYPECHECK(x, T_CONS, 1);
  return(scdr(x, y));
}

#ifdef HAVE_TRACING

extern int vmtrace;

value arc_trace(arc *c)
{
  if (vmtrace) {
    printf("Tracing disabled\n");
    vmtrace = 0;
    return(CNIL);
  }
  printf("Tracing enabled\n");
  vmtrace = 1;
  return(CTRUE);
}

#endif

static struct {
  char *fname;
  int argc;
  value (*fnptr)();
} fntable[] = {
  { "type", 1, arc_type_compat },
  { "atype", 1, arc_type },
  { "coerce", -2, arc_coerce },
  { "annotate", 2, arc_annotate },
  { "rep", 1, arc_rep },

  { "code-setname", 2, arc_code_setname },

  /* Used when they don't appear in functional position.
     Inline definition will be used otherwise. */
  { "+", -1, __arc_add },
  { "-", -1, __arc_sub },
  { "*", -1, __arc_mul },
  { "/", -1, __arc_div },

  { ">", -1, arc_gt },
  { "<", -1, arc_lt },
  { "<=>", 2, arc_cmp },
  { "bound", 1, arc_bound },
  { "exact", 1, arc_exact },
  { "is", -1, arc_is_multi },
  { "iso", -1, arc_iso_multi },
  { "fixnump", 1, arc_fixnump },

  { "idiv", 2, __arc_idiv2 },
  { "expt", 2, arc_expt },
  { "pow", 2, arc_expt },
  { "mod", 2, __arc_mod2 },
  { "srand", 1, arc_srand },
  { "rand", -1, arc_rand },
  { "trunc", 1, arc_trunc },
  { "sqrt", 1, arc_sqrt },
  { "abs", 1, __arc_abs },
  { "real", 1, __arc_real },
  { "imag", 1, __arc_imag },
  { "conj", 1, __arc_conj },
  { "carg", 1, __arc_arg },

  { "acos", 1, __arc_acos },
  { "acosh", 1, __arc_acosh },
  { "asin", 1, __arc_asin },
  { "asinh", 1, __arc_asinh },
  { "atan", 1, __arc_atan },
  { "atan2", 2, __arc_atan2 },
  { "atanh", 1, __arc_atanh },
  { "cbrt", 1, __arc_cbrt },
  { "ceil", 1, __arc_ceil },
  { "cos", 1, __arc_cos },
  { "cosh", 1, __arc_cosh },
  { "erf", 1, __arc_erf },
  { "erfc", 1, __arc_erfc },
  { "exp", 1, __arc_exp },
  { "expm1", 1, __arc_expm1 },
  { "floor", 1, __arc_floor },
  { "fmod", 2, __arc_fmod },
  { "frexp", 1, __arc_frexp },
  { "hypot", 2, __arc_hypot },
  { "ldexp", 2, __arc_ldexp },
  { "lgamma", 1, __arc_lgamma },
  { "log", 1, __arc_log },
  { "log10", 1, __arc_log10 },
  { "log2", 1, __arc_log2 },
  { "logb", 1, __arc_logb },
  { "modf", 1, __arc_modf },
  { "nan", -1, __arc_nan },
  { "nearbyint", 1, __arc_nearbyint },
  { "sin", 1, __arc_sin },
  { "sinh", 1, __arc_sinh },
  { "tan", 1, __arc_tan },
  { "tanh", 1, __arc_tanh },
  { "tgamma", 1, __arc_tgamma },

  { "maptable", -3, arc_hash_map },
  { "table", 0, arc_table },

  { "apply", -3, arc_apply2 },
  { "eval", -3, arc_eval },
  { "ssyntax", 1, arc_ssyntax },
  { "ssexpand", 1, arc_ssexpand },

  { "macex", 1, arc_macex },
  { "macex1", 1, arc_macex1 },
  { "uniq", 0, arc_uniq },

  { "readb", -1, arc_screadb },
  { "readc", -1, arc_sreadc },
  { "ungetc", -1, arc_sungetc },
  { "peekc", -1, arc_speekc },
  { "sread", 2, arc_read },

  { "writeb", -1, arc_swriteb },
  { "writec", -1, arc_swritec },
  { "write", -1, arc_write },

  { "infile", 1, arc_infile },
  { "outfile", -1, arc_soutfile },
  { "close", -1, arc_sclose },
  /* XXX - should we treat this as something other than an alias? */
  { "force-close", -1, arc_sclose },

  { "instring", -1, arc_sinstring },
  { "outstring", -1, arc_soutstring },
  { "inside", 1, arc_fstr_inside },

  { "seek", 3, arc_seek },
  { "tell", 1, arc_tell },

  { "pipe-from", 1, arc_pipe_from },
  { "stdin", 0, arc_stdin },
  { "stdout", 0, arc_stdout },
  { "stderr", 0, arc_stderr },
  { "flushout", -1, arc_flushout },
  { "disp", -1, arc_disp },

  { "cgenctx", 2, arc_mkcctx},
  { "cptr", 2, arc_vcptr},

  { "current-gc-milliseconds", 0, arc_current_gc_milliseconds },
  { "current-process-milliseconds", 0, arc_current_process_milliseconds },
  { "gc", 0, arc_rungc },
  { "msec", 0, arc_msec },
  { "seconds", 0, arc_seconds },
  { "timedate", -1, arc_timedate },

  { "sref", 3, arc_sref },
  { "len", 1, arc_len },

  { "car", 1, arc_car },
  { "cdr", 1, arc_cdr },
  { "cons", 2, arc_cons },
  { "scar", 2, arc_scar },
  { "scdr", 2, arc_scdr },

  { "newstring", -1, arc_newstring },

  { "disasm", 1, arc_disasm },
#ifdef HAVE_TRACING
  { "trace", 0, arc_trace },
#endif

  { "err", -3, arc_err },
  { "on-err", -3, arc_on_err },
  { "details", 1, arc_exc_details },
  { "ccc", -3, arc_callcc },
  { "dynamic-wind", -3, arc_dynamic_wind },

  { "system", 1, arc_system },
  { "quit", -1, arc_quit },

  { "setuid", 1, arc_setuid },

  { "dir", 1, arc_dir },
  { "dir-exists", 1, arc_dir_exists },
  { "file-exists", 1, arc_file_exists },
  { "rmfile", 1, arc_rmfile },
  { "mvfile", 2, arc_mvfile },

  { "spawn", 1, arc_spawn },
  { "new-thread", 1, arc_spawn },
  { "sleep", 1, arc_sleep },
  { "chan", 0, arc_mkchan },
  { "<-", 1, arc_recv_channel },
  { "<-=", 2, arc_send_channel },
  { "dead", 1, arc_dead },
  { "join-thread", 1, arc_tjoin },
  { "kill-thread", 1, arc_kill_thread },
  { "break-thread", 1, arc_break_thread },

  /* used by atomic-invoke */
  { "__acell__", -1, arc_atomic_cell },
  { "__achan__", 0, arc_atomic_chan },

  { "open-socket", 1, arc_open_socket },
  { "socket-accept", 1, arc_socket_accept },
  { "client-ip", 1, arc_client_ip },

  { "sym", 1, arc_intern },

  { "cmark", 1, arc_cmark },
  { "scmark", 2, arc_scmark },
  { "ccmark", 1, arc_ccmark },

  { "memory", 0, arc_memory },

  { "declare", 2, arc_declare },

  { NULL, 0, NULL }
};

value arc_bindsym(arc *c, value sym, value binding)
{
  return(arc_hash_insert(c, c->genv, sym, binding));
}

value arc_bindcstr(arc *c, const char *csym, value binding)
{
  value sym = arc_intern_cstr(c, csym);
  return(arc_bindsym(c, sym, binding));
}

value arc_init_builtins(arc *c)
{
  int i;
  value cfunc;

  for (i=0; fntable[i].fname != NULL; i++) {
    value name = arc_intern_cstr(c, fntable[i].fname);
    cfunc = arc_mkccode(c, fntable[i].argc, fntable[i].fnptr, name);
    arc_bindsym(c, name, cfunc);
  }

  arc_bindsym(c, ARC_BUILTIN(c, S_NIL), CNIL);
  arc_bindsym(c, ARC_BUILTIN(c, S_T), CTRUE);
  arc_bindsym(c, ARC_BUILTIN(c, S_SIG), arc_mkhash(c, 12));

  /* bindings for stdin, stdout, and stderr */
  arc_bindsym(c, ARC_BUILTIN(c, S_STDIN_FD), arc_filefp(c, stdin, CNIL));
  arc_bindsym(c, ARC_BUILTIN(c, S_STDOUT_FD), arc_filefp(c, stdout, CNIL));
  arc_bindsym(c, ARC_BUILTIN(c, S_STDERR_FD), arc_filefp(c, stderr, CNIL));

  return(CNIL);
}

void arc_init_sq(arc *c, int stksize, int quanta, int gcquanta)
{
  arc_set_memmgr(c);
  c->genv = arc_mkhash(c, 8);
  arc_init_reader(c);
  arc_init_builtins(c);
  c->stksize = stksize;
  c->quantum = quanta;
  c->gcquantum = gcquanta;
  c->curthread = CNIL;
  c->vmthreads = CNIL;
  c->vmthrtail = CNIL;
  c->vmqueue = CNIL;
  c->in_compile = 0;
  c->iowaittbl = arc_mkhash(c, 8);
  c->achan = arc_mkchan(c);
  c->splforms = CNIL;
  c->inlfuncs = CNIL;
  c->gcstatus = 0;
  c->atstrings = 0;
}

void arc_init(arc *c)
{
  arc_init_sq(c, TSTKSIZE, PQUANTA, GCQUANTA);
}

value arc_declare(arc *c, value key, value val)
{
  if (key == arc_intern_cstr(c, "atstrings")) {
    c->atstrings = (NIL_P(val)) ? 0 : 1;
    return((c->atstrings) ? CTRUE : CNIL);
  }

  /* XXX - These declarations have no effect. */
  if (key == arc_intern_cstr(c, "direct-calls")
      || key == arc_intern_cstr(c, "explicit-flush")) {
    return((NIL_P(val)) ? CNIL : CTRUE);
  }
  arc_err_cstrfmt(c, "Unknown declaration");
  return(CNIL);
}
