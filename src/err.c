/* Copyright (C) 2017, 2018 Rafael R. Sevilla

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
#include <stdarg.h>
#include <stdio.h>
#include <setjmp.h>
#include <errno.h>
#include <string.h>
#include "arcueid.h"
#include "vmengine.h"

/* This is going to get a lot more complicated later once we implement
   error handling and dynamic-wind. */
void arc_err_cstr(arc *c, value fileline, const char *fmt, ...)
{
  va_list ap;
  char cstr[1000];

  va_start(ap, fmt);
  vsnprintf(cstr, sizeof(char)*1000, fmt, ap);
  fputs(cstr, stderr);
  va_end(ap);
  longjmp(((arc_thread *)c->curthread)->errjmp, 2);
}

/* Report a fatal error */
void __arc_fatal(const char *errmsg, int errnum)
{
  char serrmsg[1024];

  if (errnum > 0) {
    strerror_r(errnum, serrmsg, sizeof(serrmsg)/sizeof(char));
    fprintf(stderr, "FATAL: %s (%s)", errmsg, serrmsg);
  } else {
    fprintf(stderr, "FATAL: %s", errmsg);
  }
  exit(1);
}
