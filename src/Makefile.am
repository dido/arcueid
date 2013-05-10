# Copyright (C) 2013 Rafael R. Sevilla
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
libarcueid_la_SOURCES = alloc.c arith.c arcueid.c ccode.c chan.c \
	clos.c codegen.c compiler.c cons.c cont.c dirops.c env.c \
	err.c fileio.c gopt.c hash.c io.c load.c mathfns.c net.c \
	osdep.c re.c regaux.c regcomp.c rregexec.c sio.c sread.c \
	ssyntax.c string.c symbol.c thread.c util.c utf.c vector.c \
	vmengine.c

include_HEADERS = arcueid.h
noinst_HEADERS = alloc.h arith.h builtins.h compiler.h gopt.h \
	hash.h io.h osdep.h regexp.h regcomp.h utf.h vmengine.h

bin_PROGRAMS = arcueid
arcueid_SOURCES = repl.c
arcueid_CFLAGS = -DPKGDATA=\"$(pkgdatadir)\"
arcueid_LDADD = $(LIBOBJS) @LIBARCUEID_LIBS@ @RLLIBS@
