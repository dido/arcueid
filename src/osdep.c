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

void __arc_sleep(unsigned long long st)
{
#ifdef HAVE_NANOSLEEP
  struct timespec req;
  req.tv_sec = st/1000;
  req.tv_nsec = ((st % 1000) * 1000000L);
  nanosleep(&req, NULL);
#elif HAVE_USLEEP
  usleep(st * 1000);
#else
#error No sleep function available
#endif
}
