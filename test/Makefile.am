# Copyright (C) 2017, 2018 Rafael R. Sevilla
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
TESTS = check_alloc check_cons check_env check_gc check_hash check_rune \
	check_string check_symbol
check_PROGRAMS = check_alloc check_cons check_env check_gc check_hash \
	check_rune check_string check_symbol

check_alloc_SOURCES = check_alloc.c $(top_builddir)/src/arcueid.h $(top_builddir)/src/alloc.h
check_alloc_CFLAGS = @CHECK_CFLAGS@
check_alloc_LDADD = @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_cons_SOURCES = check_cons.c $(top_builddir)/src/arcueid.h
check_cons_CFLAGS = @CHECK_CFLAGS@
check_cons_LDADD = @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_env_SOURCES = check_env.c $(top_builddir)/src/arcueid.h
check_env_CFLAGS = @CHECK_CFLAGS@
check_env_LDADD = @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_gc_SOURCES = check_gc.c $(top_builddir)/src/arcueid.h $(top_builddir)/src/alloc.h $(top_builddir)/src/gc.h
check_gc_CFLAGS = @CHECK_CFLAGS@
check_gc_LDADD = @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_hash_SOURCES = check_hash.c $(top_builddir)/src/arcueid.h $(top_builddir)/src/hash.h
check_hash_CFLAGS = @CHECK_CFLAGS@
check_hash_LDADD = @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_rune_SOURCES = check_rune.c $(top_builddir)/src/arcueid.h
check_rune_CFLAGS = @CHECK_CFLAGS@
check_rune_LDADD = @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_string_SOURCES = check_string.c $(top_builddir)/src/arcueid.h
check_string_CFLAGS = @CHECK_CFLAGS@
check_string_LDADD = @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_symbol_SOURCES = check_symbol.c $(top_builddir)/src/arcueid.h
check_symbol_CFLAGS = @CHECK_CFLAGS@
check_symbol_LDADD = @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@
