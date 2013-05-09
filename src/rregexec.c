/*
	Copyright © 1994-1999 Lucent Technologies Inc.
	Revisions Copyright © 2000-2003 Vita Nuova Holdings Limited
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
#include "utf.h"
#include "regexp.h"
#include "regcomp.h"

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

/*
 *  return	0 if no match
 *		>0 if a match
 *		<0 if we ran out of _relist space
 */
static int
rregexec1(Reprog *progp,	/* program to run */
	  value str,		/* string to run machine on */
	  Resub *mp,		/* subexpression elements */
	  int ms,		/* number of elements at mp */
	  Reljunk *j)
{
  int flag=0;
  Reinst *inst;
  Relist *tlp;
  int i, checkstart, ptr, len;
  Rune r, *rp, *ep;
  Relist* tl;		/* This list, next list */
  Relist* nl;
  Relist* tle;		/* ends of this and next list */
  Relist* nle;
  int match;
  arc *c;

  c = j->c;
  len = arc_strlen(c, str);
  j->matchstart = -1;
  match = 0;
  checkstart = j->startchar;
  if (mp) {
    for(i=0; i<ms; i++) {
      mp[i].csp = -1;
      mp[i].cep = -1;
    }
  }

  j->relist[0][0].inst = 0;
  j->relist[1][0].inst = 0;

  /* Execute machine once for each character, including terminal NUL */
  ptr = j->rstarts;
  do {
    /* fast check for first char */
    if (checkstart) {
      switch (j->starttype) {
      case RUNE:
	while (arc_strindex(c, str, ptr) != j->startchar) {
	  if (ptr >= len || ptr == j->reol)
	    return(match);
	  ptr++;
	}
	break;
      case BOL:
	if (ptr == 0)
	  break;
	while(arc_strindex(c, str, ptr) != '\n') {
	  if (ptr == len || ptr == j->reol)
	    return(match);
	  ptr++;
	}
	break;
      }
    }

    r = (ptr < len) ? arc_strindex(c, str, ptr) : -1;

    /* switch run lists */
    tl = j->relist[flag];
    tle = j->reliste[flag];
    nl = j->relist[flag^=1];
    nle = j->reliste[flag];
    nl->inst = 0;

    /* Add first instruction to current list */
    _rrenewemptythread(tl, progp->startinst, ptr);

    /* Execute machine until current list is empty */
    for (tlp=tl; tlp->inst; tlp++) {
      for (inst=tlp->inst; ; inst = inst->u2.next) {
	switch (inst->type) {
	case RUNE:	/* regular character */
	  if(inst->u1.r == r) {
	    if (_renewthread(nl, inst->u2.next, &tlp->se)==nle)
	      return(-1);
	  }
	  break;
	case LBRA:
	  tlp->se.m[inst->u1.subid].csp = ptr;
	  continue;
	case RBRA:
	  tlp->se.m[inst->u1.subid].cep = ptr;
	  continue;
	case ANY:
	  if (r != '\n') {
	    if (_renewthread(nl, inst->u2.next, &tlp->se)==nle)
	      return(-1);
	  }
	  break;
	case ANYNL:
	  if (_renewthread(nl, inst->u2.next, &tlp->se)==nle)
	    return(-1);
	  break;
	case BOL:
	  if (ptr == 0 || arc_strindex(c, str, ptr-1) == '\n')
	    continue;
	  break;
	case EOL:
	  if (ptr == j->reol || r == '\n')
	    continue;
	  break;
	case CCLASS:
	  ep = inst->u1.cp->end;
	  for (rp = inst->u1.cp->spans; rp < ep; rp += 2) {
	    if(r >= rp[0] && r <= rp[1]) {
	      if (_renewthread(nl, inst->u2.next, &tlp->se)==nle)
		return(-1);
	      break;
	    }
	  }
	  break;
	case NCCLASS:
	  ep = inst->u1.cp->end;
	  for (rp = inst->u1.cp->spans; rp < ep; rp += 2) {
	    if(r >= rp[0] && r <= rp[1])
	      break;
	  }
	  if (rp == ep) {
	    if (_renewthread(nl, inst->u2.next, &tlp->se)==nle)
	      return(-1);
	  }
	  break;
	case OR:
	  /* evaluate right choice later */
	  if (_renewthread(tlp, inst->u1.right, &tlp->se) == tle)
	    return(-1);
	  /* efficiency: advance and re-evaluate */
	  continue;
	case END:	/* Match! */
	  match = 1;
	  if (j->matchstart < 0)
	    j->matchstart = tlp->se.m[0].csp;
	  tlp->se.m[0].cep = ptr;
	  if (mp != 0)
	    _renewmatch(mp, ms, &tlp->se);
	  break;
	}
	break;
      }
    }

    if (ptr == j->reol)
      break;
    checkstart = j->startchar && nl->inst==0;
    ptr++;
  } while (r >= 0);
  return(match);
}

#define MAX_RETRIES 4

static int
rregexec2(Reprog *progp,	/* program to run */
	  value str,		/* string to run machine on */
	  Resub *mp,		/* subexpression elements */
	  int ms,		/* number of elements at mp */
	  Reljunk *j,
	  int listsize)
{
  Relist *relist0, *relist1;

  relist0 = (Relist *)alloca(listsize*sizeof(Relist));
  relist1 = (Relist *)alloca(listsize*sizeof(Relist));
  /* mark space */
  j->relist[0] = relist0;
  j->relist[1] = relist1;
  j->reliste[0] = relist0 + nelem(relist0) - 2;
  j->reliste[1] = relist1 + nelem(relist1) - 2;
  return(rregexec1(progp, str, mp, ms, j));
}

extern int
rregexec(arc *c,
	 Reprog *progp,	/* program to run */
	 value str,	/* string to run machine on */
	 Resub *mp,	/* subexpression elements */
	 int ms)	/* number of elements at mp */
{
  Reljunk j;
  int rv, listsize = LISTSIZE, i;

  j.c = c;
  /* use user-specified starting/ending location if specified */
  j.rstarts = 0;
  j.reol = arc_strlen(c, str);
  if (mp && ms>0) {
    if (mp->csp >= 0)
      j.rstarts = mp->csp;
    if (mp->cep >= 0)
      j.reol = mp->cep;
  }
  j.starttype = 0;
  j.startchar = 0;
  if (progp->startinst->type == RUNE
      && progp->startinst->u1.r < (Rune)Runeself) {
    j.starttype = RUNE;
    j.startchar = progp->startinst->u1.r;
  }

  if (progp->startinst->type == BOL)
    j.starttype = BOL;

  for (i=0; i<MAX_RETRIES; i++) {
    rv = rregexec2(progp, str, mp, ms, &j, listsize);
    if (rv >= 0)
      return(j.matchstart);
    listsize *= 5;
  }
  return(-2);
}
