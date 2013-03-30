/* 
  Copyright (C) 2013 Rafael R. Sevilla

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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <float.h>
#include <inttypes.h>
#include "arcueid.h"
#include "builtins.h"
#include "io.h"
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

void __arc_null_marker(arc *c, value v, int depth,
			void (*markfn)(arc *, value, int))
{
  /* Does nothing */
}

void __arc_null_sweeper(arc *c, value v)
{
  /* Does nothing */
}

value arc_mkobject(arc *c, size_t size, int type)
{
  struct cell *cc;

  cc = (struct cell *)c->alloc(c, sizeof(struct cell) + size - sizeof(value));
  cc->_type = type;
  return((value)cc);
}

/* compare with def of pairwise in ac.scm: tail-recursive version */
static AFFDEF(pairwise)
{
  AARG(pred, lst, vh1, vh2);
  AFBEGIN;

  for (;;) {
    if (NIL_P(AV(lst)))
      ARETURN(CTRUE);
    if (NIL_P(cdr(AV(lst))))
      ARETURN(CTRUE);
    AFCALL(AV(pred), car(AV(lst)), cadr(AV(lst)), AV(vh1), AV(vh2));
    if (NIL_P(AFCRV))
      ARETURN(CNIL);
    AV(lst) = cddr(AV(lst));
  }
  AFEND;
}
AFFEND

value arc_is2(arc *c, value a, value b)
{
  typefn_t *tfn;

  /* An object is definitely equivalent to itself */
  if (a == b)
    return(CTRUE);
  /* Two objects of different types cannot be equivalent */
  if (TYPE(a) != TYPE(b))
    return(CNIL);
  /* a == b check should have covered this, but just in case */
  if (IMMEDIATE_P(a))
    return(CNIL);
  /* Look for a type-specific shallow compare. If there is none,
     two objects of that type cannot be equivalent unless they
     are the same object. */
  tfn = __arc_typefn(c, a);
  if (tfn == NULL || tfn->iscmp == NULL)
    return(CNIL);
  /* Use the result given by iscmp */
  return(tfn->iscmp(c, a, b));
}

/* two-argument wrapper to arc_is2. vh1 and vh2 are not used. */
static AFFDEF(is2)
{
  AARG(a, b, vh1, vh2);
  AFBEGIN;
  ((void)vh1);
  ((void)vh2);
  ARETURN(arc_is2(c, AV(a), AV(b)));
  AFEND;
}
AFFEND


AFFDEF(arc_is)
{
  ARARG(list);
  AFBEGIN;
  AV(list) = CNIL;

  /* Tail call */
  AFTCALL(arc_mkaff(c, pairwise, CNIL), arc_mkaff(c, is2, CNIL),
	  AV(list), CNIL, CNIL);
  AFEND;
}
AFFEND

/* Two-argment iso. */
AFFDEF(arc_iso2)
{
  AARG(a, b, vh1, vh2);
  typefn_t *tfn;
  AFBEGIN;
  /* An object is isomorphic to itself */
  if (AV(a) == AV(b))
    return(CTRUE);

  /* Objects with different types cannot be isomorphic */
  if (TYPE(AV(a)) != TYPE(AV(b)))
    ARETURN(CNIL);

  /* a == b check should have covered this, but just in case */
  if (IMMEDIATE_P(AV(a)))
    ARETURN(CNIL);

  /* Go to the type-specific iso.  If there is none, they cannot
     be isomorphic. */
  tfn = __arc_typefn(c, AV(a));
  if (tfn == NULL)
    ARETURN(CNIL);
  /* Use a type-specific iso if available.  Use a type-specific
     iscmp if available.  If neither is available, then they cannot
     be compared.  */
  if (tfn->isocmp != NULL) {
    AFTCALL(arc_mkaff(c, tfn->isocmp, CNIL), AV(a), AV(b), AV(vh1), AV(vh2));
  } else if (tfn->iscmp != NULL) {
    ARETURN(tfn->iscmp(c, AV(a), AV(b)));
  }
  ARETURN(CNIL);
  AFEND;
}
AFFEND

AFFDEF(arc_iso)
{
  ARARG(list);
  AFBEGIN;
  /* Call pairwise with new visithashes */
  AFTCALL(arc_mkaff(c, pairwise, CNIL),
	  arc_mkaff(c, arc_iso2, CNIL),
	  AV(list),
	  arc_mkhash(c, ARC_HASHBITS),
	  arc_mkhash(c, ARC_HASHBITS));
  AFEND;
}
AFFEND

void arc_err_cstrfmt(arc *c, const char *fmt, ...)
{
  va_list ap;

  /* XXX - should do something more */
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  abort();
}

typefn_t *__arc_typefn(arc *c, value v)
{
  value typedesc;

  if (TYPE(v) != T_TAGGED)
    return(c->typefns[TYPE(v)]);
  /* For tagged types (custom types), the type descriptor hash should
     contain a type descriptor wrapped in a table. */
  typedesc = arc_hash_lookup(c, c->typedesc, car(v));
  /* no type descriptor defaults to the type descriptor of a cons */
  if (typedesc == CNIL)
    return(c->typefns[T_CONS]);
  return((typefn_t *)REP(typedesc));
}

value arc_type(arc *c, value obj)
{
  switch (TYPE(obj)) {
  case T_NIL:
  case T_TRUE:
  case T_SYMBOL:
    return(ARC_BUILTIN(c, S_SYM));
    break;
  case T_FIXNUM:
    return(ARC_BUILTIN(c, S_FIXNUM));
    break;
  case T_BIGNUM:
    return(ARC_BUILTIN(c, S_BIGNUM));
    break;
  case T_FLONUM:
    return(ARC_BUILTIN(c, S_FLONUM));
    break;
  case T_RATIONAL:
    return(ARC_BUILTIN(c, S_RATIONAL));
    break;
  case T_COMPLEX:
    return(ARC_BUILTIN(c, S_COMPLEX));
    break;
  case T_CHAR:
    return(ARC_BUILTIN(c, S_CHAR));
    break;
  case T_STRING:
    return(ARC_BUILTIN(c, S_STRING));
    break;
  case T_CONS:
    return(ARC_BUILTIN(c, S_CONS));
    break;
  case T_TABLE:
    return(ARC_BUILTIN(c, S_TABLE));
    break;
  case T_TAGGED:
    /* A tagged object created by annotate has as its car the type
       as a symbol */
    return(car(obj));
    break;
  case T_INPORT:
    return(ARC_BUILTIN(c, S_INPUT));
    break;
  case T_OUTPORT:
    return(ARC_BUILTIN(c, S_OUTPUT));
    break;
  case T_EXCEPTION:
    return(ARC_BUILTIN(c, S_EXCEPTION));
    break;
  case T_THREAD:
    return(ARC_BUILTIN(c, S_THREAD));
    break;
  case T_VECTOR:
    return(ARC_BUILTIN(c, S_VECTOR));
    break;
  case T_CONT:
    return(ARC_BUILTIN(c, S_CONTINUATION));
    break;
  case T_CLOS:
    return(ARC_BUILTIN(c, S_FN));
    break;
  case T_CODE:
    return(ARC_BUILTIN(c, S_CODE));
    break;
  case T_ENV:
    return(ARC_BUILTIN(c, S_ENVIRONMENT));
    break;
  case T_CCODE:
    return(ARC_BUILTIN(c, S_CCODE));
    break;
  case T_CUSTOM:
    return(ARC_BUILTIN(c, S_CUSTOM));
    break;
  case T_CHAN:
    return(ARC_BUILTIN(c, S_CHAN));
    break;
  default:
    break;
  }
  return(ARC_BUILTIN(c, S_UNKNOWN));
}

/* Arc3-compatible type */
value arc_type_compat(arc *c, value obj)
{
  switch (TYPE(obj)) {
  case T_NIL:
  case T_TRUE:
  case T_SYMBOL:
    return(ARC_BUILTIN(c, S_SYM));
    break;
  case T_FIXNUM:
  case T_BIGNUM:
    return(ARC_BUILTIN(c, S_INT));
    break;
  case T_FLONUM:
  case T_RATIONAL:
  case T_COMPLEX:
    return(ARC_BUILTIN(c, S_NUM));
    break;
  case T_CHAR:
    return(ARC_BUILTIN(c, S_CHAR));
    break;
  case T_STRING:
    return(ARC_BUILTIN(c, S_STRING));
    break;
  case T_CONS:
    return(ARC_BUILTIN(c, S_CONS));
    break;
  case T_TABLE:
    return(ARC_BUILTIN(c, S_TABLE));
    break;
  case T_TAGGED:
    /* A tagged object created by annotate has as its car the type
       as a symbol */
    return(car(obj));
    break;
  case T_INPORT:
    return(ARC_BUILTIN(c, S_INPUT));
    break;
  case T_OUTPORT:
    return(ARC_BUILTIN(c, S_OUTPUT));
    break;
  case T_EXCEPTION:
    return(ARC_BUILTIN(c, S_EXCEPTION));
    break;
  case T_THREAD:
    return(ARC_BUILTIN(c, S_THREAD));
    break;
  case T_VECTOR:
    return(ARC_BUILTIN(c, S_VECTOR));
    break;
  case T_CONT:
    return(ARC_BUILTIN(c, S_CONTINUATION));
    break;
  case T_CLOS:
    return(ARC_BUILTIN(c, S_FN));
    break;
  case T_CODE:
    return(ARC_BUILTIN(c, S_FN));
    break;
  case T_ENV:
    return(ARC_BUILTIN(c, S_ENVIRONMENT));
    break;
  case T_CCODE:
    return(ARC_BUILTIN(c, S_FN));
    break;
  case T_CUSTOM:
    return(ARC_BUILTIN(c, S_CUSTOM));
    break;
  case T_CHAN:
    return(ARC_BUILTIN(c, S_CHAN));
    break;
  default:
    break;
  }
  return(ARC_BUILTIN(c, S_UNKNOWN));
}

value arc_rep(arc *c, value obj)
{
  if (TYPE(obj) != T_TAGGED)
    return(obj);
  return(cdr(obj));
}

value arc_annotate(arc *c, value typesym, value obj)
{
  value ann;

  /* Do not re-tag something with the same typesym */
  if (TYPE(obj) == T_TAGGED && arc_is2(c, car(obj), typesym) == CTRUE)
    return(obj);

  ann = cons(c, typesym, obj);
  ((struct cell *)ann)->_type = T_TAGGED;
  return(ann);
}

enum arc_types typesym2type(arc *c, value typesym)
{
  if (typesym == ARC_BUILTIN(c, S_SYM))
    return(T_SYMBOL);

  if (typesym == ARC_BUILTIN(c, S_FIXNUM))
    return(T_FIXNUM);

  if (typesym == ARC_BUILTIN(c, S_BIGNUM) || typesym == ARC_BUILTIN(c, S_INT))
    return(T_BIGNUM);

  if (typesym == ARC_BUILTIN(c, S_FLONUM))
    return(T_FLONUM);

  if (typesym == ARC_BUILTIN(c, S_RATIONAL))
    return(T_RATIONAL);

  if (typesym == ARC_BUILTIN(c, S_COMPLEX))
    return(T_COMPLEX);

  if (typesym == ARC_BUILTIN(c, S_CHAR))
    return(T_CHAR);

  if (typesym == ARC_BUILTIN(c, S_STRING))
    return(T_STRING);

  if (typesym == ARC_BUILTIN(c, S_CONS))
    return(T_CONS);

  if (typesym == ARC_BUILTIN(c, S_TABLE))
    return(T_TABLE);

  if (typesym == ARC_BUILTIN(c, S_INPUT))
    return(T_INPORT);

  if (typesym == ARC_BUILTIN(c, S_OUTPUT))
    return(T_OUTPORT);

  if (typesym == ARC_BUILTIN(c, S_EXCEPTION))
    return(T_EXCEPTION);

  if (typesym == ARC_BUILTIN(c, S_THREAD))
    return(T_THREAD);

  if (typesym == ARC_BUILTIN(c, S_VECTOR))
    return(T_VECTOR);

  if (typesym == ARC_BUILTIN(c, S_CONTINUATION))
    return(T_CONT);

  if (typesym == ARC_BUILTIN(c, S_FN))
    return(T_CLOS);

  if (typesym == ARC_BUILTIN(c, S_CODE))
    return(T_CODE);

  if (typesym == ARC_BUILTIN(c, S_ENVIRONMENT))
    return(T_ENV);

  if (typesym == ARC_BUILTIN(c, S_CCODE))
    return(T_CCODE);

  if (typesym == ARC_BUILTIN(c, S_CHAN))
    return(T_CHAN);

  return(T_NONE);
}

AFFDEF(arc_coerce)
{
  AARG(obj, typesym);
  AOARG(args);
  typefn_t *tfn;
  AFBEGIN;

  if (NIL_P(AV(obj)) && AV(typesym) == ARC_BUILTIN(c, S_STRING))
    ARETURN(arc_mkstringc(c, ""));

  tfn = __arc_typefn(c, AV(obj));
  if (tfn == NULL || tfn->xcoerce == NULL) {
    arc_err_cstrfmt(c, "cannot coerce");
    ARETURN(AV(obj));
  }
  AFTCALL(arc_mkaff(c, tfn->xcoerce, CNIL), AV(obj),
	  INT2FIX(typesym2type(c, AV(typesym))), AV(args));
  AFEND;
}
AFFEND

extern typefn_t __arc_fixnum_typefn__;
extern typefn_t __arc_flonum_typefn__;
extern typefn_t __arc_complex_typefn__;
extern typefn_t __arc_string_typefn__;
extern typefn_t __arc_char_typefn__;

#ifdef HAVE_GMP_H
extern typefn_t __arc_bignum_typefn__;
extern typefn_t __arc_rational_typefn__;
#endif

extern typefn_t __arc_cons_typefn__;
extern typefn_t __arc_table_typefn__;
extern typefn_t __arc_hb_typefn__;
extern typefn_t __arc_wtable_typefn__;
extern typefn_t __arc_code_typefn__;
extern typefn_t __arc_io_typefn__; /* used for both T_INPORT and T_OUTPORT */
extern typefn_t __arc_thread_typefn__;
extern typefn_t __arc_vector_typefn__;
extern typefn_t __arc_cfunc_typefn__;
extern typefn_t __arc_cont_typefn__;
extern typefn_t __arc_clos_typefn__;

void arc_init_datatypes(arc *c)
{
  c->typefns[T_FIXNUM] = &__arc_fixnum_typefn__;
  c->typefns[T_FLONUM] = &__arc_flonum_typefn__;
  c->typefns[T_COMPLEX] = &__arc_complex_typefn__;
  c->typefns[T_STRING] = &__arc_string_typefn__;
  c->typefns[T_CHAR] = &__arc_char_typefn__;

#ifdef HAVE_GMP_H
  c->typefns[T_BIGNUM] = &__arc_bignum_typefn__;
  c->typefns[T_RATIONAL] = &__arc_rational_typefn__;
#endif

  c->typefns[T_CONS] = &__arc_cons_typefn__;
  c->typefns[T_TABLE] = &__arc_table_typefn__;
  c->typefns[T_TBUCKET] = &__arc_hb_typefn__;
  c->typefns[T_INPORT] = &__arc_io_typefn__;
  c->typefns[T_OUTPORT] = &__arc_io_typefn__;
  c->typefns[T_THREAD] = &__arc_thread_typefn__;
  c->typefns[T_VECTOR] = &__arc_vector_typefn__;

  c->typefns[T_WTABLE] = &__arc_wtable_typefn__;
  c->typefns[T_CCODE] = &__arc_cfunc_typefn__;
  c->typefns[T_CONT] = &__arc_cont_typefn__;
  c->typefns[T_CLOS] = &__arc_clos_typefn__;
}


static struct {
  char *fname;
  int argc;
  void *fnptr;
} fntable[] = {
  { "type", 1, arc_type_compat },
  { "atype", 1, arc_type },
  { "annotate", 2, arc_annotate },
  { "rep", 1, arc_rep },
  { "coerce", -2, arc_coerce },
  {NULL, 0, NULL }
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

void arc_init_builtins(arc *c)
{
  int i;
  value cfunc;

  for (i=0; fntable[i].fname != NULL; i++) {
    value name = arc_intern_cstr(c, fntable[i].fname);
    if (fntable[i].argc == -2)
      cfunc = arc_mkaff(c, fntable[i].fnptr, name);
    else
      cfunc = arc_mkccode(c, fntable[i].argc, fntable[i].fnptr, name);
    arc_bindsym(c, name, cfunc);
  }

  arc_bindsym(c, ARC_BUILTIN(c, S_NIL), CNIL);
  arc_bindsym(c, ARC_BUILTIN(c, S_T), CTRUE);
  arc_bindsym(c, ARC_BUILTIN(c, S_SIG), arc_mkhash(c, 12));
}

void arc_init(arc *c)
{
  /* Initialise memory manager first */
  arc_init_memmgr(c);
  /* Initialise built-in data type definitions */
  arc_init_datatypes(c);

  /* Create global environment and type descriptor table */
  c->genv = arc_mkhash(c, ARC_HASHBITS);
  c->typedesc = arc_mkhash(c, ARC_HASHBITS);

  /* Create builtins table */
  c->builtins = arc_mkvector(c, BI_last+1);

  /* Initialise symbol table and built-in symbols*/
  arc_init_symtable(c);

  /* Initialise thread system */
  arc_init_threads(c);

  /* Initialise I/O subsystem */
  arc_init_io(c);

  /* Initialise builtin functions */
  arc_init_builtins(c);

}
