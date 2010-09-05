/* 
  Copyright (C) 2010 Rafael R. Sevilla

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
#include <stdio.h>
#include <alloca.h>
#include <errno.h>
#include <string.h>
#include "arcueid.h"

enum file_types {
  FT_FILE,			/* normal file */
  FT_STRING,			/* string file */
  FT_SOCKET			/* socket */
};

struct arc_port {
  int type;			/* type of file */
  union {
    struct {
      value str;
      int idx;
    } strfile;
    struct {
      value name;
      FILE *fp;
      int open;
    } file;
    int sock;
  } u;
};

static value file_pp(arc *c, value v)
{
  value nstr = arc_mkstringc(c, "#<port:");

  nstr = arc_strcat(c, nstr, ((struct arc_port *)REP(v)._custom.data)->u.file.name);
  nstr = arc_strcat(c, nstr, arc_mkstringc(c, ">"));
  return(nstr);
}

static void file_marker(arc *c, value v, int level,
			void (*markfn)(arc *, value, int))
{
  /* does nothing */
}

static void file_sweeper(arc *c, value v)
{
  struct arc_port *p;

  p = (struct arc_port *)(REP(v)._custom.data);

  if (p->u.file.open) {
    fclose(p->u.file.fp);
    /* XXX: error handling here? */
  }
  /* release memory */
  c->free_block(c, (void *)v);
}

value arc_infile(arc *c, value filename)
{
  void *cellptr;
  value fd;
  struct arc_port *p;
  char *utf_filename;
  FILE *fp;
  int en;

  cellptr = c->get_block(c, sizeof(struct cell) + sizeof(struct arc_port));
  if (cellptr == NULL)
    return(CNIL);
  fd = (value)cellptr;
  BTYPE(fd) = T_PORT;
  REP(fd)._custom.pprint = file_pp;
  REP(fd)._custom.marker = file_marker;
  REP(fd)._custom.sweeper = file_sweeper;
  p = (struct arc_port *)(REP(fd)._custom.data);
  /* XXX make this file name a fully qualified pathname */
  p->u.file.name = filename;
  utf_filename = alloca(FIX2INT(arc_strutflen(c, filename)) + 1);
  arc_str2cstr(c, filename, utf_filename);
  fp = fopen(utf_filename, "r");
  if (fp == NULL) {
    en = errno;
    c->signal_error(c, "open-input-file: cannot open input file \"%s\", (%s; errno=%d)", utf_filename, strerror(en), en);
  }
  p->u.file.fp = fp;
  p->u.file.open = 1;
  return(fd);
}
