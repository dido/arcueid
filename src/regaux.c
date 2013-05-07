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
#include <string.h>
#include "utf.h"
#include "regexp.h"
#include "regcomp.h"


/*
 *  save a new match in mp
 */
extern void
_renewmatch(Resub *mp, int ms, Resublist *sp)
{
	int i;

	if(mp==0 || ms<=0)
		return;
	if(mp[0].s.sp==0 || sp->m[0].s.sp<mp[0].s.sp ||
	   (sp->m[0].s.sp==mp[0].s.sp && sp->m[0].e.ep>mp[0].e.ep)){
		for(i=0; i<ms && i<NSUBEXP; i++)
			mp[i] = sp->m[i];
		for(; i<ms; i++)
			mp[i].s.sp = mp[i].e.ep = 0;
	}
}

/*
 * Note optimization in _renewthread:
 * 	*lp must be pending when _renewthread called; if *l has been looked
 *		at already, the optimization is a bug.
 */
extern Relist*
_renewthread(Relist *lp,	/* _relist to add to */
	Reinst *ip,		/* instruction to add */
	Resublist *sep)		/* pointers to subexpressions */
{
	Relist *p;

	for(p=lp; p->inst; p++){
		if(p->inst == ip){
			if((sep)->m[0].s.sp < p->se.m[0].s.sp)
				p->se = *sep;
			return 0;
		}
	}
	p->inst = ip;
	p->se = *sep;
	(++p)->inst = 0;
	return p;
}

/*
 * same as renewthread, but called with
 * initial empty start pointer.
 */
extern Relist*
_renewemptythread(Relist *lp,	/* _relist to add to */
	Reinst *ip,		/* instruction to add */
	char *sp)		/* pointers to subexpressions */
{
	Relist *p;

	for(p=lp; p->inst; p++){
		if(p->inst == ip){
			if(sp < p->se.m[0].s.sp) {
				memset((void *)&p->se, 0, sizeof(p->se));
				p->se.m[0].s.sp = sp;
			}
			return 0;
		}
	}
	p->inst = ip;
	memset((void *)&p->se, 0, sizeof(p->se));
	p->se.m[0].s.sp = sp;
	(++p)->inst = 0;
	return p;
}

extern Relist*
_rrenewemptythread(Relist *lp,	/* _relist to add to */
	Reinst *ip,		/* instruction to add */
	Rune *rsp)		/* pointers to subexpressions */
{
	Relist *p;

	for(p=lp; p->inst; p++){
		if(p->inst == ip){
			if(rsp < p->se.m[0].s.rsp) {
				memset((void *)&p->se, 0, sizeof(p->se));
				p->se.m[0].s.rsp = rsp;
			}
			return 0;
		}
	}
	p->inst = ip;
	memset((void *)&p->se, 0, sizeof(p->se));
	p->se.m[0].s.rsp = rsp;
	(++p)->inst = 0;
	return p;
}
