/* 
  Copyright (C) 2010 Rafael R. Sevilla

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
#include "../config.h"
#include "arcueid.h"
#include "vmengine.h"
#include "alloc.h"
#include "arith.h"
#include "symbols.h"

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
  char str[256];
  static value *bpofs = NULL;		/* breakpoint offset */
  static value bp = CNIL;	/* breakpoint function */

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
  if (--TQUANTA(thr) <= 0 || !(TSTATE(thr) == Tready		\
			       || TSTATE(thr)) == Texiting)	\
      goto endquantum;						\
    if (vmtrace)						\
      trace(c, thr);						\
    goto *(JTBASE + jumptbl[*TIP(thr)++]); }
#else
#define NEXT {							\
    if (--TQUANTA(thr) <= 0 || !(TSTATE(thr) == Tready		\
				 || TSTATE(thr)) == Texiting)	\
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

  setjmp(TVJMP(thr));
  if (TSTATE(thr) != Tready && TSTATE(thr) != Texiting)
    return;
  c->curthread = thr;
  TQUANTA(thr) = quanta;

#ifdef HAVE_THREADED_INTERPRETER
#ifdef HAVE_TRACING
  if (vmtrace)
    trace(c, thr);
#endif
  goto *(void *)(JTBASE + jumptbl[*TIP(thr)++]);
#else
  for (;;) {
    if (--TQUANTA(thr) <= 0 || (TSTATE(thr) != Tready
				&& TSTATE(thr) != Texiting))
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
      TVALR(thr) = CPOP(thr);
      NEXT;
    INST(ildi):
      TVALR(thr) = *TIP(thr)++;
      NEXT;
    INST(ildl):
      TVALR(thr) = CODE_LITERAL(TFUNR(thr), *TIP(thr)++);
      NEXT;
    INST(ildg):
      {
	value tmp, tmpstr;
	char *cstr;

	tmp = CODE_LITERAL(TFUNR(thr), *TIP(thr)++);
	if ((TVALR(thr) = arc_hash_lookup(c, c->genv, tmp)) == CUNBOUND) {
	  tmpstr = arc_sym2name(c, tmp);
	  cstr = alloca(sizeof(char)*(FIX2INT(arc_strutflen(c, tmpstr)) + 1));
	  arc_str2cstr(c, tmpstr, cstr);
	  /* arc_print_string(c, arc_prettyprint(c, tmp)); printf("\n"); */
	  arc_err_cstrfmt(c, "Unbound symbol: _%s", cstr);
	  TVALR(thr) = CNIL;
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
	TVALR(thr) = ENV_VALUE(car(tmp), iindx);
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
	ENV_NAMES(car(TENVR(thr))) = (namesidx < 0) ? CNIL
	  : CODE_LITERAL(TFUNR(thr), namesidx);
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
      TVALR(thr) = CTRUE;
      NEXT;
    INST(inil):
      TVALR(thr) = CNIL;
      NEXT;
    INST(ihlt):
      TSTATE(thr) = Trelease;
      goto endquantum;
      NEXT;
    INST(iadd):
      TVALR(thr) = __arc_add2(c, TVALR(thr), CPOP(thr));
      NEXT;
    INST(isub):
      TVALR(thr) = __arc_sub2(c, CPOP(thr), TVALR(thr));
      NEXT;
    INST(imul):
      TVALR(thr) = __arc_mul2(c, TVALR(thr), CPOP(thr));
      NEXT;
    INST(idiv):
      TVALR(thr) = __arc_div2(c, CPOP(thr), TVALR(thr));
      NEXT;
    INST(icons):
      TVALR(thr) = cons(c, TVALR(thr), CPOP(thr));
      NEXT;
    INST(icar):
      TVALR(thr) = (TVALR(thr) == CNIL) ? CNIL : car(TVALR(thr));
      NEXT;
    INST(icdr):
      TVALR(thr) = (TVALR(thr) == CNIL) ? CNIL : cdr(TVALR(thr));
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
	  TVALR(thr) = nlist;
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
      TVALR(thr) = arc_is(c, TVALR(thr), CPOP(thr));
      NEXT;
    INST(iiso):
      TVALR(thr) = arc_iso(c, TVALR(thr), CPOP(thr));
      NEXT;
    INST(igt):
      TVALR(thr) = (FIX2INT(arc_cmp(c, TVALR(thr), CPOP(thr))) > 0) ?
	CTRUE : CNIL;
      NEXT;
    INST(ilt):
      TVALR(thr) = (FIX2INT(arc_cmp(c, TVALR(thr), CPOP(thr))) < 0) ?
	CTRUE : CNIL;
      NEXT;
    INST(idup):
      TVALR(thr) = *(TSP(thr)+1);
      NEXT;
    INST(icls):
      TVALR(thr) = arc_mkclosure(c, TVALR(thr), TENVR(thr));
      NEXT;
    INST(iconsr):
      TVALR(thr) = cons(c, CPOP(thr), TVALR(thr));
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
  TECONT(thr) = CNIL;
  TEXC(thr) = CNIL;
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
    TVALR(thr) = VINDEX(retval, 2);
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

  oldthr = c->curthread;
  thr = arc_mkthread(c, func, c->stksize, 0);
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
    c->curthread = oldthr;
    return(retval);
  }

  if (TYPE(func) == T_CLOS) {
    WB(&TFUNR(thr), car(func));
    WB(&TENVR(thr), cdr(func));
  } else {
    WB(&TFUNR(thr), func);
  }
  TIP(thr) = &VINDEX(VINDEX(TFUNR(thr), 0), 0);
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
  c->curthread = oldthr;
  return(retval);
}

/* The presence of protect (dynamic-wind) in Arc means that this is
   no longer a simple matter of restoring the continuation passed
   to it.  We now need to go down the continuation chain to look for
   protect functions, executing them until we get to the continuation
   that we are trying to apply, and only then can we restore the applied
   continuation. */
value apply_continuation(arc *c, value argv, value rv, CC4CTX)
{
  CC4VDEFBEGIN;
  CC4VARDEF(cont);
  CC4VARDEF(conarg);
  CC4VARDEF(tcr);
  CC4VDEFEND;
  value thr;

  CC4BEGIN(c);
  CC4V(cont) = VINDEX(argv, 0);
  CC4V(conarg) = VINDEX(argv, 1);
  CC4V(tcr) = TCONR(c->curthread);
  while (CC4V(tcr) != CNIL && car(CC4V(tcr)) != CC4V(cont)) {
    /* XXX - execute the protect sections of all the continuations above
       the chain from the continuation which we are about to restore. */
    value tcont = VINDEX(car(CC4V(tcr)), 6), protclos;

    if (NIL_P(tcont)) {
      /* no protect section, move on */
      CC4V(tcr) = cdr(CC4V(tcr));
      continue;
    }

    /* We have a continuation with a protect section.  We should now
       execute the closure embedded inside it. */
    protclos = arc_mkclosure(c, VINDEX(tcont, 1), VINDEX(tcont, 2));
    CC4CALL(c, argv, protclos, 0, CNIL);
    CC4V(tcr) = cdr(CC4V(tcr));
  }
  CC4END;
  /* Now, we have to modify the continuation register so that when this
     function finally returns, it will restore the continuation we
     provided it. */
  thr = c->curthread;
  TCONR(thr) = cons(c, CC4V(cont), VINDEX(CC4V(cont), 4));
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
	 of called function are initially nil. */
      retval = c4apply(c, thr, avec, CNIL, CNIL);
      if (TYPE(retval) == T_XCONT)
	return;			/* go back to the virtual machine loop */
      else
	TVALR(thr) = retval;	/* normal return */
      break;
    case -2:
      avec = arc_mkvector(c, argc);
      memcpy(&VINDEX(avec, 0), argv, sizeof(value)*argc);
      TVALR(thr) = REP(cfn)._cfunc.fnptr(c, avec);
      break;
    case -1:
      TVALR(thr) = REP(cfn)._cfunc.fnptr(c, argc, argv);
      break;
    case 0:
      TVALR(thr) = REP(cfn)._cfunc.fnptr(c);
      break;
    case 1:
      TVALR(thr) = REP(cfn)._cfunc.fnptr(c, argv[0]);
      break;
    case 2:
      TVALR(thr) = REP(cfn)._cfunc.fnptr(c, argv[0], argv[1]);
      break;
    case 3:
      TVALR(thr) = REP(cfn)._cfunc.fnptr(c, argv[0], argv[1], argv[2]);
      break;
    case 4:
      TVALR(thr) = REP(cfn)._cfunc.fnptr(c, argv[0], argv[1], argv[2],
					 argv[3]);
      break;
    case 5:
      TVALR(thr) = REP(cfn)._cfunc.fnptr(c, argv[0], argv[1], argv[2],
					 argv[3], argv[4]);
      break;
    case 6:
      TVALR(thr) = REP(cfn)._cfunc.fnptr(c, argv[0], argv[1], argv[2],
					 argv[3], argv[4], argv[5]);
      break;
    case 7:
      TVALR(thr) = REP(cfn)._cfunc.fnptr(c, argv[0], argv[1], argv[2],
					 argv[3], argv[4], argv[5],
					 argv[6]);
      break;
    case 8:
      TVALR(thr) = REP(cfn)._cfunc.fnptr(c, argv[0], argv[1], argv[2],
					 argv[3], argv[4], argv[5],
					 argv[6], argv[7]);
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
      TVALR(thr) = CNIL;
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
	TVALR(thr) = res;
      } else {
	arc_err_cstrfmt(c, "list application expects non-negative exact integer argument, given object of type %d", INT2FIX(TYPE(count)));
	TVALR(thr) = CNIL;
      }
    }
    arc_return(c, thr);
    break;
  case T_VECTOR:
    if (TARGC(thr) != 1) {
      arc_err_cstrfmt(c, "vector application expects 1 argument, given %d",
		      INT2FIX(TARGC(thr)));
      TVALR(thr) = CNIL;
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
	TVALR(thr) = res;
      } else {
	/* XXX - Permit negative indices?  Could be useful. */
	arc_err_cstrfmt(c, "vector application expects non-negative fixnum argument, given object of type %d", INT2FIX(TYPE(count)));
	TVALR(thr) = CNIL;
      }
    }
    arc_return(c, thr);
    break;
  case T_TABLE:
    if (TARGC(thr) != 1 && TARGC(thr) != 2) {
      arc_err_cstrfmt(c, "table application expects 1 or 2 arguments, given %d",
		      INT2FIX(TARGC(thr)));
      TVALR(thr) = CNIL;
    } else {
      value tbl, key, dflt, bind;

      tbl = fun;
      key = CPOP(thr);
      dflt = (TARGC(thr) == 1) ? CNIL : CPOP(thr);
      bind = arc_hash_lookup(c, tbl, key);
      TVALR(thr) = (bind == CUNBOUND) ? dflt : bind;
    }
    arc_return(c, thr);
    break;
  case T_STRING:
    if (TARGC(thr) != 1) {
      arc_err_cstrfmt(c, "string application expects 1 argument, given %d",
		      INT2FIX(TARGC(thr)));
      TVALR(thr) = CNIL;
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
	TVALR(thr) = res;
      } else {
	/* XXX - permit negative indices? Not allowed by reference Arc. */
	arc_err_cstrfmt(c, "string application expects non-negative fixnum argument, given object of type %d", INT2FIX(TYPE(count)));
	TVALR(thr) = CNIL;
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
    TVALR(thr) = retval;
#else
    /* the continuation is in the value register when we get here */
    retval = CPOP(thr);
    CPUSH(thr, TVALR(thr));
    CPUSH(thr, retval);
    TVALR(thr) = arc_mkccode(c, -3, apply_continuation, CNIL);
    TARGC(thr) = 2;
    arc_apply(c, thr, TVALR(thr));
#endif
    break;
  default:
    arc_err_cstrfmt(c, "invalid function application");
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
  TCONR(thr) = VINDEX(cont, 4);
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
  value cont, tmp;

  for (;;) {
    if (!CONS_P(TCONR(thr))) {
      if (TSTATE(thr) == Texiting)
	c->signal_error(c, VINDEX(TEXC(thr), 0));
      return(1);
    }
    cont = car(TCONR(thr));
    if (cont == CNIL) {
      if (TSTATE(thr) == Texiting)
	c->signal_error(c, VINDEX(TEXC(thr), 0));
      return(1);
    }
    /* see if the continuation has a cleanup continuation created
       by protect.  If so, restore it first and remove it from the
       continuation at the top, so that when the cleanup continuation
       returns, it will see this same continuation again. */
    if (CONT_PRT(cont) != CNIL) {
      if (CONT_PRT(cont) == CTRUE) {
	/* restore the contents of the value register before
	   proceeding. */
	WB(&CONT_PRT(cont), CNIL);
	WB(&TVALR(thr), CONT_PRV(cont));
	WB(&CONT_PRV(cont), CNIL);
	WB(&TCONR(thr), cdr(TCONR(thr)));
      } else {
	/* save the contents of the value register */
	WB(&CONT_PRV(cont), TVALR(thr));
	tmp = CONT_PRT(cont);
	WB(&CONT_PRT(cont), CTRUE);
	cont = tmp;
      }
    } else {
      WB(&TCONR(thr), cdr(TCONR(thr)));
    }
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

value arc_mkcontfull(arc *c, value offset, value funr, value envr,
		     value savedstk, value conr, value econt)
{
  value cont = arc_mkvector(c, 8);

  CONT_OFS(cont) = offset;
  CONT_FUN(cont) = funr;
  CONT_ENV(cont) = envr;
  CONT_STK(cont) = savedstk;
  CONT_CON(cont) = conr;
  CONT_ECR(cont) = econt;
  CONT_PRT(cont) = CNIL;	/* protect function */
  CONT_PRV(cont) = CNIL;	/* during protect function return */
  return(cont);
}

value arc_mkcont(arc *c, value offset, value thr)
{
  value cont = arc_mkvector(c, 8);
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
  WB(&CONT_ECR(cont), TECONT(thr));
  WB(&CONT_PRT(cont), CNIL);	/* protect function */
  WB(&CONT_PRV(cont), CNIL);	/* during protect function return */
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

  env = cons(c, arc_mkvector(c, size+1), parent);
  BTYPE(env) = T_ENV;
  return(env);
}

value arc_mkclosure(arc *c, value code, value env)
{
  value clos;

  clos = cons(c, code, env);
  BTYPE(clos) = T_CLOS;
  return(clos);
}

/* Main dispatcher.  Will run each thread in the thread list for
   at most c->quanta cycles or until the thread leaves ready state.
   Also runs garbage collector threads periodically.  This should
   be called with at least one thread already in the run queue.
   Terminates when no more threads can run. */
void arc_thread_dispatch(arc *c)
{
  value prev = CNIL, thr;
  c->vmqueue = CNIL;
  int nthreads = 0, blockedthreads = 0, ncycles = 0;

  for (;;) {
    /* loop back to the beginning if we hit the nil at the end */
    if (c->vmqueue == CNIL) {
      /* Simple deadlock detection.  We declare deadlock if after two
	 cycles through the run queue we find that all threads are in
	 a blocked state. */
      if (nthreads != 0 && nthreads == blockedthreads && ++ncycles > 2) {
	arc_err_cstrfmt(c, "fatal: deadlock detected");
	return;
      } else {
	/* reset cycle counter if we get a state where there are
	   some threads which are not blocked */
	ncycles = 0;
      }
      c->vmqueue = c->vmthreads;
      prev = CNIL;
      /* No more available threads, exit */
      if (c->vmqueue == CNIL)
	return;
      /* reset thread counts */
      nthreads = 0;
      blockedthreads = 0;
    }
    ++nthreads;
    thr = car(c->vmqueue);
    switch (TSTATE(thr)) {
      /* see if the thread has changed state to Trelease or
       Tbroken.  Remove the thread from the queue if so. */
    case Trelease:
    case Tbroken:
      /* dequeue the terminated thread */
      if (prev == CNIL) {
	WB(&c->vmthreads, cdr(c->vmqueue));
      } else {
	scdr(prev, cdr(c->vmqueue));
      }
      break;
    case Talt:
    case Tsend:
    case Trecv:
      /* increment count of threads in blocked states */
      ++blockedthreads;
      break;
    case Tsleep:
      /* Wake up a sleeping thread if the wakeup time is reached */
      if (__arc_milliseconds() >= TWAKEUP(thr))
	TSTATE(thr) = Tready;
      /* fall through -- let the thread run */
    case Tdebug:
      /* this does nothing special for now */
    case Tready:
    case Texiting:
      arc_vmengine(c, thr, c->quantum);
      break;
    }

    /* run a garbage collection cycle */
    c->rungc(c);

    /* queue next thread */
    prev = c->vmqueue;
    c->vmqueue = cdr(c->vmqueue);
  }
}

/* Exception handlers are continuations consed on top of the ECONT
   register.  It is not possible at the moment to make an error handling
   function in C.  I have to think about how to do that. */
value arc_on_err(arc *c, value argv, value rv, CC4CTX)
{
  CC4VDEFBEGIN;
  CC4VDEFEND;
  value fun, env, offset, stk, errcont;
  value errfn, fn, thr = c->curthread;

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
  fun = car(errfn);
  env = cdr(errfn);
  offset = INT2FIX(0);
  stk = CNIL;
  /* the ECONT register contains a list of all error continuations.  It's
     essentially syntactic sugaring around call/cc. */
  errcont = arc_mkcontfull(c, offset, fun, env, stk, TCONR(thr), TECONT(thr));
  WB(&TECONT(thr), cons(c, errcont, TECONT(thr)));
  CC4BEGIN(c);
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

/* When an error is raised, we first look through the ECONT register.
   If the ECONT register is not null, we will pull the topmost entry
   and the restore that continuation.  We will push the exception
   object on the stack so it becomes the parameter of the error
   handler. If the ECONT register is nil, we use the signal_error
   function as the final fallback.
*/
value arc_errexc(arc *c, value exc)
{
  value thr, errcont, newconr, cont, pcont, endcont;

  thr = c->curthread;
  if (NIL_P(TECONT(thr))) {
    /* In the absence of any on-error rescue functions,
       we attempt to build up a new CONR with each of the
       protect functions registered.  The most recent one
       should be restored.  The state of the virtual machine
       is set to Texiting so that when the last such protect
       continuation finishes execution, it will execute
       c->signal_error. */
    newconr = CNIL;
    cont = TCONR(thr);
    TEXC(thr) = exc;
    while (cont != CNIL) {
      pcont = CONT_PRT(car(cont));
      if (pcont != CNIL)
	newconr = cons(c, pcont, newconr);
      cont = cdr(cont);
    }
    if (newconr == CNIL) {
      /* if there is nothing, just signal error and be done with it. */
      c->signal_error(c, VINDEX(exc, 0));
      return(CNIL);
    }
    /* otherwise, reverse the newconr so that the continuations get invoked
       in the proper order. */
    cont = newconr = arc_list_reverse(c, newconr);
    while (cont != CNIL) {
      /* we are cooking up a new continuation register: make sure that
	 the conr's are proper. */
      CONT_CON(car(cont)) = cdr(cont);
      cont = cdr(cont);
    }
    cont = car(newconr);
    newconr = cdr(newconr);
    TSTATE(thr) = Texiting;
    TCONR(thr) = newconr;	/* replace the continuation register */
    arc_restorecont(c, thr, cont);
    longjmp(TVJMP(thr), 0);
  }

  errcont = car(TECONT(thr));
  WB(&TECONT(thr), cdr(TECONT(thr)));

  /* Now, we have to go through the CONR in the same way as above,
     adding continuations with registered error handlers as we go.
     We should stop when we reach the portion of the continuation
     register specified in CONT_CON of the errcont (which is where
     the error handler was inserted).
  */
  endcont = car(CONT_CON(errcont));
  newconr = CNIL;
  cont = TCONR(thr);
  while (cont != CNIL && cont != endcont) {
    pcont = CONT_PRT(car(cont));
    if (pcont != CNIL)
      newconr = cons(c, pcont, newconr);
    cont = cdr(cont);
  }

  if (newconr == CNIL) {
    /* if there is nothing, just jump to the error continuation and be
     done. */
    arc_restorecont(c, thr, errcont);
    CPUSH(thr, exc);
    /* jump back to the start of the virtual machine context executing
       this. */
    longjmp(TVJMP(thr), 0);
  }
  /* otherwise, combine with the error continuation and reverse to make
     the new continuation register, but first put the exception into
     the saved stack */
  CONT_STK(errcont) = arc_mkvector(c, 1);
  VINDEX(CONT_STK(errcont), 0) = exc;
  errcont = cons(c, errcont, CONT_CON(errcont));
  newconr = arc_list_reverse(c, newconr);
  cont = newconr = arc_list_append(newconr, errcont);
  while (cont != CNIL) {
    /* we are cooking up a new continuation register: make sure that
       the conr's are proper. */
    CONT_CON(car(cont)) = cdr(cont);
    cont = cdr(cont);
  }
  cont = car(newconr);
  newconr = cdr(newconr);
  TCONR(thr) = newconr;	       /* replace the continuation register */
  arc_restorecont(c, thr, cont);
  longjmp(TVJMP(thr), 0);
  return(CNIL);
}

value arc_err_cstrfmt2(arc *c, const char *lastcall, const char *fmt, ...)
{
  va_list ap;
  char text[ESTRMAX];
  value errtext;

  va_start(ap, fmt);
  /* XXX - make another formatting function, vsnprintf won't do for
     some of the other stuff we want to be able to do. */
  vsnprintf(text, ESTRMAX, fmt, ap);
  va_end(ap);
  errtext = arc_mkstringc(c, text);
  return(arc_errexc(c, arc_mkexception(c, errtext, arc_mkstringc(c, lastcall), TCONR(c->curthread))));
}

value arc_err(arc *c, value emsg)
{
  value thr = c->curthread;

  return(arc_errexc(c, arc_mkexception(c, emsg, CODE_NAME(TFUNR(thr)),
				       TCONR(thr))));
}

value arc_exc_details(arc *c, value exc)
{
  return(VINDEX(exc, 0));
}

value arc_callcc(arc *c, value argv, value rv, CC4CTX)
{
  CC4VDEFBEGIN;
  CC4VDEFEND;
  value thr = c->curthread, func;

  if (VECLEN(argv) != 1) {
    arc_err_cstrfmt(c, "procedure ccc: expects 1 argument, given %d",
		    VECLEN(argv));
    return(CNIL);
  }
  func = VINDEX(argv, 0);
  CC4BEGIN(c);
  CC4CALL(c, argv, func, 1, car(TCONR(thr)));
  CC4END;
  return(rv);
}

value arc_protect(arc *c, value argv, value rv, CC4CTX)
{
  CC4VDEFBEGIN;
  CC4VDEFEND;
  value during, after, acont, thr = c->curthread;
  value fun, env, offset, stk;

  if (VECLEN(argv) != 2) {
    arc_err_cstrfmt(c, "procedure protect: expects 2 arguments, given %d",
		    VECLEN(argv));
    return(CNIL);
  }
  during = VINDEX(argv, 0);
  after = VINDEX(argv, 1);
  if (TYPE(during) != T_CLOS) {
    arc_err_cstrfmt(c, "procedure protect: closure required for first arg");
    return(CNIL);
  }

  if (TYPE(after) != T_CLOS) {
    arc_err_cstrfmt(c, "procedure protect: closure required for second arg");
    return(CNIL);
  }

  /* The continuation at the top of the stack is the continuation
     created by the call to 'protect'.  Without the protect call,
     it would have been created for the call to the during function.
     Patch the slot in that continuation for the after procedure. */
  fun = car(after);
  env = cdr(after);
  offset = INT2FIX(0);
  stk = CNIL;
  acont = arc_mkcontfull(c, offset, fun, env, stk, TCONR(thr), TECONT(thr));
  WB(&VINDEX(car(TCONR(thr)), 6), acont);
  CC4BEGIN(c);
  CC4CALL(c, argv, during, 0, CNIL);
  CC4END;
  return(rv);
}
