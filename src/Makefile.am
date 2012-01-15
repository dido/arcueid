# Copyright (C) 2011 Rafael R. Sevilla
#
# This file is part of Arcueid
#
# Arcueid is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation; either version 3 of the
# License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not,  see <http://www.gnu.org/licenses/>.
#
lib_LTLIBRARIES = libarcueid.la

libarcueid_la_LDFLAGS = -version-info 0:0:0
libarcueid_la_SOURCES = arcueid.c ciel.c arith.c alloc.c \
	codegen.c hash.c io.c osdep.c pprint.c reader.c string.c \
	typehandling.c utf.c vmengine.c compiler.c thread.c

include_HEADERS = arcueid.h
noinst_HEADERS = arith.h alloc.h utf.h symbols.h vmengine.h
bin_PROGRAMS = arcueid
arcueid_SOURCES = repl.c
arcueid_CFLAGS = -DPKGDATA=\"$(pkgdatadir)\"
arcueid_LDADD = $(LIBOBJS) @LIBARCUEID_LIBS@
