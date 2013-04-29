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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/utsname.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "arcueid.h"
#include "io.h"
#include "builtins.h"
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

struct sock_t {
  int closed;
  int fd;
  int ai_family;
  int socktype;
  void *addr;
};

static typefn_t sock_tfn;

#define SOCKDATA(sock) (IODATA(sock, struct sock_t *))

static void sock_marker(arc *c, value v, int depth,
		       void (*markfn)(arc *, value, int))
{
  /* does nothing */
}

static void sock_sweeper(arc *c, value v)
{
  if (!SOCKDATA(v)->closed) {
    close(SOCKDATA(v)->fd);
  }
  /* XXX: error handling here? */
  if (SOCKDATA(v)->addr != NULL) {
    free(SOCKDATA(v)->addr);
    SOCKDATA(v)->addr = NULL;
  }
}


static unsigned long sock_hash(arc *c, value v, arc_hs *s)
{
  int len;

  len = arc_hash_increment(c, INT2FIX(SOCKDATA(v)->fd), s);
  return(len);
}

static AFFDEF(sock_closed_p)
{
  AARG(sock);
  AFBEGIN;
  ARETURN((SOCKDATA(AV(sock))->closed) ? CTRUE : CNIL);
  AFEND;
}
AFFEND

static AFFDEF(sock_ready)
{
  AARG(sock);
  fd_set rfds;
  int retval;
  struct timeval tv;
  AFBEGIN;
  for (;;) {
    FD_ZERO(&rfds);
    FD_SET(SOCKDATA(AV(sock))->fd, &rfds);
    tv.tv_usec = tv.tv_sec = 0;
    retval = select(SOCKDATA(AV(sock))->fd+1, &rfds, NULL, NULL, &tv);
    if (retval == -1) {
      int en = errno;

      arc_err_cstrfmt(c, "error checking socket for reading (%s; errno=%d)",
		      strerror(en), en);
      ARETURN(CNIL);
    }

    if (FD_ISSET(SOCKDATA(AV(sock))->fd, &rfds))
      ARETURN(CTRUE);

    /* We have to wait */
    AIOWAITR(SOCKDATA(AV(sock))->fd);
  }
  AFEND;
}
AFFEND

static AFFDEF(sock_wready)
{
  AARG(sock);
  fd_set wfds;
  int retval;
  struct timeval tv;
  AFBEGIN;
  for (;;) {
    FD_ZERO(&wfds);
    FD_SET(SOCKDATA(AV(sock))->fd, &wfds);
    tv.tv_usec = tv.tv_sec = 0;
    retval = select(SOCKDATA(AV(sock))->fd+1, NULL, &wfds, NULL, &tv);
    if (retval == -1) {
      int en = errno;

      arc_err_cstrfmt(c, "error checking socket for writing (%s; errno=%d)",
		      strerror(en), en);
      ARETURN(CNIL);
    }

    if (FD_ISSET(SOCKDATA(AV(sock))->fd, &wfds))
      ARETURN(CTRUE);

    /* We have to wait */
    AIOWAITW(SOCKDATA(AV(sock))->fd);
  }
  AFEND;
}
AFFEND

static AFFDEF(sock_getb)
{
  AARG(sock);
  char ch;
  int rb;
  AFBEGIN;

  rb = recv(SOCKDATA(AV(sock))->fd, (void *)&ch, sizeof(ch), 0);
  if (rb == 0)
    ARETURN(CNIL);
  if (rb < 0) {
    int en = errno;

    arc_err_cstrfmt(c, "error reading socket (%s; errno=%d)", strerror(en), en);
    ARETURN(CNIL);
  }

  ARETURN(INT2FIX(ch));
  AFEND;
}
AFFEND

static AFFDEF(sock_putb)
{
  AARG(sock, byte);
  unsigned char ch;
  int wb;
  AFBEGIN;
  ch = (char)(FIX2INT(AV(byte)) & 0xff);
  wb = send(SOCKDATA(AV(sock))->fd, (void *)&ch, sizeof(ch), 0);
  if (wb < 0) {
    int en = errno;

    arc_err_cstrfmt(c, "error writing socket (%s; errno=%d)", strerror(en), en);
    return(CNIL);
  }
  ARETURN(INT2FIX(ch));
  AFEND;
}
AFFEND

static AFFDEF(sock_seek)
{
  AARG(sock, offset, whence);
  AFBEGIN;
  (void)sock;
  (void)offset;
  (void)whence;
  arc_err_cstrfmt(c, "cannot seek a socket");
  ARETURN(INT2FIX(-1));
  AFEND;
}
AFFEND

static AFFDEF(sock_tell)
{
  AARG(sock);
  AFBEGIN;
  (void)sock;
  arc_err_cstrfmt(c, "cannot tell a socket");
  ARETURN(INT2FIX(-1));
  AFEND;
}
AFFEND

static AFFDEF(sock_close)
{
  AARG(sock);
  AFBEGIN;
  if (SOCKDATA(AV(sock))->closed == 0) {
    close(SOCKDATA(AV(sock))->fd);
    SOCKDATA(AV(sock))->closed = 1;
  }
  ARETURN(CNIL);
  AFEND;
}
AFFEND

static value mksocket(arc *c, int type, int sockfd, int ai_family, int socktype)
{
  value sock;

  sock = __arc_allocio(c, type, &sock_tfn, sizeof(struct sock_t));
  IO(sock)->flags = 0;
  IO(sock)->io_ops = VINDEX(VINDEX(c->builtins, BI_io), BI_io_sock);
  IO(sock)->name = CNIL;
  SOCKDATA(sock)->closed = 0;
  SOCKDATA(sock)->fd = sockfd;
  SOCKDATA(sock)->addr = NULL;
  SOCKDATA(sock)->ai_family = ai_family;
  SOCKDATA(sock)->socktype = socktype;
  return(sock);
}

void __arc_init_sockio(arc *c)
{
  value io_ops;

  io_ops = arc_mkvector(c, IO_last+1);
  SVINDEX(io_ops, IO_closed_p, arc_mkaff(c, sock_closed_p, CNIL));
  SVINDEX(io_ops, IO_ready, arc_mkaff(c, sock_ready, CNIL));
  SVINDEX(io_ops, IO_wready, arc_mkaff(c, sock_wready, CNIL));
  SVINDEX(io_ops, IO_getb, arc_mkaff(c, sock_getb, CNIL));
  SVINDEX(io_ops, IO_putb, arc_mkaff(c, sock_putb, CNIL));
  SVINDEX(io_ops, IO_seek, arc_mkaff(c, sock_seek, CNIL));
  SVINDEX(io_ops, IO_tell, arc_mkaff(c, sock_tell, CNIL));
  SVINDEX(io_ops, IO_close, arc_mkaff(c, sock_close, CNIL));
  SVINDEX(VINDEX(c->builtins, BI_io), BI_io_sock, io_ops);
}

AFFDEF(arc_open_socket)
{
  AARG(port);
  int sockfd, rv;
  struct addrinfo hints, *servinfo, *p;
  int yes=1;
  char portstr[6];
  int family, socktype;
  AFBEGIN;

  TYPECHECK(AV(port), T_FIXNUM);
  if (FIX2INT(AV(port)) < 1 || FIX2INT(AV(port)) > 65535) {
    arc_err_cstrfmt(c, "open-socket: port number %d out of range", FIX2INT(AV(port)));
    ARETURN(CNIL);
  }
  snprintf(portstr, 6, "%d", (int)FIX2INT(AV(port)));

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  if ((rv = getaddrinfo(NULL, portstr, &hints, &servinfo)) != 0) {
    arc_err_cstrfmt(c, "open-socket: getaddrinfo error (%s; code=%d)", gai_strerror(rv), rv);
    ARETURN(CNIL);
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
      ARETURN(CNIL);
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
    ARETURN(CNIL);
  }

  freeaddrinfo(servinfo);

  if (listen(sockfd, 10) < 0) {
    int en = errno;
    arc_err_cstrfmt(c, "open-socket: error listening (%s; errno=%d)", strerror(en), en);
    ARETURN(CNIL);
  }
  ARETURN(mksocket(c, T_INPORT, sockfd, family, socktype));
  AFEND;
}
AFFEND

AFFDEF(arc_socket_accept)
{
  AARG(sock);
  struct sockaddr_storage their_addr;
  socklen_t addr_size;
  int newfd;
  value rsock, wsock;
  char ipstr[INET6_ADDRSTRLEN];
  void *addr, *addr2;
  AFBEGIN;

  TYPECHECK(AV(sock), T_INPORT);
  /* XXX - find a way to verify that sock really is a socket */

  /* Check if it's open for reading.  If not it will block the thread */
  AFCALL(VINDEX(IO(AV(sock))->io_ops, IO_ready), AV(sock));
  if (AFCRV == CNIL) {
    arc_err_cstrfmt(c, "socket is not ready for reading");
    ARETURN(CNIL);
  }

  addr_size = sizeof(struct sockaddr_storage);
  newfd = accept(SOCKDATA(AV(sock))->fd, (struct sockaddr *)&their_addr, &addr_size);
  if (newfd < 0) {
    int en = errno;

    arc_err_cstrfmt(c, "error accepting socket (%s; errno=%d)", strerror(en), en);
    ARETURN(CNIL);
  }
  rsock = mksocket(c, T_INPORT, newfd, SOCKDATA(AV(sock))->ai_family,
		   SOCKDATA(AV(sock))->socktype);
  wsock = mksocket(c, T_OUTPORT, newfd, SOCKDATA(AV(sock))->ai_family,
		   SOCKDATA(AV(sock))->socktype);
  addr = malloc(addr_size);
  addr2 = malloc(addr_size);
  if (SOCKDATA(AV(sock))->ai_family == AF_INET) {
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)&their_addr;
    memcpy(addr, &(ipv4->sin_addr), addr_size);
  } else {
    struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)&their_addr;
    memcpy(addr, &(ipv6->sin6_addr), addr_size);
  }
  memcpy(addr2, addr, addr_size);
  SOCKDATA(rsock)->addr = addr;
  SOCKDATA(wsock)->addr = addr2;
  inet_ntop(SOCKDATA(AV(sock))->ai_family, addr, ipstr, sizeof(ipstr));
  ARETURN(cons(c, rsock, cons(c, wsock,
			      cons(c, arc_mkstringc(c, ipstr), CNIL))));
  AFEND;
}
AFFEND

value arc_client_ip(arc *c, value sock)
{
  char ipstr[INET6_ADDRSTRLEN];
  void *addr;

  /* XXX - typechecks */
  if (SOCKDATA(sock)->addr == NULL) {
    arc_err_cstrfmt(c, "client-ip: no peer associated with socket");
    return(CNIL);
  }
  if (SOCKDATA(sock)->ai_family == AF_INET) {
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)SOCKDATA(sock)->addr;
    addr = &(ipv4->sin_addr);
  } else {
    struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)SOCKDATA(sock)->addr;
    addr = &(ipv6->sin6_addr);
  }
  inet_ntop(SOCKDATA(sock)->ai_family, addr, ipstr, sizeof(ipstr));
  return(arc_mkstringc(c, ipstr));
}

static typefn_t sock_tfn = {
  sock_marker,
  sock_sweeper,
  NULL,
  sock_hash,
  NULL,
  NULL,
  NULL,
  NULL
};
