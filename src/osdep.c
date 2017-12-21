/* 
  Copyright (C) 2017 Rafael R. Sevilla

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
#include "config.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "arcueid.h"

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

unsigned long long __arc_milliseconds(void)
{
#ifdef HAVE_CLOCK_GETTIME
  struct timespec tp;
  unsigned long long t;

  if (clock_gettime(CLOCK_REALTIME, &tp) < 0) {
    /* fall back to using time(2) if we have an error */
    return((unsigned long long)time(NULL)*1000LL);
  }
  t = ((unsigned long long)tp.tv_sec)*1000LL
    + ((unsigned long long)tp.tv_nsec / 1000000LL);
  return(t);
#else
  /* fall back to using time(2) if clock_gettime is not available */
  return((unsigned long long)time(NULL)*1000LL);
#endif
}
