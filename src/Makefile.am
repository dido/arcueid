# Copyright (C) 2009 Rafael R. Sevilla
#
# This file is part of CArc
#
# CArc is free software; you can redistribute it and/or modify it
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
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA
# 02110-1301 USA.
#
lib_LTLIBRARIES = libcarc.la

libcarc_la_LDFLAGS = -version-info 0:0:0
libcarc_la_SOURCES = arith.c alloc.c carc.c compiler.c hash.c osdep.c \
	reader.c string.c utf.c vmengine.c

include_HEADERS = carc.h
noinst_HEADERS = arith.h alloc.h utf.h vmengine.h

VMGEN=/usr/bin/vmgen

carcvm-vm.i carcvm-disasm.i carcvm-gen.i carcvm-labels.i carcvm-profile.i carcvm-peephole.i: carcvm.vmg
	$(VMGEN) carcvm.vmg

