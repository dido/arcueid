/* 
  Copyright (C) 2012 Rafael R. Sevilla

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
#include <stdlib.h>
#include <unistd.h>
#include <popt.h>
#include <errno.h>
#include <string.h>
#include <float.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <sys/utsname.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "arcueid.h"
#include "io.h"
#include "symbols.h"
#include "builtin.h"
#include "alloc.h"
#include "../config.h"

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

static value sock_pp(arc *c, value v)
{
  return(arc_mkstringc(c, "#<socket>"));
}

static void sock_marker(arc *c, value v, int level,
			void (*markfn)(arc *, value, int))
{
  /* does nothing */
}

static void sock_sweeper(arc *c, value v)
{
  if (!PORT(v)->closed) {
    close(PORT(v)->u.sock.sockfd);
    /* XXX: error handling here? */
    if (PORT(v)->u.sock.addr != NULL) {
      free(PORT(v)->u.sock.addr);
      PORT(v)->u.sock.addr = NULL;
    }
  }
  /* release memory */
  c->free_block(c, (void *)v);
}

static int sock_getb(arc *c, struct arc_port *p)
{
  char ch;
  int rb;

  rb = read(p->u.sock.sockfd, (void *)&ch, sizeof(ch));
  if (rb == 0)
    return(EOF);
  if (rb < 0) {
    int en = errno;

    arc_err_cstrfmt(c, "error reading socket (%s; errno=%d)", strerror(en), en);
    return(CNIL);
  }
  return((int)ch);
}

static int sock_putb(arc *c, struct arc_port *p, int byte)
{
  unsigned char ch;
  int wb;

  ch = (char)byte&0xff;
  wb = write(p->u.sock.sockfd, (void *)&ch, sizeof(ch));
  if (wb < 0) {
    int en = errno;

    arc_err_cstrfmt(c, "error writing socket (%s; errno=%d)", strerror(en), en);
    return(CNIL);
  }
  return(byte);
}

static int sock_seek(arc *c, struct arc_port *p, int64_t offset, int whence)
{
  arc_err_cstrfmt(c, "cannot seek a socket");
  return(-1);
}

static int64_t sock_tell(arc *c, struct arc_port *p)
{
  arc_err_cstrfmt(c, "cannot tell a socket");
  return(-1);
}

static int sock_close(arc *c, struct arc_port *p)
{
  int ret = 0;

  if (!p->closed) {
    ret = close(p->u.sock.sockfd);
    p->closed = 1;
  }
  return(ret);
}

static int sock_ready(arc *c, struct arc_port *p)
{
  fd_set rfds;
  struct timeval tv;
  int retval;

  FD_ZERO(&rfds);
  FD_SET(p->u.sock.sockfd, &rfds);
  tv.tv_usec = tv.tv_sec = 0;
  retval = select(p->u.sock.sockfd+1, &rfds, NULL, NULL, &tv);

  if (retval == -1) {
    int en = errno;

    arc_err_cstrfmt(c, "error selecting for socket (%s; errno=%d)", strerror(en), en);
    return(0);
  }

  if (retval == 0)
    return(0);
  if (FD_ISSET(p->u.sock.sockfd, &rfds))
    return(1);
  return(0);
}

static int sock_fd(arc *c, struct arc_port *p)
{
  return(p->u.sock.sockfd);
}

static value mksocket(arc *c, int sock, int ai_family, int socktype)
{
  void *cellptr;
  value fd;

  cellptr = c->get_block(c, sizeof(struct cell) + sizeof(struct arc_port));
  if (cellptr == NULL)
    arc_err_cstrfmt(c, "socket: cannot allocate memory");
  fd = (value)cellptr;
  BTYPE(fd) = T_PORT;
  REP(fd)._custom.pprint = sock_pp;
  REP(fd)._custom.marker = sock_marker;
  REP(fd)._custom.sweeper = sock_sweeper;
  PORT(fd)->type = FT_SOCKET;
  PORT(fd)->u.sock.sockfd = sock;
  PORT(fd)->u.sock.addr = NULL;
  PORT(fd)->u.sock.ai_family = ai_family;
  PORT(fd)->u.sock.socktype = socktype;
  PORT(fd)->name = CNIL;
  PORT(fd)->getb = sock_getb;
  PORT(fd)->putb = sock_putb;
  PORT(fd)->seek = sock_seek;
  PORT(fd)->tell = sock_tell;
  PORT(fd)->close = sock_close;
  PORT(fd)->ready = sock_ready;
  PORT(fd)->fd = sock_fd;
  PORT(fd)->closed = 0;
  PORT(fd)->ungetrune = -1;	/* no rune available */
  return(fd);
}

value arc_socket(arc *c, value d, value t, value p)
{
  int domain, stype, proto, sock;

  if (TYPE(d) == T_SYMBOL) {
    if (d == ARC_BUILTIN(c, S_AF_UNIX))
      domain = AF_UNIX;
    else if (d == ARC_BUILTIN(c, S_AF_INET))
      domain = AF_INET;
    else if (d == ARC_BUILTIN(c, S_AF_INET6))
      domain = AF_INET6;
    else {
      arc_err_cstrfmt(c, "socket: unknown socket domain");
      return(CNIL);
    }
  } else if (TYPE(d) == T_FIXNUM) {
    domain = FIX2INT(d);
  } else {
    arc_err_cstrfmt(c, "expected argument 1 to be type %s or type %s given type %s", TYPENAME(T_SYMBOL), TYPENAME(T_FIXNUM), TYPENAME(TYPE(d)));
    return(CNIL);
  }

  if (TYPE(t) == T_SYMBOL) {
    if (t == ARC_BUILTIN(c, S_SOCK_STREAM))
      stype = SOCK_STREAM;
    else if (t == ARC_BUILTIN(c, S_SOCK_DGRAM))
      stype = SOCK_DGRAM;
    else if (t == ARC_BUILTIN(c, S_SOCK_RAW))
      stype = SOCK_RAW;
    else {
      arc_err_cstrfmt(c, "socket: unknown socket type");
      return(CNIL);
    }
  } else if (TYPE(t) == T_FIXNUM) {
    stype = FIX2INT(t);
  } else {
arc_err_cstrfmt(c, "expected argument 2 to be type %s or type %s given type %s", TYPENAME(T_SYMBOL), TYPENAME(T_FIXNUM), TYPENAME(TYPE(t)));
    return(CNIL);
  }

  TYPECHECK(p, T_FIXNUM, 3);
  proto = FIX2INT(p);
  sock = socket(domain, stype, proto);
  if (sock < 0) {
    int en = errno;

    arc_err_cstrfmt(c, "error opening socket (%s; errno=%d)", strerror(en), en);
    return(CNIL);
  }
  return(mksocket(c, sock, domain, stype));
}

value arc_socket_bind(arc *c, value sockfd, value family, value socktype,
		      value flags, value node, value service)
{
  struct addrinfo hints, *res, *rp;
  char *nnode, *svc;
  int err, fd;

  memset(&hints, 0, sizeof(hints));

  TYPECHECK(sockfd, T_PORT, 1);
  if (PORT(sockfd)->type != FT_SOCKET) {
    arc_err_cstrfmt(c, "expected argument 1 to be a socket");
    return(CNIL);
  }
  fd = PORT(sockfd)->u.sock.sockfd;
  if (TYPE(family) == T_SYMBOL) {
    if (family == ARC_BUILTIN(c, S_AF_UNIX))
      hints.ai_family = AF_UNIX;
    else if (family == ARC_BUILTIN(c, S_AF_INET))
      hints.ai_family = AF_INET;
    else if (family == ARC_BUILTIN(c, S_AF_INET6))
      hints.ai_family = AF_INET6;
    else {
      arc_err_cstrfmt(c, "bind: unknown bind family");
      return(CNIL);
    }
  } else if (TYPE(family) == T_FIXNUM) {
    hints.ai_family = FIX2INT(family);
  } else if (NIL_P(family)) {
    hints.ai_family = AF_UNSPEC;
  } else {
    arc_err_cstrfmt(c, "expected argument 2 to be type %s or type %s given type %s", TYPENAME(T_SYMBOL), TYPENAME(T_FIXNUM), TYPENAME(TYPE(family)));
    return(CNIL);
  }

  if (TYPE(socktype) == T_SYMBOL) {
    if (socktype == ARC_BUILTIN(c, S_SOCK_STREAM))
      hints.ai_socktype = SOCK_STREAM;
    else if (socktype == ARC_BUILTIN(c, S_SOCK_DGRAM))
      hints.ai_socktype = SOCK_DGRAM;
    else if (socktype == ARC_BUILTIN(c, S_SOCK_RAW))
      hints.ai_socktype = SOCK_RAW;
    else {
      arc_err_cstrfmt(c, "bind: unknown socket type");
      return(CNIL);
    }
  } else if (TYPE(socktype) == T_FIXNUM) {
    hints.ai_socktype = FIX2INT(socktype);
  } else {
arc_err_cstrfmt(c, "expected argument 3 to be type %s or type %s given type %s", TYPENAME(T_SYMBOL), TYPENAME(T_FIXNUM), TYPENAME(TYPE(socktype)));
    return(CNIL);
  }

  if (NIL_P(flags)) {
    hints.ai_flags = AI_PASSIVE;
  } else {
    TYPECHECK(flags, T_FIXNUM, 4);
    hints.ai_flags = FIX2INT(flags);
  }

  if (NIL_P(node)) {
    nnode = NULL;
  } else {
    TYPECHECK(node, T_STRING, 5);
    nnode = alloca(sizeof(char)*(FIX2INT(arc_strutflen(c, node)) + 1));
    arc_str2cstr(c, node, nnode);
  }

  if (NIL_P(service)) {
    svc = NULL;
  } else {
    TYPECHECK(service, T_STRING, 6);
    /* XXX conversions, etc */
    svc = alloca(sizeof(char)*(FIX2INT(arc_strutflen(c, service)) + 1));
    arc_str2cstr(c, service, svc);
  }
  if ((err = getaddrinfo(nnode, svc, &hints, &res)) != 0) {
    arc_err_cstrfmt(c, "getaddrinfo error (%s; code=%d)", gai_strerror(err), err);
    return(CNIL);
  }

  for (rp = res; rp != NULL; rp = rp->ai_next) {
    if (bind(fd, rp->ai_addr, rp->ai_addrlen) == 0)
      break;
  }

  if (rp == NULL) {
    int en = errno;

    freeaddrinfo(res);
    arc_err_cstrfmt(c, "bind error (%s; errno=%d)", strerror(en), en);
    return(CNIL);
  }
  freeaddrinfo(res);
  return(CTRUE);
}

value arc_socket_listen(arc *c, value sockfd, value backlog)
{
  TYPECHECK(sockfd, T_PORT, 1);
  if (PORT(sockfd)->type != FT_SOCKET) {
    arc_err_cstrfmt(c, "expected argument 1 to be a socket");
    return(CNIL);
  }
  TYPECHECK(backlog, T_FIXNUM, 2);
  if (listen(PORT(sockfd)->u.sock.sockfd, FIX2INT(backlog)) < 0) {
    int en = errno;
    arc_err_cstrfmt(c, "error listening socket (%s; errno=%d)", strerror(en), en);
    return(CNIL);
  }
  return(CTRUE);
}

extern value coerce_string(arc *c, value obj, value argv);

value arc_open_socket(arc *c, value port)
{
  value portstr;
  value sock;

  TYPECHECK(port, T_FIXNUM, 1);
  portstr = coerce_string(c, port, CNIL);
  sock = arc_socket(c, ARC_BUILTIN(c, S_AF_INET),
		    ARC_BUILTIN(c, S_SOCK_STREAM), INT2FIX(0));
  arc_socket_bind(c, sock, ARC_BUILTIN(c, S_AF_INET),
		  ARC_BUILTIN(c, S_SOCK_STREAM), CNIL, CNIL, portstr);
  arc_socket_listen(c, sock, INT2FIX(10));
  return(sock);
}

value arc_socket_accept(arc *c, value sock)
{
  struct sockaddr_storage *their_addr;
  socklen_t addr_size;
  int newfd;
  value asock;
  char ipstr[INET6_ADDRSTRLEN];

  READ_CHECK(c, sock);
  their_addr = (struct sockaddr_storage *)malloc(sizeof(struct sockaddr_storage));
  addr_size = sizeof(struct sockaddr_storage);
  newfd = accept(PORT(sock)->u.sock.sockfd, (struct sockaddr *)their_addr, &addr_size);
  if (newfd < 0) {
    int en = errno;

    arc_err_cstrfmt(c, "error accepting socket (%s; errno=%d)", strerror(en), en);
    return(CNIL);
  }
  asock = mksocket(c, newfd, PORT(sock)->u.sock.ai_family,
		   PORT(sock)->u.sock.socktype);
  PORT(asock)->u.sock.addr = their_addr;
  inet_ntop(PORT(sock)->u.sock.ai_family, (void *)their_addr, ipstr, sizeof(ipstr));

  return(cons(c, asock, cons(c, asock, cons(c, arc_mkstringc(c, ipstr), CNIL))));
}
