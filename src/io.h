/* 
  Copyright (C) 2011 Rafael R. Sevilla

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
#ifndef _IO_H_

enum file_types {
  FT_FILE,			/* normal file */
  FT_STRING,			/* string file */
  FT_SOCKET			/* socket */
};

struct arc_port {
  int type;			/* type of file */
  value name;			/* name of file */
  union {
    struct {
      value str;
      int idx;
    } strfile;
    struct {
      FILE *fp;
      int open;
    } file;
    int sock;
  } u;
  int closed;
  int (*ready)(arc *, struct arc_port *);
  int (*fd)(arc *, struct arc_port *);
  int (*getb)(arc *, struct arc_port *);
  int (*putb)(arc *, struct arc_port *, int);
  int (*seek)(arc *, struct arc_port *, int64_t, int);
  int64_t (*tell)(arc *, struct arc_port *);
  int (*close)(arc *, struct arc_port *);
  Rune ungetrune;		/* unget rune */
};

#define PORT(v) ((struct arc_port *)REP(v)._custom.data)
#define PORTF(v) (PORT(v)->u.file)
#define PORTS(v) (PORT(v)->u.strfile)

extern void arc_thread_wait_fd(arc *c, int fd);

/* This function should be used by basic read I/O functions and must
   be called before any side effects occur. */
#define READ_CHECK(c, port) do {				\
  if (!PORT(port)->ready(c, PORT(port))) {			\
    arc_thread_wait_fd(c, PORT(port)->fd(c, PORT(port)));	\
  } while (0)

#define _IO_H_

#endif
