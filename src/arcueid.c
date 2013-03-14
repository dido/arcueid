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

/* compare with def of pairwise in ac.scm */
static AFFDEF(pairwise, pred, lst, vh1, vh2)
{
  AVAR(self);
  AFBEGIN;

  AV(self) = arc_mkaff(c, pairwise, CNIL);
  if (NIL_P(AV(lst)))
    ARETURN(CTRUE);
  if (NIL_P(cdr(AV(lst))))
    ARETURN(CTRUE);
  AFCALL(AV(pred), car(AV(lst)), cadr(AV(lst)), AV(vh1), AV(vh2));
  if (NIL_P(AFCRV))
    ARETURN(CNIL);
  AFCALL(AV(self), AV(pred), cddr(AV(lst)));
  ARETURN(AFCRV);
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
static AFFDEF(is2, a, b, vh1, vh2)
{
  AFBEGIN;
  ((void)vh1);
  ((void)vh2);
  ARETURN(arc_is2(c, AV(a), AV(b)));
  AFEND;
}
AFFEND


AFFDEF0(arc_is)
{
  int argc;
  AVAR(list, fis2, pw);
  AFBEGIN;
  AV(list) = CNIL;
  argc = arc_thr_argc(c, thr);
  while (--argc >= 0)
    AV(list) = cons(c, arc_thr_pop(c, thr), AV(list));
  AV(fis2) = arc_mkaff(c, is2, CNIL);
  AV(pw) = arc_mkaff(c, pairwise, CNIL);
  AFCALL(AV(pw), AV(fis2), AV(list), CNIL, CNIL);
  ARETURN(AFCRV);
  AFEND;
}
AFFEND

/* Two-argment iso. */
AFFDEF(arc_iso2, a, b, vh1, vh2)
{
  typefn_t *tfn;
  AVAR(isop);
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
    AV(isop) = arc_mkaff(c, tfn->isocmp, CNIL);
    AFCALL(AV(isop), AV(a), AV(b), AV(vh1), AV(vh2));
    ARETURN(AFCRV);
  } else if (tfn->iscmp != NULL) {
    ARETURN(tfn->iscmp(c, AV(a), AV(b)));
  }
  ARETURN(CNIL);
  AFEND;
}
AFFEND

AFFDEF0(arc_iso)
{
  int argc;
  AVAR(list, fiso2, pw);
  AFBEGIN;
  AV(list) = CNIL;
  argc = arc_thr_argc(c, thr);
  while (--argc >= 0)
    AV(list) = cons(c, arc_thr_pop(c, thr), AV(list));
  AV(fiso2) = arc_mkaff(c, arc_iso2, CNIL);
  AV(pw) = arc_mkaff(c, pairwise, CNIL);
  /* Call pairwise with new visithashes */
  AFCALL(AV(pw), AV(fiso2), AV(list),
	 arc_mkhash(c, ARC_HASHBITS),
	 arc_mkhash(c, ARC_HASHBITS));
  ARETURN(AFCRV);
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
  if (typedesc == CNIL)
    return(NULL);		/* XXX should this be an error? */
  return((typefn_t *)REP(typedesc));
}


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
}
