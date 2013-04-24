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
#include "arcueid.h"
#include "builtins.h"

value arc_intern(arc *c, value name)
{
  value symid, symval;
  int symintid;

  if ((symid = arc_hash_lookup(c, c->symtable, name)) != CUNBOUND) {
    /* convert the fixnum ID into the symbol value */
    symval = ID2SYM(FIX2INT(symid));
    /* do not allow nil or t to have a symbol value */
    if (symval == ARC_BUILTIN(c, S_NIL))
      symval = CNIL;
    /*    else if (symval == ARC_BUILTIN(c, S_T)) 
	  symval = CTRUE; */
    return(symval);
  }

  symintid = ++c->lastsym;
  symid = INT2FIX(symintid);
  symval = ID2SYM(symintid);
  arc_hash_insert(c, c->symtable, name, symid);
  arc_hash_insert(c, c->rsymtable, symid, name);
  return(symval);
}

value arc_intern_cstr(arc *c, const char *name)
{
  value symstr = arc_mkstringc(c, name);
  return(arc_intern(c, symstr));
}

value arc_sym2name(arc *c, value sym)
{
  value symid;

  symid = INT2FIX(SYM2ID(sym));
  return(arc_hash_lookup(c, c->rsymtable, symid));
}

value arc_unintern(arc *c, value sym)
{
  value symid, name;

  symid = INT2FIX(SYM2ID(sym));
  name = arc_hash_lookup(c, c->rsymtable, symid);
  if (name == CUNBOUND)
    return(CNIL);
  arc_hash_delete(c, c->symtable, name);
  arc_hash_delete(c, c->rsymtable, symid);
  return(CTRUE);
}

/* We must synchronize this against builtin_syms in builtins.h as necessary! */
static char *syms[] = { "fn", "_", "quote", "quasiquote", "unquote",
			"unquote-splicing", "compose", "complement",
			"t", "nil", "no", "andf", "get", "sym",
			"fixnum", "bignum", "flonum", "rational",
			"complex", "char", "string", "cons", "table",
			"input", "output", "exception", "port",
			"thread", "vector", "continuation", "closure",
			"code", "environment", "vmcode", "ccode",
			"custom", "int", "unknown", "re", "im", "num",
			"sig", "stdin-fd", "stdout-fd", "stderr-fd",
			"mac", "if", "assign", "o", ".", "car", "cdr",
			"scar", "scdr", "is", "+", "-", "*", "/",
			"and", "apply", "chan", "AF_UNIX", "AF_INET",
			"AF_INET6", "SOCK_STREAM", "SOCK_DGRAM",
			"SOCK_RAW", "binary", "text", "append",
			"atstrings", "lndata" };

static struct {
  char *str;
  Rune val;
} chartbl[] = {
  { "null", 0 }, { "nul", 0 }, { "backspace", 8 }, { "tab", 9 },
  { "newline", 10 }, { "vtab", 11 }, { "page", 12 }, { "return", 13 },
  { "space", 32 }, { "rubout", 127 }, { NULL, -1 }
};

void arc_init_symtable(arc *c)
{
  int i;

  c->symtable = arc_mkwtable(c, ARC_HASHBITS);
  c->rsymtable = arc_mkwtable(c, ARC_HASHBITS);
  c->lastsym = 0;

  /* Set up builtin symbols */
  SVINDEX(c->builtins, BI_syms, arc_mkvector(c, S_THE_END));
  for (i=0; i<S_THE_END; i++)
    SARC_BUILTIN(c, i, arc_intern(c, arc_mkstringc(c, syms[i])));

  /* Set up character escape table */
  SVINDEX(c->builtins, BI_charesc, arc_mkhash(c, ARC_HASHBITS));
  for (i=0; chartbl[i].str; i++) {
    value str = arc_mkstringc(c, chartbl[i].str);
    value chr = arc_mkchar(c, chartbl[i].val);

    arc_hash_insert(c, VINDEX(c->builtins, BI_charesc), str, chr);
    arc_hash_insert(c, VINDEX(c->builtins, BI_charesc), chr, str);
  }
  c->ctrue = ARC_BUILTIN(c, S_T);
}

static AFFDEF(symbol_pprint)
{
  AARG(sexpr, disp, fp);
  AOARG(visithash);
  AFBEGIN;
  (void)visithash;
  (void)disp;
  AFTCALL(arc_mkaff(c, arc_disp, CNIL), arc_sym2name(c, AV(sexpr)), AV(fp));
  AFEND;
}
AFFEND

static AFFDEF(symbol_xcoerce)
{
  AARG(obj, stype, arg);
  AFBEGIN;
  (void)arg;
  if (FIX2INT(AV(stype)) == T_SYMBOL)
    ARETURN(AV(obj));

  if (FIX2INT(AV(stype)) == T_STRING)
    ARETURN(arc_sym2name(c, AV(obj)));

  arc_err_cstrfmt(c, "cannot coerce");
  ARETURN(CNIL);
  AFEND;
}
AFFEND

typefn_t __arc_symbol_typefn__ = {
  NULL,
  NULL,
  symbol_pprint,
  NULL,
  NULL,
  NULL,
  NULL,
  symbol_xcoerce
};
