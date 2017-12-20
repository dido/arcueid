# Copyright (C) 2017 Rafael R. Sevilla
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
# License along with this library; if not, see <http://www.gnu.org/licenses/>.
#
TESTS = check_alloc
check_PROGRAMS = check_alloc

check_alloc_SOURCES = check_alloc.c $(top_builddir)/src/arcueid.h $(top_builddir)/src/alloc.h
check_alloc_CFLAGS = @CHECK_CFLAGS@
check_alloc_LDADD = @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@
