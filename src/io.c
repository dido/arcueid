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
#include "arcueid.h"
#include "alloc.h"
#include "io.h"

/* Delegate to type-specific I/O free function if any */
static void iofree(arc *c, value v)
{
  struct io_t *io = (struct io_t *)v;

  if (io->iot->free != NULL)
    io->iot->free(c, v);
}

/* Delegate to type-specific I/O marker if any, after marking ioops vector */
static void iomark(arc *c, value v,
		   void (*marker)(struct arc *, value, int),
		   int depth)
{
  struct io_t *io = (struct io_t *)v;

  marker(c, io->io_ops, depth);
  if (io->iot->mark != NULL)
    io->iot->mark(c, v, marker, depth);
}

static enum arc_trstate ioapply(arc *c, value t, value v)
{
  struct io_t *io = (struct io_t *)v;

  /* Delegate to a type-specific apply if any */
  if (io->iot->apply != NULL)
    return(io->iot->apply(c, t, v));
  arc_err_cstr(c, CNIL, "invalid argument for apply");
  return(TR_RC);
}


arctype __arc_io_t = { iofree, iomark, NULL, NULL, NULL, NULL, ioapply };

value __arc_allocio(arc *c, size_t xdsize, arctype *t, value ioops)
{
  struct io_t *io;

  io = (struct io_t *)arc_new(c, &__arc_io_t, IOALIGNSIZE + ALIGN_SIZE(xdsize));
  io->ungetrune = -1;
  io->io_ops = ioops;
  io->iot = t;
  return((value)io);
}

void *__arc_iodata(value io)
{
  return((void *)IODATA(io));
}
