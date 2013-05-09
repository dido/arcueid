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

/*
 *  substitution list
 */
#define NSUBEXP 32
typedef struct Resublist	Resublist;
struct Resublist
{
  Resub	m[NSUBEXP];
};

/* max character classes per program */
extern Reprog RePrOg;
#define	NCLASS (sizeof(RePrOg.class)/sizeof(Reclass))

/* max rune ranges per character class */
#define NCCRUNE	(sizeof(Reclass)/sizeof(Rune))

/*
 * Actions and Tokens (Reinst types)
 *
 *	02xx are operators, value == precedence
 *	03xx are tokens, i.e. operands for operators
 */
#define RUNE		0177
#define	OPERATOR	0200	/* Bitmask of all operators */
#define	START		0200	/* Start, used for marker on stack */
#define	RBRA		0201	/* Right bracket, ) */
#define	LBRA		0202	/* Left bracket, ( */
#define	OR		0203	/* Alternation, | */
#define	CAT		0204	/* Concatentation, implicit operator */
#define	STAR		0205	/* Closure, * */
#define	PLUS		0206	/* a+ == aa* */
#define	QUEST		0207	/* a? == a|nothing, i.e. 0 or 1 a's */
#define	ANY		0300	/* Any character except newline, . */
#define	ANYNL		0301	/* Any character including newline, . */
#define	NOP		0302	/* No operation, internal use only */
#define	BOL		0303	/* Beginning of line, ^ */
#define	EOL		0304	/* End of line, $ */
#define	CCLASS		0305	/* Character class, [] */
#define	NCCLASS		0306	/* Negated character class, [] */
#define	END		0377	/* Terminate: match found */

/*
 *  regexec execution lists
 */
#define LISTSIZE	100
#define BIGLISTSIZE	(10*LISTSIZE)

typedef struct Relist	Relist;

struct Relist
{
  Reinst *inst;		   /* Reinstruction of the thread */
  Resublist se;		   /* matched subexpressions in this thread */
};

typedef struct Reljunk	Reljunk;

struct	Reljunk
{
  struct arc *c;
  Relist *relist[2];
  Relist *reliste[2];
  int starttype;
  Rune startchar;
  int rstarts;
  value str;
  int bol;
  int reol;
  int matchstart;
};

extern Relist*	_renewthread(Relist*, Reinst*, Resublist*);
extern void	_renewmatch(Resub*, int, Resublist*);
extern Relist*	_rrenewemptythread(Relist*, Reinst*, int);
