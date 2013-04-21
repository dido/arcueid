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
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include "arcueid.h"

value arc_dir(arc *c, value dirname)
{
  char *utf_filename;
  DIR *dirp;
  int en;
  value dirlist;
  struct dirent *entry, *result;
  int delen;

  TYPECHECK(dirname, T_STRING);
  utf_filename = alloca(FIX2INT(arc_strutflen(c, dirname)) + 1);
  arc_str2cstr(c, dirname, utf_filename);
  dirp = opendir(utf_filename);
  if (dirp == NULL) {
    en = errno;
    arc_err_cstrfmt(c, "dir: cannot open directory \"%s\", (%s; errno=%d)", utf_filename, strerror(en), en);
    return(CNIL);
  }
  dirlist = CNIL;
  delen = offsetof(struct dirent, d_name)
    + pathconf(utf_filename, _PC_NAME_MAX) + 1;
  entry = (struct dirent *)alloca(delen);
  for (;;) {
    if (readdir_r(dirp, entry, &result) != 0) {
      /* error */
      en = errno;
      arc_err_cstrfmt(c, "dir: error reading directory \"%s\", (%s; errno=%d)", utf_filename, strerror(en), en);
      return(CNIL);
    }
    /* end of list */
    if (result == NULL)
      break;
    /* ignore the . and .. directories */
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;
    dirlist = cons(c, arc_mkstringc(c, entry->d_name), dirlist);
  }
  return(dirlist);
}
