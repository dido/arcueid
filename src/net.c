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
			void (*markfn)(arc *, value, int, value))
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
}

static unsigned long sock_hash(arc *c, arc_hs *s, value v)
{
  unsigned long len;

  len = arc_hash_increment(c, INT2FIX(FT_SOCKET), s);
  len += arc_hash_increment(c, INT2FIX(PORT(v)->u.sock.sockfd), s);
  return(len);
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

static int sock_wready(arc *c, struct arc_port *p)
{
  fd_set wfds;
  struct timeval tv;
  int retval;

  FD_ZERO(&wfds);
  FD_SET(p->u.sock.sockfd, &wfds);
  tv.tv_usec = tv.tv_sec = 0;
  retval = select(p->u.sock.sockfd+1, NULL, &wfds, NULL, &tv);

  if (retval == -1) {
    int en = errno;

    arc_err_cstrfmt(c, "error selecting for socket (%s; errno=%d)", strerror(en), en);
    return(0);
  }

  if (retval == 0)
    return(0);
  if (FD_ISSET(p->u.sock.sockfd, &wfds))
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
  REP(fd)._custom.hash = sock_hash;
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
  PORT(fd)->wready = sock_wready;
  PORT(fd)->fd = sock_fd;
  PORT(fd)->closed = 0;
  PORT(fd)->ungetrune = -1;	/* no rune available */
  return(fd);
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

value arc_open_socket(arc *c, value port)
{
  int sockfd, rv;
  struct addrinfo hints, *servinfo, *p;
  int yes=1;
  char portstr[6];
  int family, socktype;

  TYPECHECK(port, T_FIXNUM, 1);
  if (FIX2INT(port) < 1 || FIX2INT(port) > 65535) {
    arc_err_cstrfmt(c, "open-socket: port number %d out of range", FIX2INT(port));
    return(CNIL);
  }
  snprintf(portstr, 6, "%d", (int)FIX2INT(port));

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  if ((rv = getaddrinfo(NULL, portstr, &hints, &servinfo)) != 0) {
    arc_err_cstrfmt(c, "open-socket: getaddrinfo error (%s; code=%d)", gai_strerror(rv), rv);
    return(CNIL);
  }

  for (p = servinfo; p; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
			 p->ai_protocol)) == -1)
      continue;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
      int en = errno;
      arc_err_cstrfmt(c, "open-socket: setsockopt error (%s; code=%d)", strerror(en), en);
      freeaddrinfo(servinfo);
      return(CNIL);
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      continue;
    }
    family = p->ai_family;
    socktype = p->ai_socktype;
    break;
  }
  if (p == NULL) {
    int en = errno;
    arc_err_cstrfmt(c, "open-socket: failed to bind (last error %s; code=%d)", strerror(en), en);
    freeaddrinfo(servinfo);
    return(CNIL);
  }

  if (listen(sockfd, 10) < 0) {
    int en = errno;
    arc_err_cstrfmt(c, "open-socket: error listening (%s; errno=%d)", strerror(en), en);
    return(CNIL);
  }
  return(mksocket(c, sockfd, family, socktype));
}

value arc_socket_accept(arc *c, value sock)
{
  struct sockaddr_storage their_addr;
  socklen_t addr_size;
  int newfd;
  value asock;
  char ipstr[INET6_ADDRSTRLEN];
  void *addr;

  TYPECHECK(sock, T_PORT, 1);
  if (PORT(sock)->type != FT_SOCKET) {
    arc_err_cstrfmt(c, "expected argument 1 to be a socket");
    return(CNIL);
  }
  READ_CHECK(c, sock);

  addr_size = sizeof(struct sockaddr_storage);
  newfd = accept(PORT(sock)->u.sock.sockfd, (struct sockaddr *)&their_addr, &addr_size);
  if (newfd < 0) {
    int en = errno;

    arc_err_cstrfmt(c, "error accepting socket (%s; errno=%d)", strerror(en), en);
    return(CNIL);
  }
  asock = mksocket(c, newfd, PORT(sock)->u.sock.ai_family,
		   PORT(sock)->u.sock.socktype);
  addr = malloc(addr_size);
  if (PORT(sock)->u.sock.ai_family == AF_INET) {
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)&their_addr;
    memcpy(addr, &(ipv4->sin_addr), addr_size);
  } else {
    struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)&their_addr;
    memcpy(addr, &(ipv6->sin6_addr), addr_size);
  }
  PORT(asock)->u.sock.addr = addr;
  inet_ntop(PORT(sock)->u.sock.ai_family, addr, ipstr, sizeof(ipstr));
  return(cons(c, asock, cons(c, asock, cons(c, arc_mkstringc(c, ipstr), CNIL))));
}

value arc_client_ip(arc *c, value sock)
{
  char ipstr[INET6_ADDRSTRLEN];
  void *addr;

  TYPECHECK(sock, T_PORT, 1);
  if (PORT(sock)->type != FT_SOCKET) {
    arc_err_cstrfmt(c, "expected argument 1 to be a socket");
    return(CNIL);
  }
  if (PORT(sock)->u.sock.addr == NULL) {
    arc_err_cstrfmt(c, "client-ip: no peer associated with socket");
    return(CNIL);
  }
  if (PORT(sock)->u.sock.ai_family == AF_INET) {
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)PORT(sock)->u.sock.addr;
    addr = &(ipv4->sin_addr);
  } else {
    struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)PORT(sock)->u.sock.addr;
    addr = &(ipv6->sin6_addr);
  }
  inet_ntop(PORT(sock)->u.sock.ai_family, addr, ipstr, sizeof(ipstr));
  return(arc_mkstringc(c, ipstr));
}
