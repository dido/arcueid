/* 
  Copyright (C) 2010 Rafael R. Sevilla

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
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA
  02110-1301 USA.
*/
/* miscellaneous procedures and initialization */
#include <stdlib.h>
#include <string.h>
#include "arcueid.h"
#include "alloc.h"
#include "arith.h"
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
  }
  return(CNIL);
}

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
  }
  return(CNIL);
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
  memcpy(REP(vect)._vector.data, REP(newvect)._vector.data, len);
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

/* Create a tagged object.  Type should be a symbol, and rep
   should be the representation.  This is most commonly used
   for macros. */
value arc_tag(arc *c, value type, value rep)
{
  value tag;

  tag = cons(c, type, rep);
  BTYPE(tag) = T_TAGGED;
  return(tag);
}

value arc_cmp(arc *c, value v1, value v2)
{
  if (TYPE(v1) != TYPE(v2)) {
    c->signal_error(c, "Invalid types for comparison");
    return(CNIL);
  }
  switch (TYPE(v1)) {
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
  }
  c->signal_error(c, "Invalid types for comparison");
  return(CNIL);
}

value arc_gt(arc *c, value v1, value v2)
{
  return((FIX2INT(arc_cmp(c, v1, v2)) > 0) ? CTRUE : CNIL);
}

value arc_lt(arc *c, value v1, value v2)
{
  return((FIX2INT(arc_cmp(c, v1, v2)) < 0) ? CTRUE : CNIL);
}

value arc_gte(arc *c, value v1, value v2)
{
  return((FIX2INT(arc_cmp(c, v1, v2)) >= 0) ? CTRUE : CNIL);
}

value arc_lte(arc *c, value v1, value v2)
{
  return((FIX2INT(arc_cmp(c, v1, v2)) <= 0) ? CTRUE : CNIL);
}

value arc_bound(arc *c, value sym)
{
  return((arc_hash_lookup(c, c->genv, sym) == CUNBOUND) ? CNIL: CTRUE);
}

static struct {
  char *name;
  value sym;
} typesyms[] = {
  { "sym", CNIL },
  { "fixnum", CNIL },
  { "bignum", CNIL },
  { "flonum", CNIL },
  { "rational", CNIL },
  { "complex", CNIL },
  { "char", CNIL },
  { "string", CNIL },
  { "cons", CNIL },
  { "table", CNIL },
  { "input", CNIL },
  { "output", CNIL },
  { "exception", CNIL },
  { "port", CNIL },
  { "thread", CNIL },
  { "vector", CNIL },
  { "continuation", CNIL },
  { "closure", CNIL },
  { "code", CNIL },
  { "environment", CNIL },
  { "vmcode", CNIL },
  { "ccode", CNIL },
  { "custom", CNIL },
  { "int", CNIL },
  { "unknown", CNIL },
  { "re", CNIL },
  { "im", CNIL },
  { NULL, CNIL }
};

enum {
  IDX_sym=0,
  IDX_fixnum,
  IDX_bignum,
  IDX_flonum,
  IDX_rational,
  IDX_complex,
  IDX_char,
  IDX_string,
  IDX_cons,
  IDX_table,
  IDX_input,
  IDX_output,
  IDX_exception,
  IDX_port,
  IDX_thread,
  IDX_vector,
  IDX_continuation,
  IDX_closure,
  IDX_code,
  IDX_environment,
  IDX_vmcode,
  IDX_ccode,
  IDX_custom,
  IDX_int,
  IDX_unknown,
  IDX_re,
  IDX_im
};

value arc_type(arc *c, value obj)
{
  switch (TYPE(obj)) {
  case T_NIL:
  case T_TRUE:
  case T_SYMBOL:
    return(typesyms[IDX_sym].sym);
    break;
  case T_FIXNUM:
    return(typesyms[IDX_fixnum].sym);
    break;
  case T_BIGNUM:
    return(typesyms[IDX_bignum].sym);
    break;
  case T_FLONUM:
    return(typesyms[IDX_flonum].sym);
    break;
  case T_RATIONAL:
    return(typesyms[IDX_rational].sym);
    break;
  case T_COMPLEX:
    return(typesyms[IDX_complex].sym);
    break;
  case T_CHAR:
    return(typesyms[IDX_char].sym);
    break;
  case T_STRING:
    return(typesyms[IDX_string].sym);
    break;
  case T_CONS:
    return(typesyms[IDX_cons].sym);
    break;
  case T_TABLE:
    return(typesyms[IDX_cons].sym);
    break;
  case T_TAGGED:
    /* A tagged object created by annotate has as its car the type
       as a symbol */
    return(car(obj));
    break;
  case T_INPUT:
    return(typesyms[IDX_input].sym);
    break;
  case T_EXCEPTION:
    return(typesyms[IDX_exception].sym);
    break;
  case T_PORT:
    return(typesyms[IDX_port].sym);
    break;
  case T_THREAD:
    return(typesyms[IDX_thread].sym);
    break;
  case T_VECTOR:
    return(typesyms[IDX_vector].sym);
    break;
  case T_CONT:
    return(typesyms[IDX_continuation].sym);
    break;
  case T_CLOS:
    return(typesyms[IDX_closure].sym);
    break;
  case T_CODE:
    return(typesyms[IDX_code].sym);
    break;
  case T_ENV:
    return(typesyms[IDX_environment].sym);
    break;
  case T_VMCODE:
    return(typesyms[IDX_vmcode].sym);
    break;
  case T_CCODE:
    return(typesyms[IDX_ccode].sym);
    break;
  case T_CUSTOM:
    return(typesyms[IDX_custom].sym);
    break;
  default:
    break;
  }
  return(typesyms[IDX_unknown].sym);
}

static value coerce_integer(arc *c, value obj, value argv)
{
  value res;

  switch (TYPE(obj)) {
  case T_FIXNUM:
  case T_BIGNUM:
    /* Null "conversions" */
    return(obj);
  case T_CHAR:
    return(INT2FIX(REP(obj)._char));
    break;
  case T_COMPLEX:
    obj = (VECLEN(argv) >= 3 && VINDEX(argv, 2) == typesyms[IDX_im].sym) ?
      arc_mkflonum(c, REP(obj)._complex.im)
      : arc_mkflonum(c, REP(obj)._complex.re);
    /* fall through */
  case T_FLONUM:
#ifdef HAVE_GMP_H
  case T_RATIONAL:
    {
      value fnres;

      res = arc_mkbignuml(c, 0);
      arc_coerce_bignum(c, obj, REP(res)._bignum);
      fnres = arc_coerce_fixnum(c, res);
      return((fnres == CNIL) ? res : fnres);
    }
#else
    res = arc_coerce_fixnum(c, obj);
    if (res == CNIL)
      res = INT2FIX(FIXNUM_MAX);
    return(res);
#endif
  case T_STRING:
  default:
    c->signal_error(c, "cannot coerce %O to integer type", obj);
    break;
  }
  return(CNIL);
}

value arc_coerce(arc *c, value argv)
{
  value obj, ntype;

  if (VECLEN(argv) < 2) {
    c->signal_error(c, "too few arguments to coerce");
    return(CNIL);
  }

  obj = VINDEX(argv, 0);
  ntype = VINDEX(argv, 1);
  /* Integer coercions */
  if (ntype == typesyms[IDX_fixnum].sym
      || ntype == typesyms[IDX_bignum].sym
      || ntype == typesyms[IDX_int].sym) {
    return(coerce_integer(c, obj, argv));
  }
  c->signal_error(c, "invalid coercion type specifier %O", ntype);
  return(CNIL);
}

static struct {
  char *fname;
  int argc;
  value (*fnptr)();
} fntable[] = {
  { "type", 1, arc_type },
  { "coerce", -2, arc_coerce },
  { ">", 2, arc_gt },
  { "<", 2, arc_lt },
  { ">=", 2, arc_gte },
  { "<=", 2, arc_lte },
  { "<=>", 2, arc_cmp },
  { "bound", 1, arc_bound },
  { "exact", 1, arc_exact },
  { "is", 2, arc_is },
  { "iso", 2, arc_iso },

  { "expt", 2, arc_expt },
  { "pow", 2, arc_expt },
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
    cfunc = arc_mkccode(c, fntable[i].argc, fntable[i].fnptr);
    arc_bindcstr(c, fntable[i].fname, cfunc);
  }
  for (i=0; typesyms[i].name != NULL; i++)
    typesyms[i].sym = arc_intern_cstr(c, typesyms[i].name);
  return(CNIL);
}
