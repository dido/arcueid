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
# License along with this library; if not, see <http://www.gnu.org/licenses/>.
#
TESTS = check_gc
check_PROGRAMS = check_gc

check_gc_SOURCES = check_gc.c $(top_builddir)/src/arcueid.h
check_gc_CFLAGS = @CHECK_CFLAGS@
check_gc_LDADD = @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@
