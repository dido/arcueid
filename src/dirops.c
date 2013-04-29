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
#include <sys/stat.h>
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
  closedir(dirp);
  return(dirlist);
}

value arc_dir_exists(arc *c, value dirname)
{
  char *utf_filename;
  struct stat st;

  TYPECHECK(dirname, T_STRING);
  utf_filename = alloca(FIX2INT(arc_strutflen(c, dirname)) + 1);
  arc_str2cstr(c, dirname, utf_filename);
  if (stat(utf_filename, &st) == -1) {
    return(CNIL);
  }
  if (S_ISDIR(st.st_mode))
    return(dirname);
  return(CNIL);
}

value arc_file_exists(arc *c, value filename)
{
  char *utf_filename;
  struct stat st;

  TYPECHECK(filename, T_STRING);
  utf_filename = alloca(FIX2INT(arc_strutflen(c, filename)) + 1);
  arc_str2cstr(c, filename, utf_filename);
  if (stat(utf_filename, &st) == -1) {
    return(CNIL);
  }
  if (!S_ISDIR(st.st_mode))
    return(filename);
  return(CNIL);
}

value arc_rmfile(arc *c, value filename)
{
  char *utf_filename;
  int en;

  TYPECHECK(filename, T_STRING);
  utf_filename = alloca(FIX2INT(arc_strutflen(c, filename)) + 1);
  arc_str2cstr(c, filename, utf_filename);
  if (unlink(utf_filename) < 0) {
    en = errno;
    arc_err_cstrfmt(c, "rmfile: cannot delete file \"%s\", (%s; errno=%d)", utf_filename, strerror(en), en);
    return(CNIL);
  }
  return(CNIL);
}

value arc_mvfile(arc *c, value oldname, value newname)
{
  char *utf_oldname, *utf_newname;
  int en;

  TYPECHECK(oldname, T_STRING);
  TYPECHECK(newname, T_STRING);
  utf_oldname = alloca(FIX2INT(arc_strutflen(c, oldname)) + 1);
  arc_str2cstr(c, oldname, utf_oldname);
  utf_newname = alloca(FIX2INT(arc_strutflen(c, newname)) + 1);
  arc_str2cstr(c, newname, utf_newname);
  if (rename(utf_oldname, utf_newname) != 0) {
    en = errno;
    arc_err_cstrfmt(c, "mvfile: cannot move file \"%s\" to \"%s\", (%s; errno=%d)", utf_oldname, utf_newname, strerror(en), en);
    return(CNIL);
  }
  return(CNIL);
}
