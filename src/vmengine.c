/* 
  Copyright (C) 2012 Rafael R. Sevilla

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <setjmp.h>
#include <errno.h>
#include "../config.h"
#include "arcueid.h"
#include "vmengine.h"
#include "alloc.h"
#include "arith.h"
#include "symbols.h"
#include "io.h"
#include "builtin.h"

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

#ifdef HAVE_TRACING

int vmtrace = 0;

void arc_restorecont(arc *c, value thr, value cont);

static void printobj(arc *c, value obj)
{
  arc_print_string(c, arc_prettyprint(c, obj));
  if (obj != CNIL) {
    printf("<");
    arc_print_string(c, arc_prettyprint(c, arc_type(c, obj)));
    printf(">");
  }
}

static void dump_env(arc *c, value env)
{
  value enames = car(ENV_NAMES(env)), esym;
  int nvals = arc_hash_length(c, enames) / 2; /* mapping + reverse mapping */
  int i;

  printf("{");
  for (i=0; i<nvals; i++) {
    esym = arc_hash_lookup(c, enames, INT2FIX(i));
    arc_print_string(c, arc_prettyprint(c, esym));
    printf("(%d) => ", i);
    printobj(c, ENV_VALUE(env, i));
    printf(" ");
  }
  printf("}");
}

static void dump_registers(arc *c, value thr)
{
  value *sv, env;
  int envidx;

  printf("VALR = ");
  printobj(c, TVALR(thr));

  printf("\t\tFUNR = ");
  arc_print_string(c, arc_prettyprint(c, TFUNR(thr)));

  printf("\nstack = [ ");
  for (sv = TSTOP(thr); sv != TSP(thr); sv--) {
    printobj(c, *sv);
    printf(" ");
  }
  printf("]\n");  
  env = TENVR(thr);
  envidx = 0;
  while (env != CNIL) {
    printf("Env %d: ", envidx);
    dump_env(c, car(env));
    env = cdr(env);
    envidx++;
    printf("\n");
  }
}

static inline void trace(arc *c, value thr)
{
  static value *bpofs = NULL;		/* breakpoint offset */
  static value bp = CNIL;	/* breakpoint function */
  char str[256];

  if (bp == TFUNR(thr) && bpofs == TIP(thr)) {
    bp = CNIL;
    bpofs = NULL;
  } else if (bp != CNIL) {
    return;
  }
  dump_registers(c, thr);
  arc_disasm_inst(c, TIP(thr) - &VINDEX(VINDEX(TFUNR(thr), 0), 0), TIP(thr),
		  TFUNR(thr));
  printf("\n- ");
  fgets(str, 256, stdin);
  if (str[0] == 'n') {
    value cont;

    if (TCONR(thr) == CNIL)
      return;
    cont = car(TCONR(thr));
    /* create a breakpoint at the next continuation, if any */
    if (cont == CNIL || TYPE(VINDEX(cont, 0)) == T_XCONT)
      return;
    bp = VINDEX(cont, 1);
    bpofs = &VINDEX(VINDEX(bp, 0), FIX2INT(VINDEX(cont, 0)));
  }
}

#endif

/* instruction decoding macros */
#ifdef HAVE_THREADED_INTERPRETER
/* threaded interpreter */
#define INST(name) lbl_##name
#define JTBASE ((void *)&&lbl_inop)
#ifdef HAVE_TRACING
#define NEXT {							\
    if (--TQUANTA(thr) <= 0 || !(RUNNABLE(thr)))		\
      goto endquantum;						\
    if (vmtrace)						\
      trace(c, thr);						\
    goto *(JTBASE + jumptbl[*TIP(thr)++]); }
#else
#define NEXT {							\
    if (--TQUANTA(thr) <= 0 || !(RUNNABLE(thr)))		\
      goto endquantum;						\
    goto *(JTBASE + jumptbl[*TIP(thr)++]); }
#endif

#else
/* switch interpreter */
#define INST(name) case name
#define NEXT break
#endif

#define CPUSH(thr, val) (*(TSP(thr)--) = (val))
#define CPOP(thr) (*(++TSP(thr)))

void arc_vmengine(arc *c, value thr, int quanta)
{
#ifdef HAVE_THREADED_INTERPRETER
  static const int jumptbl[] = {
#include "jumptbl.h"
  };
#else
  value curr_instr;
#endif

  /* If the setjmp returns 2, apply whatever is in the value register. */
  if (setjmp(TVJMP(thr)) == 2) {
    arc_apply(c, thr, TVALR(thr));
  }

  if (!NIL_P(TEXC(thr))) {
    value exc = TEXC(thr);

    WB(&TEXC(thr), CNIL);
    arc_errexc(c, exc);
  }

  if (!RUNNABLE(thr))
    return;

  TQUANTA(thr) = quanta;

#ifdef HAVE_THREADED_INTERPRETER
#ifdef HAVE_TRACING
  if (vmtrace)
    trace(c, thr);
#endif
  goto *(void *)(JTBASE + jumptbl[*TIP(thr)++]);
#else
  for (;;) {
    if (--TQUANTA(thr) <= 0 || (!RUNNABLE(thr)))
      goto endquantum;
    curr_instr = *TIP(thr)++;
    switch (curr_instr) {
#endif
    INST(inop):
      NEXT;
    INST(ipush):
      CPUSH(thr, TVALR(thr));
      NEXT;
    INST(ipop):
      WB(&TVALR(thr), CPOP(thr));
      NEXT;
    INST(ildi):
      WB(&TVALR(thr), *TIP(thr)++);
      NEXT;
    INST(ildl):
      WB(&TVALR(thr), CODE_LITERAL(TFUNR(thr), *TIP(thr)++));
      NEXT;
    INST(ildg):
      {
	value tmp, tmpstr;
	char *cstr;

	tmp = CODE_LITERAL(TFUNR(thr), *TIP(thr)++);
	WB(&TVALR(thr), arc_hash_lookup(c, c->genv, tmp));
	if (TVALR(thr) == CUNBOUND) {
	  tmpstr = arc_sym2name(c, tmp);
	  cstr = alloca(sizeof(char)*(FIX2INT(arc_strutflen(c, tmpstr)) + 1));
	  arc_str2cstr(c, tmpstr, cstr);
	  /* arc_print_string(c, arc_prettyprint(c, tmp)); printf("\n"); */
	  arc_err_cstrfmt(c, "Unbound symbol: _%s", cstr);
	  WB(&TVALR(thr), CNIL);
	}
      }
      NEXT;
    INST(istg):
      arc_hash_insert(c, c->genv, CODE_LITERAL(TFUNR(thr), *TIP(thr)++),
		      TVALR(thr));
      NEXT;
    INST(ilde):
      {
	value iindx, tmp;
	int ienv;

	ienv = (int)*TIP(thr)++;
	iindx = *TIP(thr)++;
	tmp = TENVR(thr);
	while (--ienv >= 0)
	  tmp = cdr(tmp);
	WB(&TVALR(thr), ENV_VALUE(car(tmp), iindx));
      }
      NEXT;
    INST(iste):
      {
	value iindx, tmp;
	int ienv;

	ienv = (int)*TIP(thr)++;
	iindx = *TIP(thr)++;
	tmp = TENVR(thr);
	while (--ienv >= 0)
	  tmp = cdr(tmp);
	WB(&ENV_VALUE(car(tmp), iindx), TVALR(thr));
      }
      NEXT;
    INST(imvarg):
      {
	int iindx = (int)*TIP(thr)++;
	if (TSP(thr) == TSTOP(thr))
	  arc_err_cstrfmt(c, "too few arguments");
	WB(&ENV_VALUE(car(TENVR(thr)), iindx), CPOP(thr));
      }
      NEXT;
    INST(imvoarg):
      {
	int iindx = (int)*TIP(thr)++;
	value arg = (TSP(thr) == TSTOP(thr)) ? CNIL : CPOP(thr);

	WB(&ENV_VALUE(car(TENVR(thr)), iindx), arg);
      }
      NEXT;
    INST(imvrarg):
      {
	int iindx = (int)*TIP(thr)++;
	value list = CNIL, i, rlist = CNIL;

	while (TSP(thr) != TSTOP(thr)) {
	  i = CPOP(thr);
	  list = cons(c, i, list);
	}
	/* The list of popped arguments is in reverse order.  We
	   need to reverse it to make it come out right. */
	rlist = arc_list_reverse(c, list);
	WB(&ENV_VALUE(car(TENVR(thr)), iindx), rlist);
      }
      NEXT;
    INST(icont):
      {
	int icofs = (int)*TIP(thr)++;
	WB(&TCONR(thr), cons(c, arc_mkcont(c, INT2FIX(icofs), thr),
			       TCONR(thr)));
	TSP(thr) = TSTOP(thr);
      }
      NEXT;
    INST(ienv):
      {
	int ienvsize = *TIP(thr)++;
	int namesidx = *TIP(thr)++;

	WB(&TENVR(thr), arc_mkenv(c, TENVR(thr), ienvsize));
	WB(&ENV_NAMES(car(TENVR(thr))), (namesidx < 0) ? CNIL
	   : CODE_LITERAL(TFUNR(thr), namesidx));
	/* WB(&VINDEX(car(TENVR(thr)), 0), VINDEX(TFUNR(thr), 2)); */
      }
      NEXT;
    INST(iapply):
      {
	int argc = *TIP(thr)++;
	TARGC(thr) = argc;
	arc_apply(c, thr, TVALR(thr));
      }
      NEXT;
    INST(iret):
      /* If the continuation register is empty, we want to immediately
	 release the thread, making the ret instruction behave just
	 like a hlt instruction. */
      if (arc_return(c, thr)) {
	TSTATE(thr) = Trelease;
	goto endquantum;
      }
      NEXT;
    INST(ijmp):
      {
	int itarget = *TIP(thr)++;
	TIP(thr) += itarget-2;
      }
      NEXT;
    INST(ijt):
      {
	int itarget = *TIP(thr)++;
	if (TVALR(thr) != CNIL)
	  TIP(thr) += itarget-2;
      }
      NEXT;
    INST(ijf):
      {
	int itarget = *TIP(thr)++;
	if (TVALR(thr) == CNIL)
	  TIP(thr) += itarget-2;
      }
      NEXT;
    INST(itrue):
      WB(&TVALR(thr), CTRUE);
      NEXT;
    INST(inil):
      WB(&TVALR(thr), CNIL);
      NEXT;
    INST(ihlt):
      TSTATE(thr) = Trelease;
      goto endquantum;
      NEXT;
    INST(iadd):
      WB(&TVALR(thr), __arc_add2(c, TVALR(thr), CPOP(thr)));
      NEXT;
    INST(isub):
      WB(&TVALR(thr), __arc_sub2(c, CPOP(thr), TVALR(thr)));
      NEXT;
    INST(imul):
      WB(&TVALR(thr),  __arc_mul2(c, TVALR(thr), CPOP(thr)));
      NEXT;
    INST(idiv):
      WB(&TVALR(thr), __arc_div2(c, CPOP(thr), TVALR(thr)));
      NEXT;
    INST(icons):
      WB(&TVALR(thr), cons(c, TVALR(thr), CPOP(thr)));
      NEXT;
    INST(icar):
      if (NIL_P(TVALR(thr)))
	WB(&TVALR(thr), CNIL);
      else if (TYPE(TVALR(thr)) != T_CONS)
	arc_err_cstrfmt(c, "can't take car of value with type %s", TYPENAME(TYPE(TVALR(thr))));
      else
	WB(&TVALR(thr), car(TVALR(thr)));
      NEXT;
    INST(icdr):
      if (NIL_P(TVALR(thr)))
	WB(&TVALR(thr), CNIL);
      else if (TYPE(TVALR(thr)) != T_CONS)
	arc_err_cstrfmt(c, "can't take cdr of value with type %s", TYPENAME(TYPE(TVALR(thr))));
      else
	WB(&TVALR(thr), cdr(TVALR(thr)));
      NEXT;
    INST(iscar):
      scar(CPOP(thr), TVALR(thr));
      NEXT;
    INST(iscdr):
      scdr(CPOP(thr), TVALR(thr));
      NEXT;
    INST(ispl):
      {
	value list = TVALR(thr), nlist = CPOP(thr);
	/* Find the first cons in list whose cdr is not itself a cons.
	   Join the list from the stack to it. */
	if (list == CNIL) {
	  WB(&TVALR(thr), nlist);
	} else {
	  for (;;) {
	    if (!CONS_P(cdr(list))) {
	      if (cdr(list) == CNIL)
		scdr(list, nlist);
	      else
		arc_err_cstrfmt(c, "splicing improper list");
	      break;
	    }
	    list = cdr(list);
	  }
	}
      }
      NEXT;
    INST(iis):
      WB(&TVALR(thr), arc_is(c, TVALR(thr), CPOP(thr)));
      NEXT;
    INST(iiso):
      WB(&TVALR(thr), arc_iso(c, TVALR(thr), CPOP(thr)));
      NEXT;
    INST(igt):
      WB(&TVALR(thr), (FIX2INT(arc_cmp(c, TVALR(thr), CPOP(thr))) > 0) ?
	 CTRUE : CNIL);
      NEXT;
    INST(ilt):
      WB(&TVALR(thr), (FIX2INT(arc_cmp(c, TVALR(thr), CPOP(thr))) < 0) ?
	 CTRUE : CNIL);
      NEXT;
    INST(idup):
      WB(&TVALR(thr), *(TSP(thr)+1));
      NEXT;
    INST(icls):
      WB(&TVALR(thr), arc_mkclosure(c, TVALR(thr), TENVR(thr)));
      NEXT;
    INST(iconsr):
      WB(&TVALR(thr), cons(c, CPOP(thr), TVALR(thr)));
      NEXT;
#ifndef HAVE_THREADED_INTERPRETER
    default:
#else
    INST(invalid):
#endif
      arc_err_cstrfmt(c, "invalid opcode %02x", *TIP(thr));
#ifdef HAVE_THREADED_INTERPRETER
#else
    }
  }
#endif
 endquantum:
  return;
}

value arc_mkthread(arc *c, value funptr, int stksize, int ip)
{
  value thr;
  value code;

  thr = c->get_cell(c);
  BTYPE(thr) = T_THREAD;
  TSTACK(thr) = arc_mkvector(c, stksize);
  TSBASE(thr) = &VINDEX(TSTACK(thr), 0);
  TSP(thr) = TSTOP(thr) = &VINDEX(TSTACK(thr), stksize-1);
  TSTATE(thr) = Tready;
  TFUNR(thr) = funptr;
  code = VINDEX(TFUNR(thr), 0);
  TIP(thr) = &VINDEX(code, ip);
  TENVR(thr) = TVALR(thr) = CNIL;
  TTID(thr) = ++c->tid_nonce;
  TCONR(thr) = CNIL;
  TRVCH(thr) = arc_mkchan(c);
  TCM(thr) = arc_mkhash(c, 6);
  TEXH(thr) = CNIL;		/* no exception handlers */
  TEXC(thr) = CNIL;
  
  TACELL(thr) = 0;
  TCH(thr) = cons(c, INT2FIX(0xdead), CNIL);
  TBCH(thr) = TCH(thr);
  return(thr);
}

static void closapply(arc *c, value thr, value fun)
{
  value cl;

  cl = fun;
  WB(&TFUNR(thr), car(cl));
  WB(&TENVR(thr), cdr(cl));
  TIP(thr) = &VINDEX(VINDEX(TFUNR(thr), 0), 0);
}

static value c4apply(arc *c, value thr, value avec,
		     value c4rv, value c4ctx)
{
  value retval, cont, nargv, cfn;
  int i;

  cfn = TVALR(thr);
  WB(&TFUNR(thr), TVALR(thr));
  retval = REP(cfn)._cfunc.fnptr(c, avec, c4rv, c4ctx);
  if (TYPE(retval) == T_XCONT) {
    /* 1. Create a continuation object using the xcont as the offset */
    cont = arc_mkcont(c, retval, thr);
    /* 2. Put the continuation on the continuation register. */
    WB(&TCONR(thr), cons(c, cont, TCONR(thr)));
    TSP(thr) = TSTOP(thr);
    /* 3. Push the parameters for the new call on the stack in reverse
          order */
    nargv = VINDEX(retval, 3);
    for (i=0; i<VECLEN(nargv); i++) {
      CPUSH(thr, VINDEX(nargv, i));
    }
    /* 4. Restart, with the value register pointing to the callee,
       so that it gets "called" */
    WB(&TVALR(thr), VINDEX(retval, 2));
    TARGC(thr) = VECLEN(nargv);
    arc_apply(c, thr, VINDEX(retval, 2));
    /* when this returns, we go back to the virtual machine loop,
       resuming execution at the address of the called virtual machine
       function. */
  }
  return(retval);
}

/* Apply a function in a new thread created for this purpose.  Primarily
   intended for expanding macros.  This will run the thread until the
   thread reaches Trelease state, and no other threads can execute.
   Note that garbage collection cycles do not execute while this is done. */
value arc_macapply(arc *c, value func, value args)
{
  value thr, oldthr, retval, arg, nahd;
  int argc;

  c->in_compile = 1;
  oldthr = c->curthread;
  thr = arc_mkthread(c, func, c->stksize, 0);
  WB(&c->curthread, thr);
  /* push the args in reverse order */
  nahd = arc_list_reverse(c, args);
  argc = 0;
  for (arg = nahd; arg != CNIL; arg = cdr(arg)) {
    CPUSH(thr, car(arg));
    argc++;
  }
  TARGC(thr) = argc;
  /* run the new thread until we hit Trelease */
  if (TYPE(func) != T_CLOS && TYPE(func) != T_CODE) {
    /* handle C-defined functions -- CC4 functions still won't work though. */
    arc_apply(c, thr, func);
    retval = TVALR(thr);
    WB(&c->curthread, oldthr);
    return(retval);
  }

  if (TYPE(func) == T_CLOS) {
    WB(&TFUNR(thr), car(func));
    WB(&TENVR(thr), cdr(func));
  } else {
    WB(&TFUNR(thr), func);
  }
  TIP(thr) = &VINDEX(VINDEX(TFUNR(thr), 0), 0);
  MARKPROP(thr);
  while (TSTATE(thr) != Trelease) {
    /* XXX - this makes macros more special and restricted than they
       have to be.  Threading primitives cannot be used in macros because
       they do not run like ordinary processes! */
    if (TSTATE(thr) != Tready && TSTATE(thr) != Texiting) {
      arc_err_cstrfmt(c, "fatal: deadlock detected in macro execution");
      return(CNIL);
    }
    arc_vmengine(c, thr, c->quantum);
    /* XXX - Consider whether we should be able to execute the garbage
       collector thread after each quantum.  This will require careful
       work in ensuring that the local variables in the compiler
       cannot be garbage collected, and ensuring this is the case may
       be somewhat complicated. Garbage generated as a macro is
       executed in this way will eventually be collected of course,
       but only after the compiler finishes execution.  This may
       result in more memory being consumed if a function being
       compiled invokes many macros. */
  }
  retval = TVALR(thr);
  WB(&c->curthread, oldthr);
  c->in_compile = 0;
  return(retval);
}

/* This function will return a list of all closures that should be called
   after rerooting.  This is the core of the Hanson-Lamping algorithm
   for implementing dynamic-wind.

   Many thanks to Ross Angle (rocketnia) for helping me to understand
   the Hanson-Lamping algorithm! */
static value reroot(arc *c, value thr, value there)
{
  value before, after, calls;

  if (TCH(thr) == there)
    return(CNIL);
  calls = reroot(c, thr, cdr(there));
  before = car(car(there));
  after = cdr(car(there));
  scar(TCH(thr), cons(c, after, before));
  scdr(TCH(thr), there);
  scar(there, INT2FIX(0xdead));
  scdr(there, CNIL);
  WB(&TCH(thr), there);
  return(cons(c, before, calls));
}

/* The presence of protect (dynamic-wind) in Arc means that this is
   no longer a simple matter of restoring the continuation passed
   to it.  We have to call reroot to get a list of all functions
   that should be executed before the continuation itself
   is restored. */
value apply_continuation(arc *c, value argv, value rv, CC4CTX)
{
  CC4VDEFBEGIN;
  CC4VARDEF(cont);
  CC4VARDEF(conarg);
  CC4VARDEF(tcr);
  CC4VDEFEND;

  CC4BEGIN(c);
  CC4V(cont) = VINDEX(argv, 0);
  CC4V(conarg) = VINDEX(argv, 1);
  /* See if we have a saved TCH in the continuation.  If so,
     reroot to it and obtain all the befores/afters we need to
     execute.  Otherwise, just restore the continuation. */
  if (!NIL_P(CONT_TCH(CC4V(cont)))) {
    CC4V(tcr) = reroot(c, c->curthread, CONT_TCH(CC4V(cont)));
    CC4V(tcr) = arc_list_reverse(c, CC4V(tcr));
    while (CC4V(tcr) != CNIL) {
      value tcont = car(CC4V(tcr));

      if (!NIL_P(tcont))
	CC4CALL(c, argv, tcont, 0, CNIL);
      WB(&CC4V(tcr), cdr(CC4V(tcr)));
    }
  }
  CC4END;
  /* Now, we have to modify the continuation register so that when this
     function finally returns, it will restore the continuation we
     provided it. */
  WB(&TCONR(c->curthread), cons(c, CC4V(cont), CONT_CON(CC4V(cont))));
  return(CC4V(conarg));
}

void arc_apply(arc *c, value thr, value fun)
{
  value *argv, cfn, avec, retval;
  int argc, i;

  switch (TYPE(fun)) {
  case T_CLOS:
    /* XXX - Here is a really ugly kludge.  We reverse the arguments
       on the stack because the compiler expects arguments to appear
       in reverse order.  Pfft. */
    argc = TARGC(thr);
    argv = alloca(sizeof(value)*argc);
    for (i=0; i<argc; i++)
      argv[i] = CPOP(thr);
    for (i=0; i<argc; i++)
      CPUSH(thr, argv[i]);
    closapply(c, thr, fun);
    break;
  case T_CCODE:
    cfn = fun;
    argc = TARGC(thr);
    if (REP(cfn)._cfunc.argc >= 0 && REP(cfn)._cfunc.argc != argc) {
      /*      printf("\n");
	      printobj(c, fun);
	      printf("\nwrong number of arguments (%d for %d)\n", argc,
	      REP(cfn)._cfunc.argc); */
      arc_err_cstrfmt(c, "wrong number of arguments (%d for %d)\n", argc,
		      REP(cfn)._cfunc.argc);
      return;
    }
    argv = alloca(sizeof(value)*argc);
    /* reverse arguments again... */
    for (i=argc-1; i>=0; i--)
      argv[i] = CPOP(thr);
    switch (REP(cfn)._cfunc.argc) {
    case -3:
      /* Calling convention 4 */
      avec = arc_mkvector(c, argc);
      memcpy(&VINDEX(avec, 0), argv, sizeof(value)*argc);
      /* initial call of a CC4 function.  The context and return value
	 of the called function are initially nil. */
      retval = c4apply(c, thr, avec, CNIL, CNIL);
      if (TYPE(retval) == T_XCONT)
	return;			/* go back to the virtual machine loop */
      else
	WB(&TVALR(thr), retval); /* normal return */
      break;
    case -2:
      avec = arc_mkvector(c, argc);
      memcpy(&VINDEX(avec, 0), argv, sizeof(value)*argc);
      WB(&TVALR(thr), REP(cfn)._cfunc.fnptr(c, avec));
      break;
    case -1:
      WB(&TVALR(thr), REP(cfn)._cfunc.fnptr(c, argc, argv));
      break;
    case 0:
      WB(&TVALR(thr), REP(cfn)._cfunc.fnptr(c));
      break;
    case 1:
      WB(&TVALR(thr), REP(cfn)._cfunc.fnptr(c, argv[0]));
      break;
    case 2:
      WB(&TVALR(thr), REP(cfn)._cfunc.fnptr(c, argv[0], argv[1]));
      break;
    case 3:
      WB(&TVALR(thr), REP(cfn)._cfunc.fnptr(c, argv[0], argv[1], argv[2]));
      break;
    case 4:
      WB(&TVALR(thr), REP(cfn)._cfunc.fnptr(c, argv[0], argv[1], argv[2],
					    argv[3]));
      break;
    case 5:
      WB(&TVALR(thr), REP(cfn)._cfunc.fnptr(c, argv[0], argv[1], argv[2],
					    argv[3], argv[4]));
      break;
    case 6:
      WB(&TVALR(thr), REP(cfn)._cfunc.fnptr(c, argv[0], argv[1], argv[2],
					    argv[3], argv[4], argv[5]));
      break;
    case 7:
      WB(&TVALR(thr), REP(cfn)._cfunc.fnptr(c, argv[0], argv[1], argv[2],
					    argv[3], argv[4], argv[5],
					    argv[6]));
      break;
    case 8:
      WB(&TVALR(thr), REP(cfn)._cfunc.fnptr(c, argv[0], argv[1], argv[2],
					    argv[3], argv[4], argv[5],
					    argv[6], argv[7]));
      break;
    default:
      arc_err_cstrfmt(c, "too many arguments");
      return;
    }
    arc_return(c, thr);
    break;
  case T_CONS:
    if (TARGC(thr) != 1) {
      arc_err_cstrfmt(c, "list application expects 1 argument, given %d",
		      INT2FIX(TARGC(thr)));
      WB(&TVALR(thr), CNIL);
    } else {
      value count = CPOP(thr), ocount, cval;

      ocount = count;
      if ((FIXNUM_P(count) || TYPE(count) == T_BIGNUM) &&
	  (cval = arc_cmp(c, count, INT2FIX(0))) != INT2FIX(-1)) {
	value list=fun, res;
	/* We now have a non-negative exact integer for the count */
	do {
	  if (!CONS_P(list)) {
	    arc_err_cstrfmt(c, "index %d too large for list", ocount);
	    res = CNIL;
	    break;
	  }
	  res = car(list);
	  list = cdr(list);
	  count = __arc_sub2(c, count, INT2FIX(1));
	} while ((cval = arc_cmp(c, count, INT2FIX(0))) != INT2FIX(-1));
	WB(&TVALR(thr), res);
      } else {
	arc_err_cstrfmt(c, "list application expects non-negative exact integer argument, given object of type %d", INT2FIX(TYPE(count)));
	WB(&TVALR(thr), CNIL);
      }
    }
    arc_return(c, thr);
    break;
  case T_VECTOR:
    if (TARGC(thr) != 1) {
      arc_err_cstrfmt(c, "vector application expects 1 argument, given %d",
		      INT2FIX(TARGC(thr)));
      WB(&TVALR(thr), CNIL);
    } else {
      value count = CPOP(thr), ocount, cval;

      ocount = count;
      if (FIXNUM_P(count) &&
	  (cval = arc_cmp(c, count, INT2FIX(0))) != INT2FIX(-1)) {
	value vec=fun, res;
	/* We now have a non-negative exact integer for the count/index */
	if (FIX2INT(count) >= VECLEN(vec)) {
	  arc_err_cstrfmt(c, "index %d too large for vector", ocount);
	  res = CNIL;
	} else {
	  res = VINDEX(vec, FIX2INT(count));
	}
	WB(&TVALR(thr), res);
      } else {
	/* XXX - Permit negative indices?  Could be useful. */
	arc_err_cstrfmt(c, "vector application expects non-negative fixnum argument, given object of type %d", INT2FIX(TYPE(count)));
	WB(&TVALR(thr), CNIL);
      }
    }
    arc_return(c, thr);
    break;
  case T_TABLE:
    {
      value tbl, key, dflt, bind;

      tbl = fun;
      if (TARGC(thr) == 1) {
	key = CPOP(thr);
	dflt = CNIL;
      } else if (TARGC(thr) == 2) {
	dflt = CPOP(thr);
	key = CPOP(thr);
      } else {
	arc_err_cstrfmt(c, "table application expects 1 or 2 arguments, given %d",
			INT2FIX(TARGC(thr)));
	WB(&TVALR(thr), CNIL);
      }
      bind = arc_hash_lookup(c, tbl, key);
      WB(&TVALR(thr), (bind == CUNBOUND) ? dflt : bind);
    }
    arc_return(c, thr);
    break;
  case T_STRING:
    if (TARGC(thr) != 1) {
      arc_err_cstrfmt(c, "string application expects 1 argument, given %d",
		      INT2FIX(TARGC(thr)));
      WB(&TVALR(thr), CNIL);
    } else {
      value count = CPOP(thr), ocount, cval;

      ocount = count;
      if (FIXNUM_P(count) &&
	  (cval = arc_cmp(c, count, INT2FIX(0))) != INT2FIX(-1)) {
	value str=fun, res;
	/* We now have a non-negative exact integer for the count/index.
	   XXX - string length is always a fixnum? */
	if (FIX2INT(count) >= arc_strlen(c, str)) {
	  arc_err_cstrfmt(c, "index %d too large for string", ocount);
	  res = CNIL;
	} else {
	  res = arc_mkchar(c, arc_strindex(c, str, FIX2INT(count)));
	}
	WB(&TVALR(thr), res);
      } else {
	/* XXX - permit negative indices? Not allowed by reference Arc. */
	arc_err_cstrfmt(c, "string application expects non-negative fixnum argument, given object of type %d", INT2FIX(TYPE(count)));
	WB(&TVALR(thr), CNIL);
      }
    }
    arc_return(c, thr);
    break;
  case T_CONT:
    if (TARGC(thr) != 1) {
      arc_err_cstrfmt(c, "wrong number of arguments for continuation (%d for 1)", TARGC(thr));
      return;
    }
#if 0
    /* simple version that does not deal with protect functions */
    retval = CPOP(thr);
    arc_restorecont(c, thr, fun);
    WB(&TVALR(thr), retval);
#else
    /* the continuation is in the value register when we get here */
    retval = CPOP(thr);
    CPUSH(thr, TVALR(thr));
    CPUSH(thr, retval);
    WB(&TVALR(thr), arc_mkccode(c, -3, apply_continuation, CNIL));
    TARGC(thr) = 2;
    arc_apply(c, thr, TVALR(thr));
#endif
    break;
  default:
    arc_err_cstrfmt(c, "invalid function application, applying object of type %s", __arc_typenames[TYPE(fun)]);
    arc_return(c, thr);
  }
}

/* arc_apply intended to be called from functions */
value arc_apply2(arc *c, value argv, value rv, CC4CTX)
{
  CC4VDEFBEGIN;
  CC4VDEFEND;
  /* we don't care what happens to these variables after */
  value func, fargv, coerceargv;
  int i, argc;

  argc = VECLEN(argv);
  if (argc < 1) {
    arc_err_cstrfmt(c, "apply expects at least 1 argument");
    return(CNIL);
  }
  func = VINDEX(argv, 0);
  /* Copy the args for the call */
  fargv = (argv == 1) ? CNIL : VINDEX(argv, 1);
  if (argc > 2) {
    for (i=2; i<argc; i++)
      fargv = cons(c, fargv, VINDEX(argv, i));
  }
  coerceargv = arc_mkvector(c, 2);
  VINDEX(coerceargv, 0) = fargv;
  VINDEX(coerceargv, 1) = ARC_BUILTIN(c, S_VECTOR);
  CC4BEGIN(c);
  CC4CALLV(c, argv, func, arc_coerce(c, coerceargv));
  CC4END;
  return(rv);
}

/* Restore a continuation.  This can only restore a normal continuation. */
void arc_restorecont(arc *c, value thr, value cont)
{
  int stklen, offset, i;
  value savedstk;

  WB(&TFUNR(thr), VINDEX(cont, 1));
  WB(&TENVR(thr), VINDEX(cont, 2));
  savedstk = VINDEX(cont, 3);
  stklen = (savedstk == CNIL) ? 0 : VECLEN(savedstk);
  TSP(thr) = TSTOP(thr) - stklen;
  for (i=0; i<stklen; i++) {
    *(TSP(thr) + i + 1) = VINDEX(savedstk, i);
  }
  offset = FIX2INT(VINDEX(cont, 0));
  TIP(thr) = &VINDEX(VINDEX(TFUNR(thr), 0), offset);
  WB(&TCONR(thr), CONT_CON(cont));
  /* Restore the exception handlers as they existed when the
     continuation was created */
  WB(&TEXH(thr), CONT_EXH(cont));
}

/* Restore an extended continuation inside a continuation */
int arc_restorexcont(arc *c, value thr, value cont)
{
  value c4ctx, c4rv, oargv, xcont, retval;
  int stklen, i;

  WB(&TFUNR(thr), VINDEX(cont, 1));
  WB(&TENVR(thr), VINDEX(cont, 2));
  stklen = VECLEN(VINDEX(cont, 3));
  TSP(thr) = TSTOP(thr) - stklen;
  memcpy(TSP(thr), &VINDEX(VINDEX(cont, 3), 0), stklen*sizeof(value));
  xcont = VINDEX(cont, 0);
  /* An xcont offset is created by a CC4 "return".  If we see one,
     we should "apply" the function again with the updated context and
     return value in the value register with the same parameters.
     The function and environment have been restored as part of the
     normal progress of the continuation.  */
  c4ctx = VINDEX(xcont, 0);	/* saved context */
  c4rv = TVALR(thr);		/* return value of callee */
  WB(&TVALR(thr), TFUNR(thr));	/* set value reg to func reg in prep for
				   call to c4apply */
  /* restore original parameters */
  oargv = VINDEX(xcont, 1);
  for (i=VECLEN(oargv)-1; i>=0; i--)
    CPUSH(thr, VINDEX(oargv, i));
  /* apply the original function, causing it to return to the place
     specified by the context ("returning" as it were from the call),
     with the return value of the callee as one of its parameters. */
  retval = c4apply(c, thr, oargv, c4rv, c4ctx);
  if (TYPE(retval) == T_XCONT) {
    /* In this case, we do a simple return.  Everything has been set up
       to "call" (by returning!) to the function which the caller is
       trying to call. */
    return(1);
  }
  /* Otherwise, the CC4 function has at last returned normally, so
     now we can put its return value in the value register.  The
     continuation register now contains a continuation which should be
     a standard continuation that we can restore. */
  WB(&TVALR(thr), retval);
  return(0);
}

/* restore the continuation at the head of the continuation register */
int arc_return(arc *c, value thr)
{
  value cont;

  for (;;) {
    if (!CONS_P(TCONR(thr)))
      return(1);
    cont = car(TCONR(thr));
    if (cont == CNIL)
      return(1);
    WB(&TCONR(thr), cdr(TCONR(thr)));
    if (TYPE(VINDEX(cont, 0)) == T_XCONT) {
      if (arc_restorexcont(c, thr, cont)) {
	return(0);
      }
      continue;
    }
    arc_restorecont(c, thr, cont);
    return(0);
  }
}

value arc_mkcont(arc *c, value offset, value thr)
{
  value cont = arc_mkvector(c, 7);
  value savedstk;
  int stklen, i;
  value *base = &VINDEX(VINDEX(TFUNR(thr), 0), 0);

  BTYPE(cont) = T_CONT;
  if (FIXNUM_P(offset)) {
    /* compute the absolute address of the continuation if it is a
       fixnum (the usual case) */
    WB(&CONT_OFS(cont), INT2FIX((TIP(thr) + FIX2INT(offset) - 2) - base));
  } else {
    /* offset is probably an XCONT, just store it. */
    WB(&CONT_OFS(cont), offset);
  }
  WB(&CONT_FUN(cont), TFUNR(thr));
  WB(&CONT_ENV(cont), TENVR(thr));
  /* Save the used portion of the stack */
  stklen = TSTOP(thr) - (TSP(thr));
  savedstk = arc_mkvector(c, stklen);
  for (i=0; i<stklen; i++) {
    VINDEX(savedstk, i) = *(TSP(thr) + i + 1);
  }
  WB(&CONT_STK(cont), savedstk);
  WB(&CONT_CON(cont), TCONR(thr));
  WB(&CONT_TCH(cont), CNIL);
  WB(&CONT_EXH(cont), TEXH(thr));
  return(cont);
}

value arc_mkxcontv(arc *c, value cc4ctx, value argv, value func, value fargv)
{
  value xcont = arc_mkvector(c, 4);
  BTYPE(xcont) = T_XCONT;
  WB(&VINDEX(xcont, 0), cc4ctx); /* context */
  WB(&VINDEX(xcont, 1), argv);	 /* original params */
  WB(&VINDEX(xcont, 2), func);	 /* callee function */
  WB(&VINDEX(xcont, 3), fargv);	 /* callee arguments */
  return(xcont);
}

/* This creates a T_XCONT object, which is used to support calling
   convention 4. */
value arc_mkxcont(arc *c, value cc4ctx, value argv, value func, int fargc, ...)
{
  value fargv = arc_mkvector(c, fargc);
  va_list ap;
  int i;

  va_start(ap, fargc);
  for (i=0; i<fargc; i++)
    VINDEX(fargv, i) = va_arg(ap, value);

  va_end(ap);
  return(arc_mkxcontv(c, cc4ctx, argv, func, fargv));
}

value arc_mkenv(arc *c, value parent, int size)
{
  value env;

  env = arc_mkvector(c, size+1);
  BTYPE(env) = T_ENV;
  return(cons(c, env, parent));
}

value arc_mkclosure(arc *c, value code, value env)
{
  value clos;

  clos = cons(c, code, env);
  BTYPE(clos) = T_CLOS;
  return(clos);
}

/* Exception handlers are continuations consed on top of the TEXH
   register.  It is not possible at the moment to make an error handling
   function in C.  I have to think about how to do that. */
value arc_on_err(arc *c, value argv, value rv, CC4CTX)
{
  CC4VDEFBEGIN;
  CC4VDEFEND;
  value exh;
  value errfn, fn;

  CC4BEGIN(c);
  if (VECLEN(argv) != 2) {
    arc_err_cstrfmt(c, "procedure on-err: expects 2 arguments, given %d",
		    VECLEN(argv));
    return(CNIL);
  }
  errfn = VINDEX(argv, 0);
  fn = VINDEX(argv, 1);
  if (TYPE(errfn) != T_CLOS) {
    arc_err_cstrfmt(c, "procedure on-err: closure required for first arg");
    return(CNIL);
  }

  if (TYPE(fn) != T_CLOS) {
    arc_err_cstrfmt(c, "procedure on-err: closure required for second arg");
    return(CNIL);
  }

  /* create an exception handler cons and register it in TEXH. */
  WB(&CONT_TCH(car(TCONR(c->curthread))), TCH(c->curthread));
  exh = cons(c, errfn, car(TCONR(c->curthread)));
  WB(&TEXH(c->curthread), cons(c, exh, TEXH(c->curthread)));

  /* call the thunk passed to us */
  CC4CALL(c, argv, fn, 0, CNIL);
  CC4END;
  return(rv);
}

value arc_mkexception(arc *c, value details, value lastcall, value contchain)
{
  value exception = arc_mkvector(c, 3);

  BTYPE(exception) = T_EXCEPTION;
  VINDEX(exception, 0) = details;
  VINDEX(exception, 1) = lastcall;
  VINDEX(exception, 2) = contchain;
  return(exception);
}

#define ESTRMAX 1024

value arc_err(arc *c, value argv, value rv, CC4CTX)
{
  CC4VDEFBEGIN;
  CC4VARDEF(tcr);
  CC4VARDEF(exh);
  CC4VARDEF(there);
  CC4VARDEF(exc);
  CC4VDEFEND;

  CC4BEGIN(c);
  if (VECLEN(argv) != 1) {
    arc_err_cstrfmt(c, "err expects only one argument %d passed", VECLEN(argv));
    return(CNIL);
  }
  CC4V(exc) = VINDEX(argv, 0);
  if (TYPE(CC4V(exc)) == T_STRING) {
    CC4V(exc) = arc_mkexception(c, CC4V(exc), CODE_NAME(TFUNR(c->curthread)),
				TCONR(c->curthread));
  }
  TYPECHECK(CC4V(exc), T_EXCEPTION, 1);

  CC4V(there) = (NIL_P(TEXH(c->curthread))) ? TBCH(c->curthread)
    : CONT_TCH(cdr(car(TEXH(c->curthread))));

  /* Reroot and execute any protect handlers up to the point of our
     saved continuation's TCH. */
  CC4V(tcr) = reroot(c, c->curthread, CC4V(there));
  CC4V(tcr) = arc_list_reverse(c, CC4V(tcr));
  while (CC4V(tcr) != CNIL) {
    value tcont = car(CC4V(tcr));

    if (!NIL_P(tcont))
      CC4CALL(c, argv, tcont, 0, CNIL);
    WB(&CC4V(tcr), cdr(CC4V(tcr)));
  }

  if (NIL_P(TEXH(c->curthread))) {
    /* if no exception handlers have been registered, the thread
       should be terminated.  The default error handler is called
       when this happens.  If the default error handler returns,
       the thread just dies. */
    c->in_compile = 0;
    WB(&TCONR(c->curthread), CNIL);
    TSTATE(c->curthread) = Tbroken;
    c->signal_error(c, arc_exc_details(c, CC4V(exc)));
    longjmp(TVJMP(c->curthread), 1);
  }

  /* If we have an exception handler, the exception handler vector
     from the stack of exception handlers, and call it with the
     exception passed to us. */
  CC4V(exh) = car(TEXH(c->curthread));
  WB(&TEXH(c->curthread), cdr(TEXH(c->curthread)));
  CC4CALL(c, argv, car(CC4V(exh)), 1, CC4V(exc));
  /* Now, we restore the continuation */
  WB(&TCONR(c->curthread), cons(c, cdr(CC4V(exh)), CONT_CON(cdr(CC4V(exh)))));
  CC4END;
  /* When the saved continuation is restored, the value returned by the
     exception handler is returned to the caller of on-err that created
     the exception. */
  return(rv);
}

value arc_errexc(arc *c, value ex)
{
  /* This is one way to "call" a CC4 function, but it can only be
     used for tail calls. */
  CPUSH(c->curthread, ex);
  WB(&TVALR(c->curthread), arc_mkccode(c, -3, arc_err, CNIL));
  TARGC(c->curthread) = 1;
  longjmp(TVJMP(c->curthread), 2);
  return(CNIL);
}

value arc_err_cstrfmt2(arc *c, const char *lastcall, const char *fmt, ...)
{
  va_list ap;
  char text[ESTRMAX];
  value errtext, ex, conr;

  va_start(ap, fmt);
  /* XXX - make another formatting function, vsnprintf won't do for
     some of the other stuff we want to be able to do. */
  vsnprintf(text, ESTRMAX, fmt, ap);
  va_end(ap);
  errtext = arc_mkstringc(c, text);

  conr = (c->curthread == CNIL) ? CNIL : TCONR(c->curthread);
  ex = arc_mkexception(c, errtext, arc_mkstringc(c, lastcall), conr);
  if (c->curthread == CNIL) {
    c->in_compile = 0;
    c->signal_error(c, VINDEX(ex, 0));
    return(CNIL);
  }
  return(arc_errexc(c, ex));
}

value arc_exc_details(arc *c, value exc)
{
  TYPECHECK(exc, T_EXCEPTION, 1);
  return(VINDEX(exc, 0));
}

value arc_callcc(arc *c, value argv, value rv, CC4CTX)
{
  CC4VDEFBEGIN;
  CC4VARDEF(func);
  CC4VARDEF(cont);
  CC4VDEFEND;
  CC4BEGIN(c);

  if (VECLEN(argv) != 1) {
    arc_err_cstrfmt(c, "procedure ccc: expects 1 argument, given %d",
		    VECLEN(argv));
    return(CNIL);
  }
  CC4V(func) = VINDEX(argv, 0);
  TYPECHECK(CC4V(func), T_CLOS, 1);
  CC4V(cont) = car(TCONR(c->curthread));
  /* We need to save the current TCH in the reified continuation so
     that if it is called, the before/after handlers saved by
     protect/dynamic-wind can be executed.  This is not needed when
     a dynamic-wind call returns normally: the after handlers are
     executed by the dynamic-wind function itself when the function
     returns. */
  WB(&CONT_TCH(CC4V(cont)), TCH(c->curthread));
  CC4CALL(c, argv, CC4V(func), 1, CC4V(cont));
  CC4END;
  return(rv);
}

value arc_dynamic_wind(arc *c, value argv, value rv, CC4CTX)
{
  CC4VDEFBEGIN;
  CC4VARDEF(here);
  CC4VARDEF(tcr);
  CC4VARDEF(before);
  CC4VARDEF(during);
  CC4VARDEF(after);
  CC4VARDEF(ret);
  CC4VDEFEND;
  CC4BEGIN(c);
  if (VECLEN(argv) != 3) {
    arc_err_cstrfmt(c, "procedure dynamic-wind: expects 3 arguments, given %d",
		    VECLEN(argv));
    return(CNIL);
  }
  CC4V(before) = VINDEX(argv, 0);
  CC4V(during) = VINDEX(argv, 1);
  CC4V(after) = VINDEX(argv, 2);
  TYPECHECK(CC4V(before), T_CLOS, 1);
  TYPECHECK(CC4V(during), T_CLOS, 2);
  TYPECHECK(CC4V(after), T_CLOS, 2);
  CC4V(here) = TCH(c->curthread);
  CC4V(tcr) = reroot(c, c->curthread,
		     cons(c, cons(c, CC4V(before), CC4V(after)),
			  CC4V(here)));
  CC4V(tcr) = arc_list_reverse(c, CC4V(tcr));
  while (CC4V(tcr) != CNIL) {
    value tcont = car(CC4V(tcr));

    if (!NIL_P(tcont))
      CC4CALL(c, argv, tcont, 0, CNIL);
    WB(&CC4V(tcr), cdr(CC4V(tcr)));
  }
  CC4CALL(c, argv, CC4V(during), 0, CNIL);
  CC4V(ret) = rv;
  /* execute the after clauses if the during thunk returns normally */
  CC4V(tcr) = reroot(c, c->curthread, CC4V(here));
  CC4V(tcr) = arc_list_reverse(c, CC4V(tcr));
  while (CC4V(tcr) != CNIL) {
    value tcont = car(CC4V(tcr));

    if (!NIL_P(tcont))
      CC4CALL(c, argv, tcont, 0, CNIL);
    WB(&CC4V(tcr), cdr(CC4V(tcr)));
  }
  return(CC4V(ret));
  CC4END;
  return(CC4V(ret));
}

value arc_cmark(arc *c, value key)
{
  value cm, val;

  cm = TCM(c->curthread);
  val = arc_hash_lookup(c, cm, key);
  if (val == CUNBOUND)
    return(CNIL);
  return(car(val));
}

/* Never use these functions directly.  They should only be used within
   the call-w/cmark macro. */
value arc_scmark(arc *c, value key, value val)
{
  value cm, bind;

  cm = TCM(c->curthread);
  bind = arc_hash_lookup(c, cm, key);
  if (bind == CUNBOUND)
    bind = CNIL;
  bind = cons(c, val, bind);
  arc_hash_insert(c, cm, key, bind);
  return(val);
}

value arc_ccmark(arc *c, value key)
{
  value cm, bind, val;

  cm = TCM(c->curthread);
  bind = arc_hash_lookup(c, cm, key);
  if (bind == CUNBOUND)
    return(CNIL);
  val = car(bind);
  bind = cdr(bind);
  if (NIL_P(bind)) {
    arc_hash_delete(c, cm, key);
  } else {
    arc_hash_insert(c, cm, key, bind);
  }
  return(val);
}
