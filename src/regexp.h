/*
	Copyright (c) 1994-1999 Lucent Technologies Inc.
	Revisions Copyright (c) 2000-2003 Vita Nuova Holdings Limited
	(www.vitanuova.com).

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge,
  publish, distribute, sublicense, and/or sell copies of the Software,
  and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

typedef struct Resub		Resub;
typedef struct Reclass		Reclass;
typedef struct Reinst		Reinst;
typedef struct Reprog		Reprog;

/* Sub expression matches.  These are string indexes rather than pointers
   as in the original. */
struct Resub {
  int csp;
  int cep;
};

/*
 *	character class, each pair of rune's defines a range
 */
struct Reclass {
  Rune *end;
  Rune spans[64];
};

/*
 *	Machine instructions
 */
struct Reinst {
  int type;
  union	{
    Reclass	*cp;	     /* class pointer */
    Rune	r;	     /* character */
    int	subid;		     /* sub-expression id for RBRA and LBRA */
    Reinst	*right;	     /* right child of OR */
  } u1;
  union {     /* regexp relies on these two being in the same union */
    Reinst *left;		/* left child of OR */
    Reinst *next;		/* next instruction for CAT & LBRA */
  } u2;
};

/*
 *	Reprogram definition
 */
struct Reprog {
  Reinst *startinst;	/* start pc */
  Reclass class[16];	/* .data */
  Reinst firstinst[5];	/* .text */
};

extern Reprog *regcomp(arc *, value);
extern Reprog *regcomplit(arc *, value);
extern Reprog *regcompnl(arc *, value);
extern int rregexec(arc *, Reprog *, value, Resub *, int);

#define REGEXP_MULTILINE 1
#define REGEXP_CASEFOLD 2

extern value arc_mkregexp(arc *c, value s, unsigned int flags);
extern int arc_regcomp(arc *c, value thr);
extern value arc_regexp_match(arc *c, value regexp, value str);

