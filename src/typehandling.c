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
/* Type handling and conversions */
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "arcueid.h"
#include "arith.h"
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
  { "num", CNIL },
  { "nil", CNIL },
  { "t", CNIL },
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
  IDX_im,
  IDX_num,
  IDX_nil,
  IDX_t
};

value __arc_init_typesyms(arc *c)
{
  int i;

  for (i=0; typesyms[i].name != NULL; i++)
    typesyms[i].sym = arc_intern_cstr(c, typesyms[i].name);
  return(CNIL);

}

/* Determine numeric base from argument 2 of a coerce */
static value numeric_base(arc *c, value argv, const char *caller)
{
  value base = INT2FIX(10);

  /* Arcueid extension, bases from 2 to 36 are supported */
  if (VECLEN(argv) >= 3) {
    base = VINDEX(argv, 2);
    if (TYPE(base) != T_FIXNUM) {
      c->signal_error(c, "%s, invalid base specifier %O", caller, base);
      return(CNIL);
    }

    if (FIX2INT(base) < 2 || FIX2INT(base) > 36) {
      c->signal_error(c, "%s, out of range base %O", caller, base);
      return(CNIL);
    }
  }
  return(base);
}

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

static value str2int(arc *c, value obj, value base, int strptr, int limit)
{
  /* Tell me if this looks too much like glibc's implementation of
     strtol... */
  value res;
  int save, negative;
  Rune r;

  if (strptr >= limit)
    goto noconv;
  if (arc_strindex(c, obj, strptr) == '-') {
    negative = 1;
    ++strptr;
  } else if (arc_strindex(c, obj, strptr) == '+') {
    negative = 0;
    ++strptr;
  } else
    negative = 0;
  /* save the pointer so we can check later if anything happened */
  save = strptr;
  res = INT2FIX(0);
  for (r = arc_strindex(c, obj, strptr); strptr < limit; r = arc_strindex(c, obj, ++strptr)) {
    if (isdigit(r))
      r -= '0';
    else if (isalpha(r))
      r = toupper(r) - 'A' + 10;
    else
      goto noconv;
    if (r > FIX2INT(base))
      goto noconv;
    res = __arc_mul2(c, res, base);
    res = __arc_add2(c, res, INT2FIX(r));
  }

  /* check if anything actually happened */
  if (strptr == save)
    goto noconv;

  if (negative)
    res = __arc_neg(c, res);
  return(res);
 noconv:
  return(CNIL);
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
  case T_STRING: {
    value base = numeric_base(c, argv, "string->int");

    res = str2int(c, obj, base, 0, arc_strlen(c, obj));
    if (res != CNIL)
      return(res);
    /* fall through otherwise, go to default error */
  }
  default:
    c->signal_error(c, "string->int cannot coerce %O to integer type", obj);
    break;
  }
  return(CNIL);
}

static int digitval(Rune r, int base)
{
  int v;

  if (isdigit(r)) {
    v = r - '0';
  } else if (isalpha(r)) {
    v = toupper(r) - 'A' + 10;
  } else
    return(-1);

  if (v >= base)
    return(-1);
  return(v);
}

static value str2flo(arc *c, value obj, value b, int strptr, int limit)
{
  int sign;
  double num;			/* the number so far */
  int got_dot;			/* found a (decimal) point */
  int got_digit;		/* seen any digits */
  value exponent;		/* exponent of the number */
  Rune r;
  int dv;			/* digit value */
  int base;			/* base */
  value res;
  char str[4];

  base = FIX2INT(b);
  if (strptr >= limit)
    goto noconv;

  /* Get the sign */
  r = arc_strindex(c, obj, strptr);
  sign = (r == '-') ? -1 : 1;
  if (r == '-' || r == '+')
    ++strptr;

  num = 0.0;
  got_dot = 0;
  got_digit = 0;
  exponent = INT2FIX(0);

  /* Check special cases of INF and NAN */
  if (limit - strptr == 3) {
    int i;

    for (i=0; i<3; i++)
      str[i] = (char)arc_strindex(c, obj, strptr + i);
    str[3] = 0;
    if (strcasecmp(str, "INF") == 0)
      return(arc_mkflonum(c, sign*INFINITY));
    if (strcasecmp(str, "NAN") == 0)
      return(arc_mkflonum(c, sign*NAN));
  }
  for (;; ++strptr) {
    if (strptr >= limit)
      break;
    r = arc_strindex(c, obj, strptr);
    dv = digitval(r, base);
    if (dv >= 0) {
      got_digit = 1;
      num = (num * (double)base) + (double)dv;
      /* Keep track of the number of digits after the decimal point.
	 If we just divided by base here, we would lose precision. */
      if (got_dot)
	exponent = INT2FIX(FIX2INT(exponent) - 1);
    } else if (!got_dot && r == '.') {
      /* XXX - should we obey locale specifications for decimal points? */
      got_dot = 1;
    } else {
      /* any other character terminates the number */
      break;
    }
  }

  if (!got_digit)
    goto noconv;

  if (strptr < limit) {
    r = tolower(arc_strindex(c, obj, strptr++));
    if (r == 'e' || r == 'p' || r == '&') {
      /* get the exponent specified after the 'e', 'p', or '&' */
      value exp;

      exp = str2int(c, obj, b, strptr, limit);

#ifdef HAVE_GMP_H
      if (TYPE(exp) == T_BIGNUM) {
	/* The exponent overflowed a fixnum.  It is probably a safe assumption
	   that an exponent that needs to be represented by a bignum exceeds
	   the limits of a double! */
	if (mpz_cmp_si(REP(exp)._bignum, 0) < 0)
	  return(arc_mkflonum(c, 0.0));
	else
	  return(arc_mkflonum(c, INFINITY));
      }
      exponent = __arc_add2(c, exponent, exp);
#endif
      if (num == 0.0)
	return(arc_mkflonum(c, 0.0));

#ifdef HAVE_GMP_H
      /* check the exponent again */
      if (TYPE(exponent) == T_BIGNUM) {
	if (mpz_cmp_si(REP(exp)._bignum, 0) < 0)
	  return(arc_mkflonum(c, 0.0));
	else
	  return(arc_mkflonum(c, INFINITY));
      }
    }
#endif
  }
  /* Multiply NUM by BASE to the EXPONENT power */
  res = __arc_mul2(c, arc_mkflonum(c, sign*num),
		   arc_expt(c, arc_mkflonum(c, arc_coerce_flonum(c, b)),
			    arc_mkflonum(c, arc_coerce_flonum(c, exponent))));
  return(res);
 noconv:
  return(CNIL);
}

static value coerce_flonum(arc *c, value obj, value argv)
{
  switch (TYPE(obj)) {
  case T_FLONUM:
    /* null conversion */
    return(obj);
  case T_FIXNUM:
  case T_BIGNUM:
  case T_RATIONAL:
    return(arc_mkflonum(c, arc_coerce_flonum(c, obj)));
  case T_COMPLEX:
    return((VECLEN(argv) >= 3 && VINDEX(argv, 2) == typesyms[IDX_im].sym) ?
	   arc_mkflonum(c, REP(obj)._complex.im)
	   : arc_mkflonum(c, REP(obj)._complex.re));
  case T_STRING:
    {
      value val, base = numeric_base(c, argv, "string->flonum");

      val = str2flo(c, obj, base, 0, arc_strlen(c, obj));
      if (val == CNIL) {
	c->signal_error(c, "string->flonum cannot convert %O to a flonum", obj);
	return(CNIL);
      }
      return(val);
    }
    break;
  default:
    c->signal_error(c, "cannot coerce %O to flonum type", obj);
    break;
  }
  return(CNIL);
}

static value coerce_rational(arc *c, value obj, value argv)
{
#ifdef HAVE_GMP_H
  value val;

  switch (TYPE(obj)) {
  case T_FIXNUM:
  case T_BIGNUM:
  case T_RATIONAL:
    /* null conversion */
    return(obj);
  case T_COMPLEX:
    obj = (VECLEN(argv) >= 3 && VINDEX(argv, 2) == typesyms[IDX_im].sym) ?
      arc_mkflonum(c, REP(obj)._complex.im)
      : arc_mkflonum(c, REP(obj)._complex.re);
    /* fall through */
  case T_FLONUM:
    val = arc_mkrationall(c, 0, 1);
    mpq_set_d(REP(val)._rational, REP(obj)._flonum);
    return(val);
  case T_STRING:
    {
      value numer, denom, base = numeric_base(c, argv, "string->rational");
      int slashpos=-1, i;

      for (i=0; i<arc_strlen(c, obj); i++) {
	if (arc_strindex(c, obj, i) == '/') {
	  slashpos = i;
	  break;
	}
      }
      /* No slash, convert as integer */
      if (slashpos < 1)
	return(coerce_integer(c, obj, argv));
      numer = str2int(c, obj, base, 0, slashpos);
      denom = str2int(c, obj, base, slashpos+1, arc_strlen(c, obj));
      if (numer == CNIL || denom == CNIL) {
	c->signal_error(c, "string->flonum cannot convert %O to a rational", obj);
	return(CNIL);
      }
      return(__arc_div2(c, numer, denom));
    }
  default:
    c->signal_error(c, "cannot coerce %O to rational type", obj);
    break;
  }
#endif
  return(CNIL);
}

static value coerce_complex(arc *c, value obj, value argv)
{
  switch (TYPE(obj)) {
  case T_FLONUM:
  case T_COMPLEX:
    return(obj);
  case T_FIXNUM:
  case T_BIGNUM:
  case T_RATIONAL:
    return(coerce_flonum(c, obj, argv));
  case T_CONS:
    return(arc_mkcomplex(c, REP(coerce_flonum(c, car(obj), argv))._flonum,
			 REP(coerce_flonum(c, cdr(obj), argv))._flonum));
  case T_STRING:
    {
      /* To do this, we have to find where the real part ends and
	 the complex part begins.  The real part ends after a + or -
	 that is NOT preceded by an E, P, or &, which is where the
	 imaginary part begins.  The imaginary unit i/j must be at the
	 end of the string.   Once we know where the parts are we can
	 use the functions for extracting floating point numbers to
	 get the real and imaginary parts of the number. */
      value re, im, base = numeric_base(c, argv, "string->complex");
      int reend = -1, i, len, b;
      Rune r;

      b = FIX2INT(base);

      len = arc_strlen(c, obj);
      for (i=0; i<len; i++) {
	r = arc_strindex(c, obj, i);
	if ((r == '-' || r == '+') && i-1 >= 0) {
	  Rune prev = arc_strindex(c, obj, i-1);
	  if (b < 14 && (prev == 'e' || prev == 'E'))
	    continue;
	  if (b < 25 && (prev == 'p' || prev == 'P'))
	    continue;
	  if (prev == '&')
	    continue;
	  reend = i;
	  break;
	}
      }
      if (reend < 0) {
	c->signal_error(c, "cannot find end of real part of number", obj);
	return(CNIL);
      }
      r = tolower(arc_strindex(c, obj, len-1));
      if (r != 'i' && r != 'j') {
	c->signal_error(c, "cannot find end of imaginary part of number", obj);
	return(CNIL);
      }
      re = str2flo(c, obj, base, 0, reend);
      im = str2flo(c, obj, base, reend, len-1);
      if (re == CNIL || im == CNIL) {
	c->signal_error(c, "failed to parse complex number", obj);
	return(CNIL);
      }
      return(arc_mkcomplex(c, REP(re)._flonum, REP(im)._flonum));
    }
  default:
    c->signal_error(c, "cannot coerce %O to complex type", obj);
    break;
  }
  return(CNIL);
}

static value coerce_string(arc *c, value obj, value argv)
{
  value val, base;

  switch (TYPE(obj)) {
  case T_NIL:
    return(arc_mkstringc(c, ""));
  case T_STRING:
    return(obj);
  case T_CHAR:
    /* create a string with only that character */
    val = arc_mkstringlen(c, 1);
    arc_strsetindex(c, val, 0, REP(obj)._char);
    return(val);
  case T_FIXNUM:
  case T_BIGNUM:
    base = numeric_base(c, argv, "int->string");
    return(__arc_itoa(c, obj, base, 0, 0));
  case T_RATIONAL:
    {
      value numstr, denstr;
      base = numeric_base(c, argv, "rational->string");
      numstr = arc_mkbignuml(c, 0);
      mpz_set(REP(numstr)._bignum, mpq_numref(REP(obj)._rational));
      numstr = __arc_itoa(c, numstr, base, 0, 0);
      numstr = arc_strcatc(c, numstr, '/');
      denstr = arc_mkbignuml(c, 0);
      mpz_set(REP(denstr)._bignum, mpq_denref(REP(obj)._rational));
      denstr = __arc_itoa(c, denstr, base, 0, 0);
      return(arc_strcat(c, numstr, denstr));
    }
  case T_COMPLEX:
    {
      /* XXX - radix is ignored */
      char *buf;
      int bufsize = 32, n;

      for (;;) {
	buf = (char *)alloca(bufsize*sizeof(char));
	n = snprintf(buf, bufsize, "%lf%+lfi", REP(obj)._complex.re,
		     REP(obj)._complex.im);
	if (n > 1 && n < bufsize)
	  break;
	if (n > -1)		/* glibc 2.1 */
	  bufsize = n+1;
	else			/* glibc 2.0 */
	  bufsize *= 2;
	/* next loop iteration will allocate more memory if needed */
      }
      return(arc_mkstringc(c, buf));
    }
  case T_FLONUM:
    {
      /* XXX - radix is ignored */
      char *buf;
      int bufsize = 32, n;

      for (;;) {
	buf = (char *)alloca(bufsize*sizeof(char));
	n = snprintf(buf, bufsize, "%lf", REP(obj)._flonum);
	if (n > 1 && n < bufsize)
	  break;
	if (n > -1)		/* glibc 2.1 */
	  bufsize = n+1;
	else			/* glibc 2.0 */
	  bufsize *= 2;
	/* next loop iteration will allocate more memory if needed */
      }
      return(arc_mkstringc(c, buf));
    }
  case T_CONS:
    {
      value carstr, cdrstr;

      carstr = coerce_string(c, car(obj), argv);
      cdrstr = coerce_string(c, cdr(obj), argv);
      return(arc_strcat(c, carstr, cdrstr));
    }
  case T_VECTOR:
    {
      value str;
      int i;

      str = arc_mkstringc(c, "");
      for (i=0; i<VECLEN(obj); i++)
	str = arc_strcat(c, str, coerce_string(c, VINDEX(obj, i), argv));
      return(str);
    }
  case T_SYMBOL:
    return(arc_sym2name(c, obj));
  default:
    c->signal_error(c, "cannot coerce %O to string type", obj);
    break;
  }
  return(CNIL);
}

value coerce_cons(arc *c, value obj, value argv)
{
  switch(TYPE(obj)) {
  case T_CONS:
    return(obj);
  case T_RATIONAL:
    {
      value num, den, t;

      num = arc_mkbignuml(c, 0);
      mpz_set(REP(num)._bignum, mpq_numref(REP(obj)._rational));
      t = arc_coerce_fixnum(c, num);
      if (t != CNIL)
	num = t;
      den = arc_mkbignuml(c, 0);
      mpz_set(REP(den)._bignum, mpq_denref(REP(obj)._rational));
      t = arc_coerce_fixnum(c, den);
      if (t != CNIL)
	den = t;
      return(cons(c, num, den));
    }
  case T_COMPLEX:
    return(cons(c, arc_mkflonum(c, REP(obj)._complex.re),
		arc_mkflonum(c, REP(obj)._complex.im)));
  case T_VECTOR:
    {
      /* Step through the vector backwards to generate the equivalent cons */
      int i;
      value ret = CNIL;

      for (i=VECLEN(obj)-1; i>=0; i--)
	ret = cons(c, VINDEX(obj, i), ret);
      return(ret);
    }
  case T_STRING:
    {
      /* Step through the string backwards to generate the list */
      int i;
      value ret = CNIL;

      for (i=arc_strlen(c, obj)-1; i>=0; i--)
	ret = cons(c, arc_mkchar(c, arc_strindex(c, obj, i)), ret);
      return(ret);
    }
  default:
    c->signal_error(c, "cannot coerce %O to cons type", obj);
    break;
  }
  return(CNIL);
}

value coerce_sym(arc *c, value obj, value argv)
{
  value sym;

  switch (TYPE(obj)) {
  case T_SYMBOL:
    return(obj);
  case T_CHAR:
    sym = arc_mkstringlen(c, 1);
    arc_strsetindex(c, sym, 0, REP(obj)._char);
    return(arc_intern(c, sym));
  case T_STRING:
    sym = arc_intern(c, obj);
    if (sym == typesyms[IDX_nil].sym)
      return(CNIL);
    if (sym == typesyms[IDX_t].sym)
      return(CTRUE);
    return(sym);
  default:
    c->signal_error(c, "cannot coerce %O to sym type", obj);
    break;
  }
  return(CNIL);
}

value coerce_vector(arc *c, value obj, value argv)
{
  switch (TYPE(obj)) {
  case T_STRING:
    {
      int len = arc_strlen(c, obj), i;
      value vec = arc_mkvector(c, len);

      for (i=0; i<len; i++)
	VINDEX(vec, i) = arc_mkchar(c, arc_strindex(c, obj, i));
      return(vec);
    }
  default:
    c->signal_error(c, "cannot coerce %O to vector type", obj);
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

  /* Flonum coercions */
  if (ntype == typesyms[IDX_flonum].sym)
    return(coerce_flonum(c, obj, argv));

  /* Rational coercions */
  if (ntype == typesyms[IDX_rational].sym)
    return(coerce_rational(c, obj, argv));

  /* Complex coercions */
  if (ntype == typesyms[IDX_complex].sym)
    return(coerce_complex(c, obj, argv));

  /* String coercions */
  if (ntype == typesyms[IDX_string].sym)
    return(coerce_string(c, obj, argv));

  /* Cons coercions */
  if (ntype == typesyms[IDX_cons].sym)
    return(coerce_cons(c, obj, argv));

  /* Symbol coercions */
  if (ntype == typesyms[IDX_sym].sym)
    return(coerce_sym(c, obj, argv));

  /* Vector coercions */
  if (ntype == typesyms[IDX_vector].sym)
    return(coerce_vector(c, obj, argv));

  c->signal_error(c, "invalid coercion type specifier %O", ntype);
  return(CNIL);
}

